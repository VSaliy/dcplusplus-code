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

#ifndef DCPLUSPLUS_DCPP_PLUGIN_MANAGER_H
#define DCPLUSPLUS_DCPP_PLUGIN_MANAGER_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "typedefs.h"

#include "ClientManagerListener.h"
#include "PluginDefs.h"
#include "PluginEntity.h"
#include "QueueManagerListener.h"
#include "SettingsManager.h"
#include "Singleton.h"
#include "Tagger.h"
#include "TimerManager.h"

#ifdef _WIN32
typedef HMODULE PluginHandle;
#else
typedef void* PluginHandle;
#endif

namespace dcpp {

using std::function;
using std::map;
using std::string;
using std::unique_ptr;
using std::vector;

// Represents a plugin in hook context
struct HookSubscriber {
	DCHOOK hookProc;
	void* common;
	string owner;
};

// Hookable event
struct PluginHook {
	string guid;
	DCHOOK defProc;

	vector<unique_ptr<HookSubscriber>> subscribers;
	CriticalSection cs;
};

// Holds a loaded plugin
class PluginInfo : private boost::noncopyable
{
public:
	typedef	DCMAIN	(DCAPI *PLUGIN_INIT)(MetaDataPtr info);

	PluginInfo(const string& aFile, PluginHandle hInst, MetaData aInfo, DCMAIN aMain)
		: dcMain(aMain), info(aInfo), file(aFile), handle(hInst) { };

	~PluginInfo();

	DCMAIN dcMain;
	const MetaData& getInfo() const { return info; }
	const string& getFile() const { return file; }

private:
	MetaData info;
	string file;
	PluginHandle handle;
};

class PluginManager : public Singleton<PluginManager>, private TimerManagerListener,
	private ClientManagerListener, private QueueManagerListener, private SettingsManagerListener
{
public:
	PluginManager();
	~PluginManager();

	void loadPlugins(function<void (const string&)> f);
	bool loadPlugin(const string& fileName, function<void (const string&)> err, bool install = false);
	bool isLoaded(const string& guid);

	void unloadPlugins();
	void unloadPlugin(size_t index);

	bool addInactivePlugin(PluginHandle h);
	bool getShutdown() const { return shutdown; }

	void movePlugin(size_t index, int pos);
	vector<PluginInfo*> getPluginList() const;
	const PluginInfo* getPlugin(size_t index) const;

	DCCorePtr getCore() { return &dcCore; }

	// Functions that call the plugin
	bool onChatTags(Tagger& tagger, OnlineUser* from = nullptr);
	bool onChatDisplay(string& htmlMessage, OnlineUser* from = nullptr);
	bool onChatCommand(Client* client, const string& line);
	bool onChatCommandPM(const HintedUser& user, const string& line);

	// runHook wrappers for host calls
	bool runHook(const string& guid, dcptr_t pObject, dcptr_t pData) {
		if(shutdown) return false;
		auto i = hooks.find(guid);
		dcassert(i != hooks.end());
		return runHook(i->second.get(), pObject, pData);
	}

	template<class T>
	bool runHook(const string& guid, PluginEntity<T>* entity, dcptr_t pData = NULL) {
		if(entity) {
			Lock l(entity->cs);
			return runHook(guid, entity->getPluginObject(), pData);
		}
		return runHook(guid, nullptr, pData);
	}

	template<class T>
	bool runHook(const string& guid, PluginEntity<T>* entity, const string& data) {
		return runHook<T>(guid, entity, (dcptr_t)data.c_str());
	}

	// Plugin interface registry
	intfHandle registerInterface(const string& guid, dcptr_t funcs);
	dcptr_t queryInterface(const string& guid);
	bool releaseInterface(intfHandle hInterface);

	// Plugin Hook system
	PluginHook* createHook(const string& guid, DCHOOK defProc);
	bool destroyHook(PluginHook* hook);

	HookSubscriber* bindHook(const string& guid, DCHOOK hookProc, void* pCommon);
	bool runHook(PluginHook* hook, dcptr_t pObject, dcptr_t pData);
	size_t releaseHook(HookSubscriber* subscription);

	// Plugin configuration
	bool hasSettings(const string& pluginName);
	void processSettings(const string& pluginName, const function<void (StringMap&)>& currentSettings);

	void setPluginSetting(const string& pluginName, const string& setting, const string& value);
	const string& getPluginSetting(const string& pluginName, const string& setting);
	void removePluginSetting(const string& pluginName, const string& setting);

private:
	void loadSettings() noexcept;
	void saveSettings() noexcept;

	// Check if plugin can be loaded
	bool checkPlugin(const MetaData& info, function<void (const string&)> err);

	// Listeners
	void on(TimerManagerListener::Second, uint64_t ticks) noexcept { runHook(HOOK_TIMER_SECOND, NULL, &ticks); }
	void on(TimerManagerListener::Minute, uint64_t ticks) noexcept { runHook(HOOK_TIMER_MINUTE, NULL, &ticks); }

	void on(ClientManagerListener::ClientConnected, Client* aClient) noexcept;
	void on(ClientManagerListener::ClientDisconnected, Client* aClient) noexcept;

	void on(QueueManagerListener::Added, QueueItem* qi) noexcept;
	void on(QueueManagerListener::Moved, QueueItem* qi, const string& /*aSource*/) noexcept;
	void on(QueueManagerListener::Removed, QueueItem* qi) noexcept;
	void on(QueueManagerListener::Finished, QueueItem* qi, const string& /*dir*/, int64_t /*speed*/) noexcept;

	void on(SettingsManagerListener::Load, SimpleXML& /*xml*/) noexcept;
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	vector<unique_ptr<PluginInfo>> plugins;
	vector<PluginHandle> inactive;

	map<string, unique_ptr<PluginHook>> hooks;
	map<string, dcptr_t> interfaces;

	map<string, StringMap> settings;

	DCCore dcCore;
	mutable CriticalSection cs, csHook;
	bool shutdown;

	uintptr_t secNum;
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_PLUGIN_MANAGER_H)
