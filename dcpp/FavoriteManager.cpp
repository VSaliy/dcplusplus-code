/*
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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
#include "FavoriteManager.h"

#include "BZUtils.h"
#include "ClientManager.h"
#include "CryptoManager.h"
#include "File.h"
#include "FilteredFile.h"
#include "HttpConnection.h"
#include "HttpManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "UserCommand.h"
#include "WindowInfo.h"

namespace dcpp {

using std::make_pair;
using std::swap;

using boost::none;

FavoriteManager::FavoriteManager() : lastId(0), useHttp(false), running(false), c(nullptr), lastServer(0), listType(TYPE_NORMAL), dontSave(false) {
	ClientManager::getInstance()->addListener(this);
	HttpManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	File::ensureDirectory(Util::getHubListsPath());

	/* after the release of the first version including this blacklist (DC++ 0.780), remember to
	also update version.xml when a domain has to be added to this list, using the following format:
	<Blacklist>
		<Blacklisted Domain="example1.com" Reason="Domain used for spam purposes."/>
		<Blacklisted Domain="example2.com" Reason="Domain used for spam purposes."/>
	</Blacklist>
	(the "Blacklist" tag should be under the main "DCUpdate" tag.) */
	addBlacklist("adchublist.com", "Domain used for spam purposes.");
	addBlacklist("hublist.org", "Domain used for spam purposes.");
	addBlacklist("hubtracker.com", "Domain lost to unknown owners advertising dubious pharmaceuticals.");
	addBlacklist("openhublist.org", "Domain used for spam purposes.");
	addBlacklist("dchublist.com", "Redirection to the new domain fails. To access this hublist add its new address <https://www.te-home.net/?do=hublist&get=hublist.xml.bz2> instead.");
	addBlacklist("hublista.hu", "Server discontinued, domain may lost to unknown owners.");
}

FavoriteManager::~FavoriteManager() {
	for_each(favoriteHubs.begin(), favoriteHubs.end(), DeleteFunction());
}

void FavoriteManager::shutdown() {
	ClientManager::getInstance()->removeListener(this);
	HttpManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
}

UserCommand FavoriteManager::addUserCommand(int type, int ctx, int flags, const string& name, const string& command, const string& to, const string& hub) {
	Lock l(cs);

	// The following management is to protect users against malicious hubs or clients.
	// Hubs (or clients) can send an arbitrary amount of user commands, which means that there is a possibility that
	// the client will need to manage thousands and thousands of user commands.
	// This can naturally cause problems with memory etc, so the client may even crash at some point.
	// The following management tries to remedy this problem by doing two things;
	// a) Replaces previous user commands (if they have the same name etc)
	// b) Restricts the amount of user commands that pertain to a particlar hub
	// Note that this management only cares about externally created user commands, 
	// which means that the user themselves can create however large user commands.
	if(flags == UserCommand::FLAG_NOSAVE)
	{
		const int maximumUCs = 5000; // Completely arbitrary
		int externalCommands = 0; // Used to count the number of external commands

		for(auto& uc: userCommands) {
			if((uc.isSet(UserCommand::FLAG_NOSAVE)) &&	// Only care about external commands...
				(uc.getHub() == hub))	// ... and those pertaining to this particular hub.
			{
				++externalCommands;

				// If the UC is generally identical otherwise, change the command
				if((uc.getName() == name) &&
					(uc.getCtx() == ctx) &&
					(uc.getType() == type) &&
					(uc.isSet(flags)) &&
					(uc.getTo() == to))
				{
					uc.setCommand(command);
					return uc;
				}
			}
				
		}

		// Validate if there's too many user commands
		if(maximumUCs <= externalCommands)
		{
			return userCommands.back();
		}
	}

	userCommands.emplace_back(lastId++, type, ctx, flags, name, command, to, hub);
	UserCommand& uc = userCommands.back();
	if(!uc.isSet(UserCommand::FLAG_NOSAVE))
		save();
	return userCommands.back();
}

bool FavoriteManager::getUserCommand(int cid, UserCommand& uc) {
	Lock l(cs);
	for(auto& i: userCommands) {
		if(i.getId() == cid) {
			uc = i;
			return true;
		}
	}
	return false;
}

bool FavoriteManager::moveUserCommand(int cid, int pos) {
	dcassert(pos == -1 || pos == 1);
	Lock l(cs);
	for(auto i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getId() == cid) {
			swap(*i, *(i + pos));
			return true;
		}
	}
	return false;
}

void FavoriteManager::updateUserCommand(const UserCommand& uc) {
	bool nosave = true;
	Lock l(cs);
	for(auto& i: userCommands) {
		if(i.getId() == uc.getId()) {
			i = uc;
			nosave = uc.isSet(UserCommand::FLAG_NOSAVE);
			break;
		}
	}
	if(!nosave)
		save();
}

int FavoriteManager::findUserCommand(const string& aName, const string& aUrl) {
	Lock l(cs);
	for(auto& i: userCommands) {
		if(i.getName() == aName && i.getHub() == aUrl) {
			return i.getId();
		}
	}
	return -1;
}

void FavoriteManager::removeUserCommand(int cid) {
	bool nosave = true;
	Lock l(cs);

	for(auto i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getId() == cid) {
			nosave = i->isSet(UserCommand::FLAG_NOSAVE);
			userCommands.erase(i);
			break;
		}
	}

	if(!nosave) {
		l.unlock();
		save();
	}
}
void FavoriteManager::removeUserCommand(const string& srv) {
	Lock l(cs);
	userCommands.erase(std::remove_if(userCommands.begin(), userCommands.end(), [&](const UserCommand& uc) {
		return uc.getHub() == srv && uc.isSet(UserCommand::FLAG_NOSAVE);
	}), userCommands.end());
}

void FavoriteManager::removeHubUserCommands(int ctx, const string& hub) {
	Lock l(cs);
	userCommands.erase(std::remove_if(userCommands.begin(), userCommands.end(), [&](const UserCommand& uc) {
		return uc.getHub() == hub && uc.isSet(UserCommand::FLAG_NOSAVE) && uc.getCtx() & ctx;
	}), userCommands.end());
}

void FavoriteManager::addFavoriteUser(const UserPtr& aUser) {
	{
		Lock l(cs);
		if(users.find(aUser->getCID()) == users.end()) {
			StringList urls = ClientManager::getInstance()->getHubUrls(aUser->getCID());
			StringList nicks = ClientManager::getInstance()->getNicks(aUser->getCID());

			/// @todo make this an error probably...
			if(urls.empty())
				urls.push_back(Util::emptyString);
			if(nicks.empty())
				nicks.push_back(Util::emptyString);

			auto i = users.emplace(aUser->getCID(), FavoriteUser(aUser, nicks[0], urls[0])).first;
			fire(FavoriteManagerListener::UserAdded(), i->second);
			save();
		}
	}

	ClientManager::getInstance()->saveUser(aUser->getCID());
}

void FavoriteManager::removeFavoriteUser(const UserPtr& aUser) {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i != users.end()) {
		fire(FavoriteManagerListener::UserRemoved(), i->second);
		users.erase(i);
		save();
	}
}

std::string FavoriteManager::getUserURL(const UserPtr& aUser) const {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i != users.end()) {
		const FavoriteUser& fu = i->second;
		return fu.getUrl();
	}
	return Util::emptyString;
}

void FavoriteManager::addFavorite(const FavoriteHubEntry& aEntry) {
	if(getFavoriteHub(aEntry.getServer())) {
		return;
	}
	auto f = new FavoriteHubEntry(aEntry);
	favoriteHubs.push_back(f);
	fire(FavoriteManagerListener::FavoriteAdded(), f);
	save();
}

void FavoriteManager::removeFavorite(FavoriteHubEntry* entry) {
	auto i = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
	if(i == favoriteHubs.end()) {
		return;
	}

	fire(FavoriteManagerListener::FavoriteRemoved(), entry);
	favoriteHubs.erase(i);
	delete entry;
	save();
}

bool FavoriteManager::isFavoriteHub(const std::string& url) {
	return (bool)getFavoriteHub(url);
}

bool FavoriteManager::addFavoriteDir(const string& aDirectory, const string & aName){
	string path = aDirectory;

	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	for(auto& i: favoriteDirs) {
		if((Util::strnicmp(path, i.first, i.first.size()) == 0) && (Util::strnicmp(path, i.first, path.size()) == 0)) {
			return false;
		}
		if(Util::stricmp(aName, i.second) == 0) {
			return false;
		}
	}
	favoriteDirs.emplace_back(aDirectory, aName);
	save();
	return true;
}

bool FavoriteManager::removeFavoriteDir(const string& aName) {
	string d(aName);

	if(d[d.length() - 1] != PATH_SEPARATOR)
		d += PATH_SEPARATOR;

	for(auto j = favoriteDirs.begin(); j != favoriteDirs.end(); ++j) {
		if(Util::stricmp(j->first.c_str(), d.c_str()) == 0) {
			favoriteDirs.erase(j);
			save();
			return true;
		}
	}
	return false;
}

bool FavoriteManager::renameFavoriteDir(const string& aName, const string& anotherName) {

	for(auto& j: favoriteDirs) {
		if(Util::stricmp(j.second.c_str(), aName.c_str()) == 0) {
			j.second = anotherName;
			save();
			return true;
		}
	}
	return false;
}

class XmlListLoader : public SimpleXMLReader::CallBack {
public:
	XmlListLoader(HubEntryList& lst) : publicHubs(lst) { }
	void startTag(const string& name, StringPairList& attribs, bool) {
		if(name == "Hub") {
			const string& name = getAttrib(attribs, "Name", 0);
			const string& server = getAttrib(attribs, "Address", 1);
			const string& description = getAttrib(attribs, "Description", 2);
			const string& users = getAttrib(attribs, "Users", 3);
			const string& country = getAttrib(attribs, "Country", 4);
			const string& shared = getAttrib(attribs, "Shared", 5);
			const string& minShare = getAttrib(attribs, "Minshare", 5);
			const string& minSlots = getAttrib(attribs, "Minslots", 5);
			const string& maxHubs = getAttrib(attribs, "Maxhubs", 5);
			const string& maxUsers = getAttrib(attribs, "Maxusers", 5);
			const string& reliability = getAttrib(attribs, "Reliability", 5);
			const string& rating = getAttrib(attribs, "Rating", 5);
			publicHubs.emplace_back(name, server, description, users, country, shared, minShare, minSlots, maxHubs, maxUsers, reliability, rating);
		}
	}
private:
	HubEntryList& publicHubs;
};

bool FavoriteManager::onHttpFinished(const string& buf) noexcept {
	MemoryInputStream mis(buf);
	bool success = true;

	Lock l(cs);
	HubEntryList& list = publicListMatrix[publicListServer];
	list.clear();

	try {
		XmlListLoader loader(list);

		if(listType == TYPE_BZIP2 && !buf.empty()) {
			FilteredInputStream<UnBZFilter, false> f(&mis);
			SimpleXMLReader(&loader).parse(f);
		} else {
			SimpleXMLReader(&loader).parse(mis);
		}
	} catch(const Exception&) {
		success = false;
		fire(FavoriteManagerListener::Corrupted(), useHttp ? publicListServer : Util::emptyString);
	}

	if(useHttp) {
		try {
			File f(Util::getHubListsPath() + Util::validateFileName(publicListServer), File::WRITE, File::CREATE | File::TRUNCATE);
			f.write(buf);
			f.close();
		} catch(const FileException&) { }
	}

	return success;
}

void FavoriteManager::save() {
	if(dontSave)
		return;

	Lock l(cs);
	try {
		SimpleXML xml;

		xml.addTag("Favorites");
		xml.stepIn();

		xml.addTag("Hubs");
		xml.stepIn();

		for(auto& i: favHubGroups) {
			xml.addTag("Group");
			xml.addChildAttrib("Name", i.first);
			i.second.save(xml);
		}

		for(auto& i: favoriteHubs) {
			xml.addTag("Hub");
			xml.addChildAttrib("Name", i->getName());
			xml.addChildAttrib("Description", i->getHubDescription());
			xml.addChildAttrib("Password", i->getPassword());
			xml.addChildAttrib("Server", i->getServer());
			xml.addChildAttrib("Encoding", i->getEncoding());
			xml.addChildAttrib("Group", i->getGroup());
			i->save(xml);
		}

		xml.stepOut();

		xml.addTag("Users");
		xml.stepIn();
		for(auto& i: users) {
			xml.addTag("User");
			xml.addChildAttrib("LastSeen", i.second.getLastSeen());
			xml.addChildAttrib("GrantSlot", i.second.isSet(FavoriteUser::FLAG_GRANTSLOT));
			xml.addChildAttrib("UserDescription", i.second.getDescription());
			xml.addChildAttrib("Nick", i.second.getNick());
			xml.addChildAttrib("URL", i.second.getUrl());
			xml.addChildAttrib("CID", i.first.toBase32());
		}
		xml.stepOut();

		xml.addTag("UserCommands");
		xml.stepIn();
		for(auto& i: userCommands) {
			if(!i.isSet(UserCommand::FLAG_NOSAVE)) {
				xml.addTag("UserCommand");
				xml.addChildAttrib("Type", i.getType());
				xml.addChildAttrib("Context", i.getCtx());
				xml.addChildAttrib("Name", i.getName());
				xml.addChildAttrib("Command", i.getCommand());
				xml.addChildAttrib("To", i.getTo());
				xml.addChildAttrib("Hub", i.getHub());
			}
		}
		xml.stepOut();

		//Favorite download to dirs
		xml.addTag("FavoriteDirs");
		xml.stepIn();
		StringPairList spl = getFavoriteDirs();
		for(auto& i: spl) {
			xml.addTag("Directory", i.first);
			xml.addChildAttrib("Name", i.second);
		}
		xml.stepOut();

		xml.stepOut();

		string fname = getConfigFile();

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);

	} catch(const Exception& e) {
		dcdebug("FavoriteManager::save: %s\n", e.getError().c_str());
	}
}

void FavoriteManager::load() {

	// Add NMDC standard op commands
	static const char kickstr[] =
		"$To: %[userNI] From: %[myNI] $<%[myNI]> You are being kicked because: %[line:Reason]|<%[myNI]> %[myNI] is kicking %[userNI] because: %[line:Reason]|$Kick %[userNI]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_USER | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE,
		_("Kick user(s)"), kickstr, "", "op");
	static const char redirstr[] =
		"$OpForceMove $Who:%[userNI]$Where:%[line:Target Server]$Msg:%[line:Message]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_USER | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE,
		_("Redirect user(s)"), redirstr, "", "op");

	try {
		SimpleXML xml;
		Util::migrate(getConfigFile());
		xml.fromXML(File(getConfigFile(), File::READ, File::OPEN).read());

		if(xml.findChild("Favorites")) {
			xml.stepIn();
			load(xml);
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("FavoriteManager::load: %s\n", e.getError().c_str());
	}
}

void FavoriteManager::load(SimpleXML& aXml) {
	dontSave = true;
	bool needSave = false;

	aXml.resetCurrentChild();
	if(aXml.findChild("Hubs")) {
		aXml.stepIn();

		while(aXml.findChild("Group")) {
			string name = aXml.getChildAttrib("Name");
			if(name.empty())
				continue;
			HubSettings settings;
			settings.load(aXml);
			favHubGroups[name] = std::move(settings);
		}

		aXml.resetCurrentChild();
		while(aXml.findChild("Hub")) {
			FavoriteHubEntry* e = new FavoriteHubEntry();
			e->setName(aXml.getChildAttrib("Name"));
			e->setHubDescription(aXml.getChildAttrib("Description"));
			e->setPassword(aXml.getChildAttrib("Password"));
			e->setServer(aXml.getChildAttrib("Server"));
			e->setEncoding(aXml.getChildAttrib("Encoding"));
			e->setGroup(aXml.getChildAttrib("Group"));
			e->load(aXml);
			favoriteHubs.push_back(e);
		}

		aXml.stepOut();
	}

	aXml.resetCurrentChild();
	if(aXml.findChild("Users")) {
		aXml.stepIn();
		while(aXml.findChild("User")) {
			UserPtr u;
			const string& cid = aXml.getChildAttrib("CID");
			const string& nick = aXml.getChildAttrib("Nick");
			const string& hubUrl = aXml.getChildAttrib("URL");

			if(cid.length() != 39) {
				if(nick.empty() || hubUrl.empty())
					continue;
				u = ClientManager::getInstance()->getUser(nick, hubUrl);
			} else {
				u = ClientManager::getInstance()->getUser(CID(cid));
			}

			ClientManager::getInstance()->saveUser(u->getCID());
			auto i = users.emplace(u->getCID(), FavoriteUser(u, nick, hubUrl)).first;

			if(aXml.getBoolChildAttrib("GrantSlot"))
				i->second.setFlag(FavoriteUser::FLAG_GRANTSLOT);

			i->second.setLastSeen((uint32_t)aXml.getIntChildAttrib("LastSeen"));
			i->second.setDescription(aXml.getChildAttrib("UserDescription"));

		}
		aXml.stepOut();
	}

	aXml.resetCurrentChild();
	if(aXml.findChild("UserCommands")) {
		aXml.stepIn();
		while(aXml.findChild("UserCommand")) {
			addUserCommand(aXml.getIntChildAttrib("Type"), aXml.getIntChildAttrib("Context"), 0, aXml.getChildAttrib("Name"),
				aXml.getChildAttrib("Command"), aXml.getChildAttrib("To"), aXml.getChildAttrib("Hub"));
		}
		aXml.stepOut();
	}

	//Favorite download to dirs
	aXml.resetCurrentChild();
	if(aXml.findChild("FavoriteDirs")) {
		aXml.stepIn();
		while(aXml.findChild("Directory")) {
			string virt = aXml.getChildAttrib("Name");
			string d(aXml.getChildData());
			FavoriteManager::getInstance()->addFavoriteDir(d, virt);
		}
		aXml.stepOut();
	}

	dontSave = false;
	if(needSave)
		save();
}

void FavoriteManager::userUpdated(const OnlineUser& info) {
	Lock l(cs);
	auto i = users.find(info.getUser()->getCID());
	if(i != users.end()) {
		FavoriteUser& fu = i->second;
		fu.update(info);
		fire(FavoriteManagerListener::UserUpdated(), i->second);
		save();
	}
}

FavoriteHubEntryPtr FavoriteManager::getFavoriteHubEntry(const string& aServer) const {
	for(auto hub: favoriteHubs) {
		if(Util::stricmp(hub->getServer(), aServer) == 0) {
			return hub;
		}
	}
	return NULL;
}

void FavoriteManager::mergeHubSettings(const FavoriteHubEntry& entry, HubSettings& settings) const {
	// apply group settings first.
	const string& name = entry.getGroup();
	if(!name.empty()) {
		auto group = favHubGroups.find(name);
		if(group != favHubGroups.end())
			settings.merge(group->second);
	}

	// apply fav entry settings next.
	settings.merge(entry);
}

FavoriteHubEntryList FavoriteManager::getFavoriteHubs(const string& group) const {
	FavoriteHubEntryList ret;
	for(auto& i: favoriteHubs)
		if(i->getGroup() == group)
			ret.push_back(i);
	return ret;
}

optional<FavoriteUser> FavoriteManager::getFavoriteUser(const UserPtr &aUser) const {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i != users.end()) { return i->second; }
	return none;
}

bool FavoriteManager::hasSlot(const UserPtr& aUser) const {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i == users.end())
		return false;
	return i->second.isSet(FavoriteUser::FLAG_GRANTSLOT);
}

time_t FavoriteManager::getLastSeen(const UserPtr& aUser) const {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i == users.end())
		return 0;
	return i->second.getLastSeen();
}

void FavoriteManager::setAutoGrant(const UserPtr& aUser, bool grant) {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i == users.end())
		return;
	if(grant)
		i->second.setFlag(FavoriteUser::FLAG_GRANTSLOT);
	else
		i->second.unsetFlag(FavoriteUser::FLAG_GRANTSLOT);

	fire(FavoriteManagerListener::UserUpdated(), i->second);

	save();
}
void FavoriteManager::setUserDescription(const UserPtr& aUser, const string& description) {
	Lock l(cs);
	auto i = users.find(aUser->getCID());
	if(i == users.end())
		return;
	i->second.setDescription(description);

	fire(FavoriteManagerListener::UserUpdated(), i->second);

	save();
}

StringList FavoriteManager::getHubLists() {
	StringTokenizer<string> lists(SETTING(HUBLIST_SERVERS), ';');
	return lists.getTokens();
}

const string& FavoriteManager::blacklisted() const {
	if(publicListServer.empty())
		return Util::emptyString;

	// get the host
	string server, port, file, query, proto, fragment;
	Util::decodeUrl(publicListServer, proto, server, port, file, query, fragment);
	// only keep the last 2 words (example.com)
	size_t pos = server.rfind('.');
	if(pos == string::npos || pos == 0 || pos >= server.size() - 2)
		return Util::emptyString;
	pos = server.rfind('.', pos - 1);
	if(pos != string::npos)
		server = server.substr(pos + 1);

	auto i = blacklist.find(server);
	if(i == blacklist.end())
		return Util::emptyString;
	return i->second;
}

void FavoriteManager::addBlacklist(const string& domain, const string& reason) {
	if(domain.empty() || reason.empty())
		return;
	blacklist[domain] = reason;
}

FavoriteHubEntryPtr FavoriteManager::getFavoriteHub(const string& aServer) {
	for(auto& i: favoriteHubs) {
		if(Util::stricmp(i->getServer(), aServer) == 0) {
			return i;
		}
	}

	return nullptr;
}

void FavoriteManager::setHubList(int aHubList) {
	lastServer = aHubList;
	refresh();
}

void FavoriteManager::refresh(bool forceDownload /* = false */) {
	StringList sl = getHubLists();
	if(sl.empty()) {
		fire(FavoriteManagerListener::DownloadFailed(), Util::emptyString);
		return;
	}

	publicListServer = sl[(lastServer) % sl.size()];
	if(Util::findSubString(publicListServer, "http://") != 0 && Util::findSubString(publicListServer, "https://") != 0) {
		lastServer++;
		fire(FavoriteManagerListener::DownloadFailed(), str(F_("Invalid URL: %1%") % Util::addBrackets(publicListServer)));
		return;
	}

	if(!forceDownload) {
		string path = Util::getHubListsPath() + Util::validateFileName(publicListServer);
		if(File::getSize(path) > 0) {
			useHttp = false;
			string buf, fileDate;
			{
				Lock l(cs);
				publicListMatrix[publicListServer].clear();
			}
			listType = (Util::stricmp(path.substr(path.size() - 4), ".bz2") == 0) ? TYPE_BZIP2 : TYPE_NORMAL;
			try {
				File cached(path, File::READ, File::OPEN);
				buf = cached.read();
				char dateBuf[20];
				time_t fd = cached.getLastModified();
				if (strftime(dateBuf, 20, "%x", localtime(&fd))) {
					fileDate = string(dateBuf);
				}
			} catch(const FileException&) { }
			if(!buf.empty()) {
				if(onHttpFinished(buf)) {
					fire(FavoriteManagerListener::LoadedFromCache(), publicListServer, fileDate);
				}
				return;
			}
		}
	}

	if(!running) {
		useHttp = true;
		{
			Lock l(cs);
			publicListMatrix[publicListServer].clear();
		}
		fire(FavoriteManagerListener::DownloadStarting(), publicListServer);
		c = HttpManager::getInstance()->download(publicListServer);
		running = true;
	}
}

UserCommand::List FavoriteManager::getUserCommands(int ctx, const StringList& hubs) {
	vector<bool> isOp(hubs.size());

	for(size_t i = 0; i < hubs.size(); ++i) {
		if(ClientManager::getInstance()->isOp(ClientManager::getInstance()->getMe(), hubs[i])) {
			isOp[i] = true;
		}
	}

	Lock l(cs);
	UserCommand::List lst;
	UserCommand::List lstExternal;
	for(auto& uc: userCommands) {
		if(!(uc.getCtx() & ctx)) {
			continue;
		}

		for(size_t j = 0; j < hubs.size(); ++j) {
			const string& hub = hubs[j];
			bool hubAdc = hub.compare(0, 6, "adc://") == 0 || hub.compare(0, 7, "adcs://") == 0;
			bool commandAdc = uc.getHub().compare(0, 6, "adc://") == 0 || uc.getHub().compare(0, 7, "adcs://") == 0;
			bool addUc = false;
			if(hubAdc && commandAdc) {
				if((uc.getHub() == "adc://" || uc.getHub() == "adcs://") ||
					((uc.getHub() == "adc://op" || uc.getHub() == "adcs://op") && isOp[j]) ||
					(uc.getHub() == hub) )
				{
					addUc = true;
				}
			} else if((!hubAdc && !commandAdc) || uc.isChat()) {
				if((uc.getHub().length() == 0) ||
					(uc.getHub() == "op" && isOp[j]) ||
					(uc.getHub() == hub) )
				{
					addUc = true;
				}
			}

			if(addUc)
			{
				if(uc.isSet(UserCommand::FLAG_NOSAVE))
				{
					lstExternal.push_back(uc);
				}
				else
				{
					lst.push_back(uc);
				}
				break;
			}
		}
	}

	// If there are both normal and external user commands, add a separator.
	bool existNormalUCs = !lst.empty();
	bool existExternalUCs = !lstExternal.empty();
	if(existNormalUCs && existExternalUCs)
	{
		UserCommand ucSeparator(lastId++, UserCommand::TYPE_SEPARATOR, ctx, UserCommand::FLAG_NOSAVE, Util::emptyString, Util::emptyString, Util::emptyString, Util::emptyString);
		lst.push_back(move(ucSeparator));
	}

	lst.insert(lst.end(), lstExternal.begin(), lstExternal.end());
	return lst;
}

void FavoriteManager::on(HttpManagerListener::Added, HttpConnection* c) noexcept {
	if(c != this->c) { return; }
	fire(FavoriteManagerListener::DownloadStarting(), c->getUrl());
}

void FavoriteManager::on(HttpManagerListener::Failed, HttpConnection* c, const string& str) noexcept {
	if(c != this->c) { return; }
	this->c = nullptr;
	lastServer++;
	running = false;
	if(useHttp) {
		fire(FavoriteManagerListener::DownloadFailed(), str);
	}
}

void FavoriteManager::on(HttpManagerListener::Complete, HttpConnection* c, OutputStream* stream) noexcept {
	if(c != this->c) { return; }
	this->c = nullptr;
	bool parseSuccess = false;
	if(useHttp) {
		if(c->getMimeType() == "application/x-bzip2")
			listType = TYPE_BZIP2;
		parseSuccess = onHttpFinished(static_cast<StringOutputStream*>(stream)->getString());
	}	
	running = false;
	if(parseSuccess) {
		fire(FavoriteManagerListener::DownloadFinished(), c->getUrl());
	}
}

void FavoriteManager::on(UserUpdated, const OnlineUser& user) noexcept {
	userUpdated(user);
}
void FavoriteManager::on(UserDisconnected, const UserPtr& user) noexcept {
	bool isFav = false;
	{
		Lock l(cs);
		auto i = users.find(user->getCID());
		if(i != users.end()) {
			isFav = true;
			i->second.setLastSeen(GET_TIME());
			save();
		}
	}
	if(isFav)
		fire(FavoriteManagerListener::StatusChanged(), user);
}

void FavoriteManager::on(UserConnected, const UserPtr& user) noexcept {
	bool isFav = false;
	{
		Lock l(cs);
		auto i = users.find(user->getCID());
		if(i != users.end()) {
			isFav = true;
		}
	}
	if(isFav)
		fire(FavoriteManagerListener::StatusChanged(), user);
}

} // namespace dcpp
