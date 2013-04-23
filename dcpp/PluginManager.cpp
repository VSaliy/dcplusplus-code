/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#include "Archive.h"
#include "Client.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "File.h"
#include "LogManager.h"
#include "QueueManager.h"
#include "ScopedFunctor.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "UserConnection.h"
#include "version.h"

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

PluginManager::PluginManager() : dcCore(), shutdown(false), secNum(Util::rand()) {
}

PluginManager::~PluginManager() {
}

DcextInfo PluginManager::extract(const string& path) {
	const auto dir = Util::getTempPath() + "dcext" PATH_SEPARATOR_STR;
	const auto info_path = dir + "info.xml";
	File::deleteFile(info_path);
	Archive(path).extract(dir);

	SimpleXML xml;
	xml.fromXML(File(info_path, File::READ, File::OPEN).read());

	DcextInfo info;

	if(xml.findChild("dcext")) {
		xml.stepIn();

		auto parse = [&xml](string tag, string& out) {
			xml.resetCurrentChild();
			if(xml.findChild(tag)) {
				out = xml.getChildData();
			}
		};

		auto checkPlatform = [&xml](bool emptyAllowed) {
			auto platform = xml.getChildAttrib("Platform");
			if(emptyAllowed && platform.empty()) {
				return true;
			}
#if defined(_WIN32) && defined(_WIN64)
			return platform == "pe-x64";
#elif defined(_WIN32)
			return platform == "pe-x86";
#elif defined(__x86_64__)
			return platform == "elf-x64";
#elif defined(__i386__)
			return platform == "elf-x86";
#else
#error Unknown platform
#endif
		};

		string version;
		parse("ApiVersion", version);
		if(Util::toInt(version) < DCAPI_CORE_VER) {
			throw str(F_("%1% is too old, contact the plugin author for an update") % Util::getFileName(path));
		}

		parse("UUID", info.uuid);
		parse("Name", info.name);
		version.clear(); parse("Version", version); info.version = Util::toDouble(version);
		parse("Author", info.author);
		parse("Description", info.description);
		parse("Website", info.website);

		xml.resetCurrentChild();
		while(xml.findChild("Plugin")) {
			if(checkPlatform(false)) {
				info.plugin = xml.getChildData();
			}
		}

		xml.resetCurrentChild();
		if(xml.findChild("Files")) {
			xml.stepIn();

			while(xml.findChild("File")) {
				if(checkPlatform(true)) {
					info.files.push_back(xml.getChildData());
				}
			}

			xml.stepOut();
		}

		xml.stepOut();
	}

	if(info.uuid.empty() || info.name.empty() || info.version == 0) {
		throw Exception(str(F_("%1% is not a valid plugin") % Util::getFileName(path)));
	}
	if(info.plugin.empty()) {
		throw Exception(str(F_("%1% is not compatible with %2%") % Util::getFileName(path) % APPNAME));
	}

	if(isLoaded(info.uuid)) {
		throw Exception(str(F_("%1% is already installed") % Util::getFileName(path)));
	}

	return info;
}

void PluginManager::install(const string& uuid, const string& plugin, const StringList& files) {
	if(uuid.empty() || plugin.empty()) {
		throw Exception();
	}

	const auto source = Util::getTempPath() + "dcext" PATH_SEPARATOR_STR;
	const auto target = getInstallPath(uuid);
	const auto lib = target + Util::getFileName(plugin);

	File::ensureDirectory(lib);
	File::renameFile(source + plugin, lib);

	for(auto& file: files) {
		File::ensureDirectory(target + file);
		File::renameFile(source + file, target + file);
	}

	loadPlugin(lib, true);
}

void PluginManager::loadPlugins(function<void (const string&)> f) {
	TimerManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	loadSettings();

	StringTokenizer<string> st(getPluginSetting("CoreSetup", "Plugins"), ";");
	for(auto& i: st.getTokens()) {
		if(f) { f(Util::getFileName(i)); }
		try { loadPlugin(i); }
		catch(const Exception& e) { LogManager::getInstance()->message(e.getError()); }
	}
}

void PluginManager::loadPlugin(const string& fileName, bool install) {
	Lock l(cs);

	dcassert(dcCore.apiVersion != 0);

	PluginHandle hr = LOAD_LIBRARY(fileName);
	if(!hr) {
		throw Exception(str(F_("Error loading %1%: %2%") % Util::getFileName(fileName) % GET_ERROR()));
	}
	ScopedFunctor([&hr] { if(hr) FREE_LIBRARY(hr); });

	PluginInfo::PLUGIN_INIT pluginInfo = reinterpret_cast<PluginInfo::PLUGIN_INIT>(GET_ADDRESS(hr, "pluginInit"));
	MetaData info { };
	DCMAIN dcMain;
	if(!pluginInfo || !(dcMain = pluginInfo(&info))) {
		throw Exception(str(F_("%1% is not a valid plugin") % Util::getFileName(fileName)));
	}

	checkPlugin(info);

	if(dcMain((install ? ON_INSTALL : ON_LOAD), &dcCore, nullptr) == False) {
		throw Exception(str(F_("Error loading %1%") % Util::getFileName(fileName)));
	}

	plugins.emplace_back(new PluginInfo(fileName, hr, info, dcMain));
	hr = nullptr; // bypass the scoped deleter.
}

bool PluginManager::isLoaded(const string& guid) {
	Lock l(cs);
	auto pluginComp = [&guid](const unique_ptr<PluginInfo>& p) -> bool { return strcmp(p->getInfo().guid, guid.c_str()) == 0; };
	auto i = std::find_if(plugins.begin(), plugins.end(), pluginComp);
	return (i != plugins.end());
}

void PluginManager::checkPlugin(const MetaData& info) {
	// Check if user is trying to load a duplicate
	if(isLoaded(info.guid)) {
		throw Exception(str(F_("%1% is already installed") % info.name));
	}

	// Check API compatibility (this should only block on absolutely wrecking api changes, which generally should not happen)
	if(info.apiVersion < DCAPI_CORE_VER) {
		throw Exception(str(F_("%1% is too old, contact the plugin author for an update") % info.name));
	}

	// Check that all dependencies are loaded
	if(info.numDependencies != 0) {
		for(size_t i = 0; i < info.numDependencies; ++i) {
			if(!isLoaded(info.dependencies[i])) {
				throw Exception(str(F_("Missing dependencies for %1%") % info.name));
			}
		}
	}
}

void PluginManager::unloadPlugins() {
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);

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

	saveSettings();
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
	vector<PluginInfo*> ret(plugins.size());
	std::transform(plugins.begin(), plugins.end(), ret.begin(),
		[](const unique_ptr<PluginInfo>& p) { return p.get(); });
	return ret;
};

const PluginInfo* PluginManager::getPlugin(size_t index) const {
	Lock l(cs);
	return plugins[index].get();
}

// Functions that call the plugin
bool PluginManager::onChatTags(Tagger& tagger, OnlineUser* from) {
	TagData data = { reinterpret_cast<dcptr_t>(&tagger), True };
	return runHook(HOOK_UI_CHAT_TAGS, from, &data);
}

bool PluginManager::onChatDisplay(string& htmlMessage, OnlineUser* from) {
	StringData data = { htmlMessage.c_str() };
	bool handled = runHook(HOOK_UI_CHAT_DISPLAY, from, &data);
	if(handled && data.out) {
		htmlMessage = data.out;
		return true;
	}
	return false;
}

bool PluginManager::onChatCommand(Client* client, const string& line) {
	string cmd, param;
	auto si = line.find(' ');
	if(si != string::npos) {
		param = line.substr(si + 1);
		cmd = line.substr(1, si - 1);
	} else {
		cmd = line.substr(1);
	}

	CommandData data = { cmd.c_str(), param.c_str() };
	return runHook(HOOK_UI_CHAT_COMMAND, client, &data);
}

bool PluginManager::onChatCommandPM(const HintedUser& user, const string& line) {
	// Hopefully this is safe...
	bool res = false;
	auto lock = ClientManager::getInstance()->lock();
	OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(user.user->getCID(), user.hint);

	if(ou) {
		string cmd, param;
		auto si = line.find(' ');
		if(si != string::npos) {
			param = line.substr(si + 1);
			cmd = line.substr(1, si - 1);
		} else {
			cmd = line.substr(1);
		}

		CommandData data = { cmd.c_str(), param.c_str() };
		res = runHook(HOOK_UI_CHAT_COMMAND_PM, ou, &data);
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
	{
		Lock l(hook->cs);
		auto subscription = make_unique<HookSubscriber>();
		auto pSub = subscription.get();
		subscription->hookProc = hookProc;
		subscription->common = pCommon;
		subscription->owner = hook->guid;
		hook->subscribers.push_back(move(subscription));

		return pSub;
	}
}

bool PluginManager::runHook(PluginHook* hook, dcptr_t pObject, dcptr_t pData) {
	dcassert(hook);

	// Using shared lock (csHook) might be a tad safer, but it is also slower and I prefer not to teach plugins to rely on the effect it creates
	Lock l(hook->cs);

	Bool bBreak = False;
	Bool bRes = False;
	for(auto& sub: hook->subscribers) {
		if(sub->hookProc(pObject, pData, sub->common, &bBreak))
			bRes = True;
		if(bBreak) return (bRes != False);
	}

	// Call default hook procedure for all unused hooks
	if(hook->defProc && hook->subscribers.empty()) {
		if(hook->defProc(pObject, pData, NULL, &bBreak))
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
	{
		Lock l(hook->cs);
		hook->subscribers.erase(std::remove_if(hook->subscribers.begin(), hook->subscribers.end(),
			[subscription](const unique_ptr<HookSubscriber>& sub) { return sub.get() == subscription; }), hook->subscribers.end());

		return hook->subscribers.size();
	}
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

string PluginManager::getInstallPath(const string& uuid) {
	return Util::getPath(Util::PATH_USER_LOCAL) + "Plugins" PATH_SEPARATOR_STR + uuid + PATH_SEPARATOR_STR;
}

// Listeners
void PluginManager::on(ClientManagerListener::ClientConnected, Client* aClient) noexcept {
	runHook(HOOK_HUB_ONLINE, aClient);
}

void PluginManager::on(ClientManagerListener::ClientDisconnected, Client* aClient) noexcept {
	runHook(HOOK_HUB_OFFLINE, aClient);
}

void PluginManager::on(QueueManagerListener::Added, QueueItem* qi) noexcept {
	runHook(HOOK_QUEUE_ADDED, qi);
}

void PluginManager::on(QueueManagerListener::Moved, QueueItem* qi, const string& /*aSource*/) noexcept {
	runHook(HOOK_QUEUE_MOVED, qi);
}

void PluginManager::on(QueueManagerListener::Removed, QueueItem* qi) noexcept {
	runHook(HOOK_QUEUE_REMOVED, qi);
}

void PluginManager::on(QueueManagerListener::Finished, QueueItem* qi, const string& /*dir*/, int64_t /*speed*/) noexcept {
	runHook(HOOK_QUEUE_FINISHED, qi);
}

// Load / Save settings
void PluginManager::loadSettings() noexcept {
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
				for(auto& i: settings) {
					setPluginSetting(pluginGuid, i.first, i.second);
				}
				xml.stepOut();
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("PluginManager::loadSettings: %s\n", e.getError().c_str());
	}
}

void PluginManager::saveSettings() noexcept {
	Lock l(cs);

	try {
		SimpleXML xml;
		xml.addTag("Plugins");
		xml.stepIn();
		for(auto& i: settings) {
			xml.addTag("Plugin");
			xml.stepIn();
			for(auto& j: i.second) {
				xml.addTag(j.first, j.second);			
			}
			xml.stepOut();
			xml.addChildAttrib("Guid", i.first);
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

void PluginManager::on(SettingsManagerListener::Load, SimpleXML& /*xml*/) noexcept {
	loadSettings();
}

void PluginManager::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	saveSettings();
}

} // namespace dcpp
