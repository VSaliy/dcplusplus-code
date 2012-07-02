/* 
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "PluginManager.h"

#include "Client.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "QueueManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "UserConnection.h"

#include <utility>

#ifdef _WIN32
# define PLUGIN_EXT "*.dll"

# define LOAD_LIBRARY(filename) ::LoadLibrary(Text::toT(filename).c_str())
# define FREE_LIBRARY(lib) ::FreeLibrary(lib)
# define GET_ADDRESS(lib, name) ::GetProcAddress(lib, name)
# define GET_ERROR() Util::translateError(GetLastError())
#else
# include "dlfcn.h"
# define PLUGIN_EXT "*.so"

# define LOAD_LIBRARY(filename) ::dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL)
# define FREE_LIBRARY(lib) ::dlclose(lib)
# define GET_ADDRESS(lib, name) ::dlsym(lib, name)
# define GET_ERROR() ::dlerror()
#endif

namespace dcpp {

using std::swap;

PluginInfo::~PluginInfo() {
	bool isSafe = true;
	if(dcMain((PluginManager::getInstance()->getShutdown() ? ON_UNLOAD : ON_UNINSTALL), NULL, NULL) == False) {
		// Plugin performs operation critical tasks (runtime unload not possible)
		isSafe = !PluginManager::getInstance()->addInactivePlugin(handle);
	} if(isSafe && handle != NULL) {
		FREE_LIBRARY(handle);
		handle = NULL;
	}
}

void PluginManager::loadPlugins(void (*f)(void*, const string&), void* p) {
	PluginApiImpl::initAPI(dcCore);

	TimerManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);

	StringTokenizer<string> st(getPluginSetting("CoreSetup", "Plugins"), ";");
	for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
		if(!loadPlugin(*i) || !f) continue;
		(*f)(p, Util::getFileName(*i));
	}
}

bool PluginManager::loadPlugin(const string& fileName, bool isInstall /*= false*/) {
	Lock l(cs);

	dcassert(dcCore.apiVersion != 0);

	PluginHandle hr = LOAD_LIBRARY(fileName);
	if(!hr) {
		LogManager::getInstance()->message(str(F_("Error loading %1%: %2%") % Util::getFileName(fileName) % GET_ERROR()));
		return false;
	}

	PluginInfo::PLUGIN_INIT pluginInfo = reinterpret_cast<PluginInfo::PLUGIN_INIT>(GET_ADDRESS(hr, "pluginInit"));

	if(pluginInfo != NULL) {
		MetaData info;
		memzero(&info, sizeof(MetaData));

		DCMAIN dcMain;
		if((dcMain = pluginInfo(&info)) != NULL) {
			if(checkPlugin(info)) {
				if(dcMain((isInstall ? ON_INSTALL : ON_LOAD), &dcCore, NULL) != False) {
					plugins.emplace_back(new PluginInfo(fileName, hr, info, dcMain));
					return true;
				}
			}
		}
	} else LogManager::getInstance()->message(str(F_("%1% is not a valid plugin") % Util::getFileName(fileName)));

	FREE_LIBRARY(hr);
	return false;
}

bool PluginManager::isLoaded(const string& guid) {
	auto pluginComp = [&guid](const unique_ptr<PluginInfo>& p) -> bool { return strcmp(p->getInfo().guid, guid.c_str()) == 0; };
	auto i = std::find_if(plugins.begin(), plugins.end(), pluginComp);
	return (i != plugins.end());
}

bool PluginManager::checkPlugin(const MetaData& info) {
	// Check if user is trying to load a duplicate
	if(isLoaded(info.guid)) {
		LogManager::getInstance()->message(str(F_("%1% is already installed") % info.name));
		return false;
	}

	// Check API compatibility (this should only block on absolutely wrecking api changes, which generally should not happen)
	if(info.apiVersion < DCAPI_CORE_VER) {
		LogManager::getInstance()->message(str(F_("%1% is too old, contact the plugin author for an update") % info.name));
		return false;
	}

	// Check that all dependencies are loaded
	if(info.numDependencies != 0) {
		for(size_t i = 0; i < info.numDependencies; ++i) {
			if(!isLoaded(info.dependencies[i])) {
				LogManager::getInstance()->message(str(F_("Missing dependencies for %1%") % info.name));
				return false;
			}
		}
	}

	return true;
}

void PluginManager::unloadPlugins() {
	Lock l(cs);
	shutdown = true;

	// Update plugin order
	string installed;
	for(auto& i: plugins) {
		if(!installed.empty()) installed += ";";
		installed += i->getFile();
	}
	setPluginSetting("CoreSetup", "Plugins", installed);

	// Off we go...
	plugins.clear();

	// Really unload plugins that have been flagged inactive (ON_UNLOAD returns False)
	for(auto& i: inactive)
		FREE_LIBRARY(i);

	// Destroy hooks that may have not been correctly freed
	hooks.clear();

	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);

	PluginApiImpl::releaseAPI();
}

void PluginManager::unloadPlugin(size_t index) {
	Lock l(cs);
	plugins.erase(plugins.begin() + index);
}

bool PluginManager::addInactivePlugin(PluginHandle h) {
	if(std::find(inactive.begin(), inactive.end(), h) == inactive.end()) {
		inactive.push_back(h);
		return true;
	}
	return false;
}

void PluginManager::movePlugin(size_t index, int pos) {
	Lock l(cs);
	auto i = plugins.begin() + index;
	swap(*i, *(i + pos));
}

vector<PluginInfo*> PluginManager::getPluginList() const {
	Lock l(cs);
	vector<PluginInfo*> ret;
	std::transform(plugins.begin(), plugins.end(), ret.begin(),
		[](const unique_ptr<PluginInfo>& p) { return p.get(); });
	return ret;
};

const PluginInfo* PluginManager::getPlugin(size_t index) const {
	Lock l(cs);
	return plugins[index].get();
}

// Functions that call the plugin
bool PluginManager::onChatDisplay(string& htmlMessage, OnlineUser* from) {
	StringData data;
	memzero(&data, sizeof(StringData));

	data.in = htmlMessage.c_str();

	bool handled = from ? runHook(HOOK_UI_CHAT_DISPLAY, from, &data) : runHook(HOOK_UI_LOCAL_CHAT_DISPLAY, NULL, &data);
	if(handled && data.out != NULL) {
		htmlMessage = data.out;
		return true;
	}

	return false;
}

bool PluginManager::onChatCommand(Client* client, const string& line) {
	CommandData data;
	memzero(&data, sizeof(CommandData));

	string cmd, param;
	string::size_type si = line.find(' ');
	if(si != string::npos) {
		param = line.substr(si + 1);
		cmd = line.substr(1, si - 1);
	} else {
		cmd = line.substr(1);
	}

	data.command = cmd.c_str();
	data.params = param.c_str();
	data.isPrivate = False;

	return runHook(HOOK_UI_PROCESS_CHAT_CMD, client, &data);
}

bool PluginManager::onChatCommandPM(const HintedUser& user, const string& line) {
	// Hopefully this is safe...
	bool res = false;
	auto lock = ClientManager::getInstance()->lock();
	OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(user.user->getCID(), user.hint);

	if(ou) {
		CommandData data;
		memzero(&data, sizeof(CommandData));

		string cmd, param;
		string::size_type si = line.find(' ');
		if(si != string::npos) {
			param = line.substr(si + 1);
			cmd = line.substr(1, si - 1);
		} else {
			cmd = line.substr(1);
		}

		data.command = cmd.c_str();
		data.params = param.c_str();
		data.isPrivate = True;

		res = runHook(HOOK_UI_PROCESS_CHAT_CMD, ou, &data);
	}

	return res;
}

// Plugin interface registry
intfHandle PluginManager::registerInterface(const string& guid, dcptr_t funcs) {
	if(interfaces.find(guid) == interfaces.end())
		interfaces.insert(make_pair(guid, funcs));
	// Following ensures that only the original provider may remove this
	return reinterpret_cast<intfHandle>((uintptr_t)funcs ^ secNum);
}

dcptr_t PluginManager::queryInterface(const string& guid) {
	auto i = interfaces.find(guid);
	if(i != interfaces.end()) {
		return i->second;
	} else return NULL;
}

bool PluginManager::releaseInterface(intfHandle hInterface) {
	// Following ensures that only the original provider may remove this
	dcptr_t funcs = reinterpret_cast<dcptr_t>((uintptr_t)hInterface ^ secNum);
	for(auto i = interfaces.begin(); i != interfaces.end(); ++i) {
		if(i->second == funcs) {
			interfaces.erase(i);
			return true;
		}
	}
	return false;
}

// Plugin Hook system
PluginHook* PluginManager::createHook(const string& guid, DCHOOK defProc) {
	Lock l(csHook);

	auto i = hooks.find(guid);
	if(i != hooks.end()) return NULL;

	auto hook = make_unique<PluginHook>();
	auto pHook = hook.get();
	hook->guid = guid;
	hook->defProc = defProc;
	hooks[guid] = move(hook);
	return pHook;
}

bool PluginManager::destroyHook(PluginHook* hook) {
	Lock l(csHook);

	auto i = hooks.find(hook->guid);
	if(i != hooks.end()) {
		hooks.erase(i);
		return true;
	}
	return false;
}

HookSubscriber* PluginManager::bindHook(const string& guid, DCHOOK hookProc, void* pCommon) {
	Lock l(csHook);

	auto i = hooks.find(guid);
	if(i == hooks.end()) {
		return NULL;
	}
	auto& hook = i->second;

	auto subscription = make_unique<HookSubscriber>();
	auto pSub = subscription.get();
	subscription->hookProc = hookProc;
	subscription->common = pCommon;
	subscription->owner = hook->guid;
	hook->subscribers.push_back(move(subscription));

	return pSub;
}

bool PluginManager::runHook(PluginHook* hook, dcptr_t pObject, dcptr_t pData) {
	dcassert(hook);

	// Using shared lock (csHook) might be a tad safer, but it is also slower and I prefer not to teach plugins to rely on the effect it creates
	Lock l(hook->cs);

	Bool bBreak = False;
	Bool bRes = False;
	for(auto& sub: hook->subscribers) {
		if(sub->common ? sub->hookProcCommon(pObject, pData, sub->common, &bBreak) : sub->hookProc(pObject, pData, &bBreak))
			bRes = True;
		if(bBreak) return (bRes != False);
	}

	// Call default hook procedure for all unused hooks
	if(hook->defProc && hook->subscribers.empty()) {
		if(hook->defProc(pObject, pData, &bBreak))
			bRes = True;
	}

	return (bRes != False);
}

size_t PluginManager::releaseHook(HookSubscriber* subscription) {
	Lock l(csHook);

	if(subscription == NULL)
		return 0;

	auto i = hooks.find(subscription->owner);
	if(i == hooks.end()) {
		return 0;
	}
	auto& hook = i->second;

	hook->subscribers.erase(std::remove_if(hook->subscribers.begin(), hook->subscribers.end(),
		[subscription](const unique_ptr<HookSubscriber>& sub) { return sub.get() == subscription; }), hook->subscribers.end());

	return hook->subscribers.size();
}

// Plugin configuration
bool PluginManager::hasSettings(const string& pluginName) {
	Lock l(cs);

	auto i = settings.find(pluginName);
	if(i != settings.end())
		return (i->second.size() > 0);
	return false;
}

void PluginManager::processSettings(const string& pluginName, const function<void (StringMap&)>& currentSettings) {
	Lock l(cs);

	auto i = settings.find(pluginName);
	if(i != settings.end() && currentSettings)
		currentSettings(i->second);
}

void PluginManager::setPluginSetting(const string& pluginName, const string& setting, const string& value) {
	Lock l(cs);
	settings[pluginName][setting] = value;
}

const string& PluginManager::getPluginSetting(const string& pluginName, const string& setting) {
	Lock l(cs);

	auto i = settings.find(pluginName);
	if(i != settings.end()) {
		auto j = i->second.find(setting);
		if(j != i->second.end())
			return j->second;
	}
	return Util::emptyString;
}

void PluginManager::removePluginSetting(const string& pluginName, const string& setting) {
	Lock l(cs);

	auto i = settings.find(pluginName);
	if(i != settings.end()) {
		auto j = i->second.find(setting);
		if(j != i->second.end())
			i->second.erase(j);
	}
}

// Listeners
void PluginManager::on(ClientManagerListener::ClientConnected, Client* aClient) noexcept {
	runHook(HOOK_HUB_ONLINE, aClient);
}

void PluginManager::on(ClientManagerListener::ClientDisconnected, Client* aClient) noexcept {
	runHook(HOOK_HUB_OFFLINE, aClient);
}

void PluginManager::on(QueueManagerListener::Added, QueueItem* qi) noexcept {
	runHook(HOOK_QUEUE_ADD, qi);
}

void PluginManager::on(QueueManagerListener::Moved, QueueItem* qi, const string& /*aSource*/) noexcept {
	runHook(HOOK_QUEUE_MOVE, qi);
}

void PluginManager::on(QueueManagerListener::Removed, QueueItem* qi) noexcept {
	runHook(HOOK_QUEUE_REMOVE, qi);
}

void PluginManager::on(QueueManagerListener::Finished, QueueItem* qi, const string& /*dir*/, int64_t /*speed*/) noexcept {
	runHook(HOOK_QUEUE_FINISHED, qi);
}

// Load / Save settings
void PluginManager::on(SettingsManagerListener::Load, SimpleXML& /*xml*/) noexcept {
	Lock l(cs);

	try {
		Util::migrate(Util::getPath(Util::PATH_USER_LOCAL) + "Plugins.xml");

		SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_LOCAL) + "Plugins.xml", File::READ, File::OPEN).read());
		if(xml.findChild("Plugins")) {
			xml.stepIn();
			while(xml.findChild("Plugin")) {
				const string& pluginGuid = xml.getChildAttrib("Guid");
				xml.stepIn();
				auto settings = xml.getCurrentChildren();
				for(auto j = settings.cbegin(); j != settings.cend(); ++j) {
					setPluginSetting(pluginGuid, j->first, j->second);
				}
				xml.stepOut();
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("PluginManager::loadSettings: %s\n", e.getError().c_str());
	}
 }

void PluginManager::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	Lock l(cs);

	try {
		SimpleXML xml;
		xml.addTag("Plugins");
		xml.stepIn();
		for(auto i = settings.cbegin(); i != settings.cend(); ++i) {
			xml.addTag("Plugin");
			xml.stepIn();
			for(auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
				xml.addTag(j->first, j->second);			
			}
			xml.stepOut();
			xml.addChildAttrib("Guid", i->first);
		}
		xml.stepOut();

		string fname = Util::getPath(Util::PATH_USER_LOCAL) + "Plugins.xml";
		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("PluginManager::saveSettings: %s\n", e.getError().c_str());
	}
}

} // namespace dcpp

/**
 * @file
 * $Id: PluginManager.cpp 1245 2012-01-21 15:09:54Z crise $
 */
