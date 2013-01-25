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

#ifndef DCPLUSPLUS_DCPP_SHARE_MANAGER_H
#define DCPLUSPLUS_DCPP_SHARE_MANAGER_H

#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <boost/optional.hpp>

#include "TimerManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "HashManagerListener.h"
#include "QueueManagerListener.h"

#include "Exception.h"
#include "CriticalSection.h"
#include "StringSearch.h"
#include "Singleton.h"
#include "BloomFilter.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "Pointer.h"

#include "atomic.h"

namespace dcpp {

using std::map;
using std::set;
using std::unique_ptr;
using std::unordered_map;

using boost::optional;

STANDARD_EXCEPTION(ShareException);

class SimpleXML;
class Client;
class File;
class OutputStream;
class MemoryInputStream;

struct ShareLoader;
class ShareManager : public Singleton<ShareManager>, private SettingsManagerListener, private Thread, private TimerManagerListener,
	private HashManagerListener, private QueueManagerListener
{
public:
	/**
	 * @param aDirectory Physical directory location
	 * @param aName Virtual name
	 */
	void addDirectory(const string& realPath, const string &virtualName);
	void removeDirectory(const string& realPath);
	void renameDirectory(const string& realPath, const string& virtualName);

	string toVirtual(const TTHValue& tth) const;
	string toReal(const string& virtualFile);
	/** @return Actual file path & size. Returns 0 for file lists. */
	pair<string, int64_t> toRealWithSize(const string& virtualFile);
	StringList getRealPaths(const string& virtualPath);
	optional<TTHValue> getTTH(const string& virtualFile) const;

	void refresh(bool dirs = false, bool aUpdate = true, bool block = false) noexcept;
	void setDirty() { xmlDirty = true; }

	void search(SearchResultList& l, const string& aString, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) noexcept;
	void search(SearchResultList& l, const StringList& params, StringList::size_type maxResults) noexcept;

	StringPairList getDirectories() const noexcept;

	MemoryInputStream* generatePartialList(const string& dir, bool recurse) const;
	MemoryInputStream* getTree(const string& virtualFile) const;

	AdcCommand getFileInfo(const string& aFile);

	int64_t getShareSize() const noexcept;
	int64_t getShareSize(const string& realPath) const noexcept;

	size_t getSharedFiles() const noexcept;

	string getShareSizeString() const { return Util::toString(getShareSize()); }
	string getShareSizeString(const string& aDir) const { return Util::toString(getShareSize(aDir)); }

	void getBloom(ByteVector& v, size_t k, size_t m, size_t h) const;

	SearchManager::TypeModes getType(const string& fileName) const noexcept;

	string validateVirtual(const string& /*aVirt*/) const noexcept;
	bool hasVirtual(const string& name) const noexcept;

	void addHits(uint32_t aHits) {
		hits += aHits;
	}

	const string& getOwnListFile() {
		generateXmlList();
		return getBZXmlFile();
	}

	bool isTTHShared(const TTHValue& tth){
		Lock l(cs);
		return tthIndex.find(tth) != tthIndex.end();
	}

	GETSET(uint32_t, hits, Hits);
	GETSET(string, bzXmlFile, BZXmlFile);
private:
	struct AdcSearch;
	class Directory : public FastAlloc<Directory>, public intrusive_ptr_base<Directory>, boost::noncopyable {
	public:
		typedef boost::intrusive_ptr<Directory> Ptr;

		struct File {
			File() : size(0), parent(0) { }
			File(const string& aName, int64_t aSize, const Directory::Ptr& aParent, const optional<TTHValue>& aRoot) :
				name(aName), tth(aRoot), size(aSize), parent(aParent.get()) { }

			bool operator==(const File& rhs) const {
				return getParent() == rhs.getParent() && (Util::stricmp(getName(), rhs.getName()) == 0);
			}

			struct StringComp {
				StringComp(const string& s) : a(s) { }
				bool operator()(const File& b) const { return Util::stricmp(a, b.getName()) == 0; }
				const string& a;
			};
			struct FileLess {
				bool operator()(const File& a, const File& b) const { return (Util::stricmp(a.getName(), b.getName()) < 0); }
			};

			/** Ensure this file's name doesn't clash with the names of the parent directory's sub-
			directories or files; rename to "file (N).ext" otherwise (and set realPath to the
			actual path on disk).
			@param sourcePath Real path (on the disk) of the directory this file came from. */
			void validateName(const string& sourcePath);

			string getADCPath() const { return parent->getADCPath() + name; }
			string getFullName() const { return parent->getFullName() + name; }
			string getRealPath() const { return realPath ? realPath.get() : parent->getRealPath(name); }

			GETSET(string, name, Name);
			optional<string> realPath; // only defined if this file had to be renamed to avoid duplication.
			optional<TTHValue> tth;
			GETSET(int64_t, size, Size);
			GETSET(Directory*, parent, Parent);
		};

		int64_t size;
		unordered_map<string, Ptr, noCaseStringHash, noCaseStringEq> directories;
		set<File, File::FileLess> files;

		static Ptr create(const string& aName, const Ptr& aParent = Ptr()) { return Ptr(new Directory(aName, aParent)); }

		bool hasType(uint32_t type) const noexcept {
			return ( (type == SearchManager::TYPE_ANY) || (fileTypes & (1 << type)) );
		}
		void addType(uint32_t type) noexcept;

		const string& getRealName() const noexcept;
		template<typename SetT> void setRealName(SetT&& realName) noexcept { this->realName = std::forward<SetT>(realName); }

		string getADCPath() const noexcept;
		string getFullName() const noexcept;
		string getRealPath(const std::string& path) const;

		/** Check whether the given name would clash with this directory's sub-directories or
		files. */
		bool nameInUse(const string& name) const;

		int64_t getSize() const noexcept;

		void search(SearchResultList& aResults, StringSearch::List& aStrings, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) const noexcept;
		void search(SearchResultList& aResults, AdcSearch& aStrings, StringList::size_type maxResults) const noexcept;

		/// @param level -1 to include all levels, or the current level.
		void toXml(OutputStream& xmlFile, string& indent, string& tmp2, int8_t level) const;
		void filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const;

		auto findFile(const string& aFile) const -> decltype(files.cbegin()) { return find_if(files.begin(), files.end(), File::StringComp(aFile)); }

		void merge(const Ptr& source, const string& realPath);

		GETSET(string, name, Name);
		GETSET(Directory*, parent, Parent);

	private:
		friend void intrusive_ptr_release(intrusive_ptr_base<Directory>*);

		Directory(const string& aName, const Ptr& aParent);
		~Directory() { }

		optional<string> realName; // only defined if this directory had to be renamed to avoid duplication.

		/** Set of flags that say which SearchManager::TYPE_* a directory contains */
		uint32_t fileTypes;
	};

	friend class Directory;
	friend struct ShareLoader;

	friend class Singleton<ShareManager>;
	ShareManager();

	virtual ~ShareManager();

	struct AdcSearch {
		AdcSearch(const StringList& params);

		bool isExcluded(const string& str);
		bool hasExt(const string& name);

		StringSearch::List* include;
		StringSearch::List includeX;
		StringSearch::List exclude;
		StringList ext;
		StringList noExt;

		int64_t gt;
		int64_t lt;

		optional<TTHValue> root;

		bool isDirectory;
	};

	int64_t xmlListLen;
	optional<TTHValue> xmlRoot;
	int64_t bzXmlListLen;
	optional<TTHValue> bzXmlRoot;
	unique_ptr<File> bzXmlRef;

	bool xmlDirty;
	bool forceXmlRefresh; /// bypass the 15-minutes guard
	bool refreshDirs;
	bool update;

	int listN;

	static atomic_flag refreshing;

	uint64_t lastXmlUpdate;
	uint64_t lastFullUpdate;

	mutable CriticalSection cs;

	// List of root directory items
	unordered_map<string, Directory::Ptr, noCaseStringHash, noCaseStringEq> directories;

	/** Map real name to virtual name - multiple real names may be mapped to a single virtual one.
	The map is sorted to make sure conflicts are always resolved in the same order when merging. */
	map<string, string> shares;

	unordered_map<TTHValue, const Directory::File*> tthIndex;

	BloomFilter<5> bloom;

	const Directory::File& findFile(const string& virtualFile) const;

	Directory::Ptr buildTree(const string& realPath, optional<const string&> dirName = nullptr, const Directory::Ptr& parent = nullptr);
	bool checkHidden(const string& realPath) const;

	void rebuildIndices();

	void updateIndices(Directory& aDirectory);
	void updateIndices(Directory& dir, const decltype(std::declval<Directory>().files.begin())& i);

	void merge(const Directory::Ptr& directory, const string& realPath);

	void generateXmlList();
	pair<Directory::Ptr, string> splitVirtual(const string& virtualPath) const;
	string findRealRoot(const string& virtualRoot, const string& virtualLeaf) const;

	/** Get the directory pointer corresponding to a given real path (on disk). Note that only
	directories are considered here but not the file's base name. */
	Directory::Ptr getDirectory(const string& realPath) noexcept;
	/** Get the file corresponding to a given real path (on disk). */
	optional<const ShareManager::Directory::File&> getFile(const string& realPath, Directory::Ptr d = nullptr) noexcept;

	virtual int run();

	// QueueManagerListener
	virtual void on(QueueManagerListener::FileMoved, const string& realPath) noexcept;

	// HashManagerListener
	virtual void on(HashManagerListener::TTHDone, const string& realPath, const TTHValue& root) noexcept;

	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept {
		save(xml);
	}
	virtual void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
		load(xml);
	}

	// TimerManagerListener
	virtual void on(TimerManagerListener::Minute, uint64_t tick) noexcept;
	void load(SimpleXML& aXml);
	void save(SimpleXML& aXml);

};

} // namespace dcpp

#endif // !defined(SHARE_MANAGER_H)
