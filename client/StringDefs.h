// @Prolog: #include "stdinc.h"
// @Prolog: #include "DCPlusPlus.h"
// @Prolog: #include "ResourceManager.h"
// @Strings: string ResourceManager::strings[]
// @Names: string ResourceManager::names[]

enum Strings { // @DontAdd
	ADD_TO_FAVORITES, // "Add To Favorites"
	ALL_DOWNLOAD_SLOTS_TAKEN, // "All download slots taken"
	ALL_USERS_OFFLINE, // "All %d users offline"
	ANY, // "Any"
	AT_LEAST, // "At least"
	AT_MOST, // "At most"
	AUDIO, // "Audio"
	AUTO_CONNECT, // "Auto connect / Name"
	AWAY_MODE_OFF, // "Away mode off"
	AWAY_MODE_ON, // "Away mode on: "
	BOTH_USERS_OFFLINE, // "Both users offline"
	BROWSE, // "Browse..."
	CANT_CONNECT_IN_PASSIVE_MODE, // "Can't connect to passive user while in passive mode"
	CLOSE_CONNECTION, // "Close connection"
	CLOSE, // "Close"
	COMPRESSED, // "Compressed"
	CONNECT, // "Connect"
	CONNECTED, // "Connected"
	CONNECTING, // "Connecting..."
	CONNECTING_FORCED, // "Connecting (forced)..."
	CONNECTING_TO, // "Connecting to "
	CONNECTION, // "Connection"
	CONNECTION_TIMEOUT, // "Connection timeout"
	COUNT, // "Count"
	DESCRIPTION, // "Description"
	DISCONNECTED, // "Disconnected"
	DOCUMENT, // "Document"
	DONE, // "Done"
	DOWNLOAD, // "Download"
	DOWNLOAD_FAILED, // "Download failed: "
	DOWNLOAD_FINISHED_IDLE, // "Download finished, idle..."
	DOWNLOAD_STARTING, // "Download starting..."
	DOWNLOAD_TO, // "Download to..."
	DOWNLOAD_QUEUE, // "Download queue"
	DOWNLOADED_LEFT, // "Downloaded %s (%.01f%%), %s/s, %s left"
	DOWNLOADED_FROM, // " downloaded from "
	DOWNLOADING_HUB_LIST, // "Downloading public hub list..."
	EMAIL, // "E-Mail"
	ENTER_NICK, // "Please enter a nickname in the settings dialog!"
	ENTER_PASSWORD, // "Please enter a password"
	ENTER_REASON, // "Please enter a reason"
	ENTER_SERVER, // "Please enter a destination server"
	ERROR_OPENING_FILE, // "Error opening file"
	EXECUTABLE, // "Executable"
	FAVORITE_HUBS, // "Favorite Hubs"
	FAVORITE_USERS, // "Favorite Users"
	FILE, // "File"
	FILES, // "Files"
	FILE_LIST_REFRESHED, // "File list refreshed"
	FILE_TYPE, // "File type"
	FILENAME, // "Filename"
	FILTER, // "Filter"
	FORCE_ATTEMPT, // "Force attempt"
	GET_FILE_LIST, // "Get file list"
	GRANT_EXTRA_SLOT, // "Grant extra slot"
	HIGH, // "High"
	HUB, // "Hub"
	HUBS, // "Hubs"
	HUB_ADDRESS, // "Address"
	HUB_LIST_DOWNLOADED, // "Hub list downloaded..."
	HUB_NAME, // "Name"
	HUB_PASSWORD, // "Hub password"
	IGNORED_MESSAGE, // "Ignored message: "
	IMPORT_QUEUE, // "Import queue from NMDC"
	INVALID_NUMBER_OF_SLOTS, // "Invalid number of slots"
	JOINS, // "Joins: "
	JOIN_SHOWING_OFF, // "Join/part showing off"
	JOIN_SHOWING_ON, // "Join/part showing on"
	KICK_USER, // "Kick user(s)"
	LAST_HUB, // "Hub (last seen on if offline)"
	LOW, // "Low"
	MANUAL_ADDRESS, // "Manual connect address"
	MENU_FILE, // "&File"
	MENU_FILE_DOWNLOAD_QUEUE, // "&Download Queue\tCtrl+D"
	MENU_FILE_EXIT, // "&Exit"
	MENU_FILE_FAVORITE_HUBS, // "&Favorite Hubs\tCtrl+F"
	MENU_FILE_FAVORITE_USERS, // "Favorite &Users\tCtrl+U"
	MENU_FILE_FOLLOW_REDIRECT, // "Follow last redirec&t\tCtrl+T"
	MENU_FILE_NOTEPAD, // "&Notepad\tCtrl+N"
	MENU_FILE_PUBLIC_HUBS, // "&Public Hubs\tCtrl+P"
	MENU_FILE_RECONNECT, // "&Reconnect\tCtrl+R"
	MENU_FILE_SEARCH, // "&Search\tCtrl+S"
	MENU_FILE_SEARCH_SPY, // "Search spy"
	MENU_FILE_SETTINGS, // "Settings..."
	MENU_HELP, // "&Help"
	MENU_HELP_ABOUT, // "About DC++..."
	MENU_HELP_DISCUSS, // "DC++ discussion forum"
	MENU_HELP_DOWNLOADS, // "Downloads and translations"
	MENU_HELP_FAQ, // "Frequently asked questions"
	MENU_HELP_HELP_FORUM, // "Help forum"
	MENU_HELP_HOMEPAGE, // "DC++ Homepage"
	MENU_HELP_REQUEST_FEATURE, // "Request a feature"
	MENU_HELP_REPORT_BUG, // "Report a bug"
	MENU_VIEW, // "&View"
	MENU_VIEW_STATUS_BAR, // "&Status bar"
	MENU_VIEW_TOOLBAR, // "&Toolbar"
	MENU_WINDOW, // "&Window"
	MENU_WINDOW_ARRANGE, // "Arrange icons"
	MENU_WINDOW_CASCADE, // "Cascade"
	MENU_WINDOW_TILE, // "Tile"
	NEW, // "New..."
	NICK, // "Nick"
	NICK_TAKEN, // "Your nick was already taken, please change to something else!"
	NOT_CONNECTED, // "Not connected"
	NO_SLOTS_AVAILABLE, // "No slots available"
	NO_USERS_TO_DOWNLOAD, // "No users to download from"
	NORMAL, // "Normal"
	OFFLINE, // "Offline"
	ONLINE, // "Online"
	ONLY_FREE_SLOTS, // "Only users with free slots"
	PASSWORD, // "Password"
	PARTS, // "Parts: "
	PATH, // "Path"
	PAUSED, // "Paused"
	PICTURE, // "Picture"
	PORT_IS_BUSY, // "Port %d is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart DC++"
	PREPARING_FILE_LIST, // "Preparing file list..."
	PRIORITY, // "Priority"
	PRIVATE_MESSAGE_FROM, // "Private message from "
	PROPERTIES, // "Properties"
	PUBLIC_HUBS, // "Public Hubs"
	REALLY_EXIT, // "Really exit?"
	REDIRECT, // "Redirect"
	REDIRECT_USER, // "Redirect user(s)"
	REFRESH, // "Refresh"
	REFRESH_USER_LIST, // "Refresh user list"
	REMOVE, // "Remove"
	REMOVE_SOURCE, // "Remove source"
	ROLLBACK_INCONSISTENCY, // "Rollback inconsistency, existing file does not match the one being downloaded"
	RUNNING, // "Running..."
	SEARCH, // "Search"
	SEARCH_FOR, // "Search for"
	SEARCH_FOR_ALTERNATES, // "Search for alternates"
	SEARCH_OPTIONS, // "Search options"
	SEARCH_SPY, // "Search Spy"
	SEARCH_STRING, // "Search string"
	SEARCHING_FOR, // "Searching for "
	SEND_PRIVATE_MESSAGE, // "Send private message"
	SERVER, // "Server"
	SET_PRIORITY, // "Set priority"
	SHARED, // "Shared"
	SIZE, // "Size"
	SLOTS, // "Slots"
	SLOTS_SET, // "Slots set"
	SPECIFY_SERVER, // "Specify a server to connect to"
	SPECIFY_SEARCH_STRING, // "Specify a search string"
	STATUS, // "Status"
	TIMESTAMPS_DISABLED, // "Timestamps disabled"
	TIMESTAMPS_ENABLED, // "Timestamps enabled"
	TYPE, // "Type"
	UNKNOWN, // "Unknown"
	UPLOAD_FINISHED_IDLE, // "Upload finished, idle..."
	UPLOAD_STARTING, // "Upload starting..."
	UPLOADED_LEFT, // "Uploaded %s (%.01f%%), %s/s, %s left"
	UPLOADED_TO, // " uploaded to "
	USER, // "User"
	USER_OFFLINE, // "User offline"
	USER_WENT_OFFLINE, // "User went offline"
	USERS, // "Users"
	VIDEO, // "Video"
	WAITING, // "Waiting..."
	WAITING_USER_ONLINE, // "Waiting (User online)"
	WAITING_USERS_ONLINE, // "Waiting (%d of %d users online)"
	WAITING_TO_RETRY, // "Waiting to retry..."
	YOU_ARE_BEING_REDIRECTED, // "You are being redirected to "
	LAST // @DontAdd
};
