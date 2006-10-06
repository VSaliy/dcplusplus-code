// @Prolog: #include "stdinc.h"
// @Prolog: #include "DCPlusPlus.h"
// @Prolog: #include "ResourceManager.h"
// @Strings: string ResourceManager::strings[]
// @Names: string ResourceManager::names[]

enum Strings { // @DontAdd
	ACTIVE, // "Active"
	ACTIVE_SEARCH_STRING, // "Enabled / Search String"
	ADD, // "&Add"
	ADD_TO_FAVORITES, // "Add To Favorites"
	ADDED, // "Added"
	ADL_SEARCH, // "Automatic Directory Listing Search"
	ADLS_DESTINATION, // "Destination Directory"
	ADLS_DISCARD, // "Discard"
	ADLS_DOWNLOAD, // "Download Matches"
	ADLS_ENABLED, // "Enabled"
	ADLS_FULL_PATH, // "Full Path"
	ADLS_PROPERTIES, // "ADLSearch Properties"
	ADLS_SEARCH_STRING, // "Search String"
	ADLS_SIZE_MAX, // "Max FileSize"
	ADLS_SIZE_MIN, // "Min FileSize"
	ADLS_TYPE, // "Search Type"
	ADLS_UNITS, // "Size Type"
	ALL_DOWNLOAD_SLOTS_TAKEN, // "All download slots taken"
	ALL_USERS_OFFLINE, // "All %d users offline"
	ALL_3_USERS_OFFLINE, // "All 3 users offline"
	ALL_4_USERS_OFFLINE, // "All 4 users offline"
	ALL, // "All"
	ANY, // "Any"
	AT_LEAST, // "At least"
	AT_MOST, // "At most"
	AUDIO, // "Audio"
	AUTO_CONNECT, // "Auto connect / Name"
	AUTO_GRANT, // "Auto grant slot / Nick"
	AVERAGE, // "Average/s: "
	AWAY, // "AWAY"
	AWAY_MODE_OFF, // "Away mode off"
	AWAY_MODE_ON, // "Away mode on: "
	B, // "B"
	BAN_USER, // "Ban user(s)"
	BOTH_USERS_OFFLINE, // "Both users offline"
	BPS, // "B/s"
	BROWSE, // "Browse..."
	BROWSE_ACCEL, // "&Browse..."
	BROWSE_FILE_LIST, // "Browse file list"
	CERTIFICATE_NOT_TRUSTED, // "Certificate not trusted, unable to connect"
	CERTIFICATE_GENERATION_FAILED, // "TLS disabled, failed to generate certificate: "
	CHOOSE_FOLDER, // "Choose folder"
	CID, // "CID"
	CLOSE, // "Close"
	CLOSE_CONNECTION, // "Close connection"
	CLOSING_CONNECTION, // "Closing connection..."
	COMPRESSED, // "Compressed"
	COMPRESSION_ERROR, // "Error during compression"
	COMMAND_TOO_LONG, // "Maximum command length exceeded"
	CONFIGURE, // "&Configure"
	CONNECT, // "&Connect"
	CONNECT_FAVUSER_HUB, // "Connect to hub"
	CONNECTED, // "Connected"
	CONNECTING, // "Connecting..."
	CONNECTING_FORCED, // "Connecting (forced)..."
	CONNECTING_TO, // "Connecting to "
	CONNECTION, // "Connection"
	CONNECTION_CLOSED, // "Connection closed"
	CONNECTION_TIMEOUT, // "Connection timeout"
	CONFIGURED_HUB_LISTS, // "Configured Public Hub Lists"
	COPY_FILENAME, // "Copy Filename"
	COPY_HUB, // "Copy address to clipboard"
	COPY_MAGNET, // "Copy magnet link to clipboard"
	COPY_NICK, // "Copy nick to clipboard"
	COULD_NOT_OPEN_TARGET_FILE, // "Could not open target file: "
	COUNT, // "Count"
	COUNTRY, // "Country"
	CRC_CHECKED, // "CRC Checked"
	DECOMPRESSION_ERROR, // "Error during decompression"
	DESCRIPTION, // "Description"
	DESTINATION, // "Destination"
	DIRECTORY, // "Directory"
	DIRECTORY_ALREADY_SHARED, // "Directory already shared"
	DIRECTORY_ADD_ERROR, // "Directory or directory name already exists"
	DISC_FULL, // "Disk full(?)"
	DISCONNECT_USER, // "Disconnect user(s)"
	DISCONNECTED, // "Disconnected"
	DISCONNECTED_USER, // "Disconnected user leaving the hub: "
	DOCUMENT, // "Document"
	DONE, // "Done"
	DONT_REMOVE_SLASH_PASSWORD, // "Don't remove /password before your password"
	DONT_SHARE_TEMP_DIRECTORY, // "The temporary download directory cannot be shared"
	DOWNLOAD, // "Download"
	DOWNLOAD_FAILED, // "Download failed: "
	DOWNLOAD_FINISHED_IDLE, // "Download finished, idle..."
	DOWNLOAD_QUEUE, // "Download Queue"
	DOWNLOAD_STARTING, // "Download starting..."
	DOWNLOAD_TO, // "Download to..."
	DOWNLOAD_WHOLE_DIR, // "Download whole directory"
	DOWNLOAD_WHOLE_DIR_TO, // "Download whole directory to..."
	DOWNLOADED, // "Downloaded"
	DOWNLOADED_BYTES, // "Downloaded %s (%.01f%%) in %s"
	DOWNLOADED_FROM, // " downloaded from "
	DOWNLOADING, // "Downloading..."
	DOWNLOADING_HUB_LIST, // "Downloading public hub list..."
	DOWNLOADING_LIST, // "Downloading list..."
	DOWNLOADS, // "Downloads"
	DUPLICATE_FILE_NOT_SHARED, // "Duplicate file will not be shared: "
	DUPLICATE_MATCH, // "Dupe matched against: "
	DUPLICATE_SOURCE, // "Duplicate source"
	EDIT, // "Edit"
	EDIT_ACCEL, // "&Edit"
	EMAIL, // "E-Mail"
	ENTER_NICK, // "Please enter a nickname in the settings dialog!"
	ENTER_PASSWORD, // "Please enter a password"
	ENTER_REASON, // "Please enter a reason"
	ENTER_SEARCH_STRING, // "Enter search string"
	ENTER_SERVER, // "Please enter a destination server"
	ERRORS, // "Errors"
	ERROR_CREATING_HASH_DATA_FILE, // "Error creating hash data file: "
	ERROR_CREATING_REGISTRY_KEY_ADC, // "Error creating adc registry key"
	ERROR_CREATING_REGISTRY_KEY_DCHUB, // "Error creating dchub registry key"
	ERROR_CREATING_REGISTRY_KEY_MAGNET, // "Error creating magnet registry key"
	ERROR_HASHING, // "Error hashing "
	ERROR_SAVING_HASH, // "Error saving hash data: "
	EXACT_SIZE, // "Exact size"
	EXECUTABLE, // "Executable"
	FAILED_TO_LOAD_CERTIFICATE, // "Failed to load certificate file"
	FAILED_TO_LOAD_PRIVATE_KEY, // "Failed to load private key"
	FAV_JOIN_SHOWING_OFF, // "Join/part of favorite users showing off"
	FAV_JOIN_SHOWING_ON, // "Join/part of favorite users showing on"
	FAVORITE_DIR_NAME, // "Favorite name"
	FAVORITE_DIR_NAME_LONG, // "Under what name you see the directory"
	FAVORITE_HUB_ADDED, // "Favorite hub added"
	FAVORITE_HUB_ALREADY_EXISTS, // "Hub already exists as a favorite"
	FAVORITE_HUB_DOES_NOT_EXIST, // "This hub is not a favorite hub"
	FAVORITE_HUB_IDENTITY, // "Identification (leave blank for defaults)"
	FAVORITE_HUB_PROPERTIES, // "Favorite Hub Properties"
	FAVORITE_HUB_REMOVED, // "Favorite hub removed"
	FAVORITE_HUBS, // "Favorite Hubs"
	FAVORITE_USER_ADDED, // "Favorite user added"
	FAVORITE_USERS, // "Favorite Users"
	FILE, // "File"
	FILES, // "Files"
	FILE_HAS_NO_TTH, // "This file has no TTH"
	FILE_IS_ALREADY_QUEUED, // "This file is already queued"
	FILE_LIST_DIFF, // "Subtract list"
	FILE_LIST_REFRESH_FAILED, // "File list refresh failed: "
	FILE_LIST_REFRESH_FINISHED, // "File list refresh finished"
	FILE_LIST_REFRESH_INITIATED, // "File list refresh initiated"
	FILE_LIST_REFRRESH_IN_PROGRESS, // "File list refresh in progress, please wait for it to finish before trying to refresh again"
	FILE_NOT_AVAILABLE, // "File not available"
	FILE_TYPE, // "File type"
	FILE_WITH_DIFFERENT_SIZE, // "A file with a different size already exists in the queue"
	FILE_WITH_DIFFERENT_TTH, // "A file with different tth root already exists in the queue"
	FILENAME, // "Filename"
	FILES_LEFT, // "files left"
	FILES_PER_HOUR, // "files/h"
	FILTER, // "F&ilter"
	FILTERED, // "Filtered"
	FIND, // "Find"
	FINISHED_DOWNLOADS, // "Finished Downloads"
	FINISHED_UPLOADS, // "Finished Uploads"
	FORBIDDEN_DOLLAR_FILE, // "File with '$' cannot be downloaded and will not be shared: "
	FORCE_ATTEMPT, // "Force attempt"
	GiB, // "GiB"
	GET_FILE_LIST, // "Get file list"
	GO_TO_DIRECTORY, // "Go to directory"
	GRANT_EXTRA_SLOT, // "Grant extra slot"
	HASH_DATABASE, // "Hash database"
	HASH_PROGRESS, // "Creating file index..."
	HASH_PROGRESS_BACKGROUND, // "Run in background"
	HASH_PROGRESS_STATS, // "Statistics"
	HASH_PROGRESS_TEXT, // "Please wait while DC++ indexes your files (they won't be shared until they've been indexed)..."
	HASH_READ_FAILED, // "Unable to read hash data file"
	HASH_REBUILT, // "Hash database rebuilt"
	HASHING_FAILED, // "Hashing failed: "
	HASHING_FINISHED, // "Finished hashing: "
	HIGH, // "High"
	HIGHEST, // "Highest"
	HIT_RATIO, // "Hit Ratio: "
	HITS, // "Hits: "
	HUB, // "Hub"
	HUBS, // "Hubs"
	HUB_ADDRESS, // "Address"
	HUB_LIST_DOWNLOADED, // "Hub list downloaded..."
	HUB_LIST_EDIT, // "Edit the hublist"
	HUB_NAME, // "Name"
	HUB_LIST, // "Hublist"
	HUB_PASSWORD, // "Hub password"
	HUB_USERS, // "Users"
	IGNORE_TTH_SEARCHES, // "Ignore TTH searches"
	IGNORED_MESSAGE, // "Ignored message: "
	INCOMPLETE_FAV_HUB, // "Hub address cannot be empty."
	INVALID_LISTNAME, // "Invalid file list name"
	INVALID_NUMBER_OF_SLOTS, // "Invalid number of slots"
	INVALID_TARGET_FILE, // "Invalid target file (missing directory, check default download directory setting)"
	INVALID_TREE, // "Full tree does not match TTH root"
	IP, // "IP: "
	IP_BARE, // "IP"
	ITEMS, // "Items"
	JOIN_SHOWING_OFF, // "Join/part showing off"
	JOIN_SHOWING_ON, // "Join/part showing on"
	JOINS, // "Joins: "
	KiB, // "KiB"
	KiBPS, // "KiB/s"
	KICK_USER, // "Kick user(s)"
	LARGER_TARGET_FILE_EXISTS, // "A file of equal or larger size already exists at the target location"
	LAST_CHANGE, // "Last change: "
	LAST_HUB, // "Hub (last seen on if offline)"
	LAST_SEEN, // "Time last seen"
	LEFT, // "left"
	LISTENER_FAILED, // "Listening socket failed (you need to restart DC++): "
	LOADING, // "Loading DC++, please wait..."
	LOOKUP_AT_BITZI, // "Lookup TTH at Bitzi.com"
	LOW, // "Low"
	LOWEST, // "Lowest"
	MAGNET_DLG_FILE, // "Filename:"
	MAGNET_DLG_HASH, // "File Hash:"
	MAGNET_DLG_NOTHING, // "Do nothing"
	MAGNET_DLG_QUEUE, // "Add this file to your download queue"
	MAGNET_DLG_REMEMBER, // "Do the same action next time without asking"
	MAGNET_DLG_SEARCH, // "Start a search for this file"
	MAGNET_DLG_TEXT_BAD, // "A MAGNET link was given to DC++, but it didn't contain a valid file hash for use on the Direct Connect network.  No action will be taken."
	MAGNET_DLG_TEXT_GOOD, // "DC++ has detected a MAGNET link with a file hash that can be searched for on the Direct Connect network.  What would you like to do?"
	MAGNET_DLG_TITLE, // "MAGNET Link detected"
	MAGNET_HANDLER_DESC, // "Download files from the Direct Connect network"
	MAGNET_HANDLER_ROOT, // "DC++"
	MAGNET_SHELL_DESC, // "URL:MAGNET URI"
	MATCH_QUEUE, // "Match queue"
	MATCHED_FILES, // "Matched %d file(s)"
	MAX_HUBS, // "Max Hubs"
	MAX_SIZE, // "Max Size"
	MAX_USERS, // "Max Users"
	MIN_SIZE, // "Min Size"
	MiB, // "MiB"
	MiBPS, // "MiB/s"
	MiBITSPS, // "MiBits/s"
	MENU_ABOUT, // "About DC++..."
	MENU_ADL_SEARCH, // "ADL Search"
	MENU_ARRANGE, // "Arrange icons"
	MENU_CASCADE, // "Cascade"
	MENU_CHANGELOG, // "Change Log"
	MENU_CLOSE_ALL_DIR_LIST, // "Close all file list windows"
	MENU_CLOSE_ALL_OFFLINE_PM, // "Close all offline PM windows"
	MENU_CLOSE_ALL_PM, // "Close all PM windows"
	MENU_CLOSE_ALL_SEARCHFRAME, // "Close all search windows"
	MENU_CLOSE_DISCONNECTED, // "Close disconnected"
	MENU_CONTENTS, // "Help &Contents\tF1"
	MENU_DISCUSS, // "DC++ discussion forum"
	MENU_DONATE, // "Donate (paypal)"
	MENU_DOWNLOAD_QUEUE, // "&Download Queue\tCtrl+D"
	MENU_EXIT, // "E&xit"
	MENU_FAQ, // "Frequently asked questions"
	MENU_FAVORITE_HUBS, // "&Favorite Hubs\tCtrl+F"
	MENU_FAVORITE_USERS, // "Favorite &Users\tCtrl+U"
	MENU_FILE, // "&File"
	MENU_FOLLOW_REDIRECT, // "Follow last redirec&t\tCtrl+T"
	MENU_HASH_PROGRESS, // "Indexing progress"
	MENU_HELP, // "&Help"
	MENU_HELP_DOWNLOADS, // "Downloads"
	MENU_HELP_GEOIPFILE, // "GeoIP database update"
	MENU_HELP_FORUM, // "Help forum"
	MENU_HELP_TRANSLATIONS, // "Translations"
	MENU_HOMEPAGE, // "DC++ Homepage"
	MENU_HORIZONTAL_TILE, // "Horizontal Tile"
	MENU_MINIMIZE_ALL, // "Minimize &All"
	MENU_RESTORE_ALL, // "Restore All"
	MENU_NETWORK_STATISTICS, // "Network Statistics"
	MENU_NOTEPAD, // "&Notepad\tCtrl+N"
	MENU_OPEN_DOWNLOADS_DIR, // "Open downloads directory"
	MENU_OPEN_FILE_LIST, // "Open file list...\tCtrl+L"
	MENU_OPEN_MATCH_ALL, // "Match downloaded lists"
	MENU_OPEN_OWN_LIST, // "Open own list"
	MENU_PUBLIC_HUBS, // "&Public Hubs\tCtrl+P"
	MENU_QUICK_CONNECT, // "&Quick Connect ...\tCtrl+Q"
	MENU_RECONNECT, // "&Reconnect\tCtrl+R"
	MENU_REFRESH_FILE_LIST, // "Refresh file list\tCtrl+E"
	MENU_REPORT_BUG, // "Report a bug"
	MENU_REQUEST_FEATURE, // "Request a feature"
	MENU_SEARCH, // "&Search\tCtrl+S"
	MENU_SEARCH_SPY, // "Search Spy"
	MENU_SETTINGS, // "Settings..."
	MENU_SHOW, // "Show"
	MENU_STATUS_BAR, // "&Status bar\tCtrl+2"
	MENU_SYSTEM_LOG, // "System Log"
	MENU_TOOLBAR, // "&Toolbar\tCtrl+1"
	MENU_TRANSFER_VIEW, // "T&ransfers\tCtrl+3"
	MENU_VERTICAL_TILE, // "Vertical Tile"
	MENU_VIEW, // "&View"
	MENU_WINDOW, // "&Window"
	MIN_SHARE, // "Min Share"
	MIN_SLOTS, // "Min Slots"
	MOVE, // "Move/Rename"
	MOVE_DOWN, // "Move &Down"
	MOVE_UP, // "Move &Up"
	NETWORK_STATISTICS, // "Network Statistics"
	NEW, // "&New..."
	NEXT, // "Next"
	NICK, // "Nick"
	NICK_TAKEN, // "Your nick was already taken, please change to something else!"
	NICK_UNKNOWN, // " (Nick unknown)"
	NO_CERTIFICATE_FILE_SET, // "TLS disabled, no certificate file set"
	NO_CRC32_MATCH, // " not shared; calculated CRC32 does not match the one found in SFV file."
	NO_DIRECTORY_SPECIFIED, // "No directory specified"
	NO_DOWNLOADS_FROM_SELF, // "You're trying to download from yourself!"
	NO_DOWNLOADS_FROM_PASSIVE, // "Can't download from passive users when you're passive"
	NO_ERRORS, // "No errors"
	NO_MATCHES, // "No matches"
	NO_SLOTS_AVAILABLE, // "No slots available"
	NO_STR, // "No"
	NO_USERS, // "No users"
	NO_USERS_TO_DOWNLOAD_FROM, // "No users to download from"
	NORMAL, // "Normal"
	NOT_LISTENING, // "Not listening for connections - please restart DC++"
	NOTEPAD, // "Notepad"
	OFFLINE, // "Offline"
	ONLINE, // "Online"
	ONLY_FREE_SLOTS, // "Only users with free slots"
	ONLY_TLS_ALLOWED, // "Only TLS connections allowed"
	ONLY_TTH, // "Only results with TTH root"
	ONLY_WHERE_OP, // "Only where I'm op"
	OPEN, // "Open"
	OPEN_DOWNLOAD_PAGE, // "Open download page?"
	OPEN_FOLDER, // "Open folder"
	OPERATING_SYSTEM_NOT_COMPATIBLE, // "Operating system does not match minimum requirements for feature"
	OUT_OF_BUFFER_SPACE, // "Out of buffer space"
	PARTS, // "Parts: "
	PASSIVE_USER, // "Passive user"
	PASSWORD, // "Password"
	PATH, // "Path"
	PAUSED, // "Paused"
	PIB, // "PiB"
	PICTURE, // "Picture"
	PORT, // "Port: "
	PREPARING_FILE_LIST, // "Preparing file list..."
	PRESS_FOLLOW, // "Press the follow redirect button to connect to "
	PRIORITY, // "Priority"
	PRIVATE_MESSAGE, // "Private message"
	PRIVATE_MESSAGE_FROM, // "Private message from "
	PROPERTIES, // "&Properties"
	PUBLIC_HUBS, // "Public Hubs"
	PURGE, // "Purge"
	QUICK_CONNECT, // "Quick Connect"
	RATING, // "Rating"
	RATIO, // "Ratio"
	READD_SOURCE, // "Re-add source"
	REALLY_EXIT, // "Really exit?"
	REALLY_REMOVE, // "Really remove?"
	REDIRECT, // "Redirect"
	REDIRECT_ALREADY_CONNECTED, // "Redirect request received to a hub that's already connected"
	REDIRECT_USER, // "Redirect user(s)"
	REFRESH, // "&Refresh"
	REFRESH_USER_LIST, // "Refresh user list"
	RELIABILITY, // "Reliability"
	REMOVE, // "&Remove"
	REMOVE_ALL, // "Remove all"
	REMOVE_ALL_SUBDIRECTORIES, // "Remove all subdirectories before adding this one"
	REMOVE_FROM_ALL, // "Remove user from queue"
	REMOVE_SOURCE, // "Remove source"
	RENAMED_TO, // " renamed to "
	ROLLBACK_INCONSISTENCY, // "Rollback inconsistency, existing file does not match the one being downloaded"
	RUNNING, // "Running..."
	S, // "s"
	SEARCH, // "Search"
	SEARCH_FOR, // "Search for"
	SEARCH_FOR_ALTERNATES, // "Search for alternates"
	SEARCH_FOR_FILE, // "Search for file"
	SEARCH_OPTIONS, // "Search options"
	SEARCH_SPAM_FROM, // "Search spam detected from "
	SEARCH_SPY, // "Search Spy"
	SEARCH_STRING, // "Search String"
	SEARCHING_FOR, // "Searching for "
	SEARCHING_READY, // "Ready to search..."
	SEARCHING_WAIT, // "Searching too soon, next search in %i seconds"
	SEEK_BEYOND_END, // "Request to seek beyond the end of data"
	SEND_PRIVATE_MESSAGE, // "Send private message"
	SEPARATOR, // "Separator"
	SERVER, // "Server"
	SET_PRIORITY, // "Set priority"
	SETTINGS, // "Settings"
	SETTINGS_ADD_FINISHED_INSTANTLY, // "Add finished files to share instantly (if shared)"
	SETTINGS_ADD_FOLDER, // "&Add folder"
	SETTINGS_ADLS_BREAK_ON_FIRST, // "Break on first ADLSearch match"
	SETTINGS_ADVANCED, // "Advanced"
	SETTINGS_ADVANCED3, // "Advanced\\Experts only"
	SETTINGS_ADVANCED_RESUME, // "Advanced resume using TTH"
	SETTINGS_ADVANCED_SETTINGS, // "Advanced settings"
	SETTINGS_ALLOW_UNTRUSTED_CLIENTS, // "Allow TLS connections to clients without trusted certificate"
	SETTINGS_ALLOW_UNTRUSTED_HUBS, // "Allow TLS connections to hubs without trusted certificate"
	SETTINGS_ANTI_FRAG, // "Use antifragmentation method for downloads"
	SETTINGS_APPEARANCE, // "Appearance"
	SETTINGS_APPEARANCE2, // "Appearance\\Colors and sounds"
	SETTINGS_AUTODROP_AUTODROPSETTINGS, // "Autodrop settings"
	SETTINGS_AUTODROP_SPEED, // "Drop sources below"
	SETTINGS_AUTODROP_INTERVAL, // "Check every"
	SETTINGS_AUTODROP_ELAPSED, // "Min elapsed"
	SETTINGS_AUTODROP_INACTIVITY, // "Max inactivity"
	SETTINGS_AUTODROP_MINSOURCES, // "Min sources online"
	SETTINGS_AUTODROP_FILESIZE, // "Min filesize"
	SETTINGS_AUTODROP_ALL, // "Autodrop slow sources for all queue items (except filelists)"
	SETTINGS_AUTODROP_FILELISTS, // "Remove slow filelists"
	SETTINGS_AUTODROP_DISCONNECT, // "Don't remove the source when autodropping, only disconnect"
	SETTINGS_AUTO_AWAY, // "Auto-away on minimize (and back on restore)"
	SETTINGS_AUTO_FOLLOW, // "Automatically follow redirects"
	SETTINGS_AUTO_KICK, // "Automatically disconnect users who leave the hub"
	SETTINGS_AUTO_KICK_NO_FAVS, // "Don't automatically disconnect favorite users who leave the hub"
	SETTINGS_AUTO_SEARCH, // "Automatically search for alternative download locations"
	SETTINGS_AUTO_SEARCH_AUTO_MATCH, // "Automatically match queue for auto search hits"
	SETTINGS_AUTO_SEARCH_LIMIT, // "Auto-search limit"
	SETTINGS_AUTO_OPEN, // "Auto-open at startup"
	SETTINGS_AUTO_REFRESH_TIME, // "Auto refresh time"
	SETTINGS_BIND_ADDRESS, // "Bind address"
	SETTINGS_BOLD_OPTIONS, // "Tab bolding on content change"
	SETTINGS_CERTIFICATES, // "Advanced\\Security Certificates"
	SETTINGS_CHANGE, // "&Change"
	SETTINGS_CLEAR_SEARCH, // "Clear search box after each search"
	SETTINGS_COLORS, // "Colors"
	SETTINGS_COMMAND, // "Command"
	SETTINGS_COMPRESS_TRANSFERS, // "Enable safe and compressed transfers"
	SETTINGS_CONFIGURE_HUB_LISTS, // "Configure Public Hub Lists"
	SETTINGS_CONFIRM_DIALOG_OPTIONS, // "Confirm dialog options"
	SETTINGS_CONFIRM_EXIT, // "Confirm application exit"
	SETTINGS_CONFIRM_HUB_REMOVAL, // "Confirm favorite hub removal"
	SETTINGS_CONFIRM_ITEM_REMOVAL, // "Confirm item removal in download queue"
	SETTINGS_CONNECTION_TYPE, // "Connection Type"
	SETTINGS_DEFAULT_AWAY_MSG, // "Default away message"
	SETTINGS_DIRECT, // "Direct connection"
	SETTINGS_DIRECTORIES, // "Directories"
	SETTINGS_DONT_DL_ALREADY_QUEUED, // "Don't download files already in the queue"
	SETTINGS_DONT_DL_ALREADY_SHARED, // "Don't download files already in share"
	SETTINGS_DOWNLOAD_DIRECTORY, // "Default download directory"
	SETTINGS_DOWNLOAD_LIMITS, // "Limits"
	SETTINGS_DOWNLOADS, // "Downloads"
	SETTINGS_DOWNLOADS_MAX, // "Maximum simultaneous downloads (0 = infinite)"
	SETTINGS_DOWNLOADS_SPEED_PAUSE, // "No new downloads if speed exceeds (KiB/s, 0 = disable)"
	SETTINGS_EXAMPLE_TEXT, // "Donate :s! (ok, dirty dollars are fine as well =) (see help menu)"
	SETTINGS_EXTERNAL_IP, // "External / WAN IP"
	SETTINGS_FAV_SHOW_JOINS, // "Only show joins / parts for favorite users"
	SETTINGS_FAVORITE_DIRS_PAGE, // "Downloads\\Favorites"
	SETTINGS_FAVORITE_DIRS, // "Favorite download directories"
	SETTINGS_FILE_NAME, // "Filename"
	SETTINGS_FILTER_MESSAGES, // "Filter kick and NMDC debug messages"
	SETTINGS_FIREWALL_NAT, // "Firewall with manual port forwarding"
	SETTINGS_FIREWALL_PASSIVE, // "Firewall (passive, last resort)"
	SETTINGS_FIREWALL_UPNP, // "Firewall with UPnP"
	SETTINGS_FORMAT, // "Format"
	SETTINGS_GENERAL, // "Personal information"
	SETTINGS_GET_USER_COUNTRY, // "Guess user country from IP"
	SETTINGS_HUB_USER_COMMANDS, // "Accept custom user commands from hub"
	SETTINGS_IGNORE_HUB_PMS, // "Ignore private messages from the hub"
	SETTINGS_IGNORE_BOT_PMS, // "Ignore private messages from bots"
	SETTINGS_INCOMING, // "Incoming connection settings (see Help/FAQ if unsure)"
	SETTINGS_KEEP_LISTS, // "Don't delete file lists when exiting"
	SETTINGS_LANGUAGE_FILE, // "Language file"
	SETTINGS_LIST_DUPES, // "Keep duplicate files in your file list"
	SETTINGS_LOG_DOWNLOADS, // "Log downloads"
	SETTINGS_LOG_FILELIST_TRANSFERS, // "Log filelist transfers"
	SETTINGS_LOG_MAIN_CHAT, // "Log main chat"
	SETTINGS_LOG_PRIVATE_CHAT, // "Log private chat"
	SETTINGS_LOG_STATUS_MESSAGES, // "Log status messages"
	SETTINGS_LOG_SYSTEM_MESSAGES, // "Log system messages"
	SETTINGS_LOG_UPLOADS, // "Log uploads"
	SETTINGS_LOGGING, // "Logging"
	SETTINGS_LOGS, // "Advanced\\Logs"
	SETTINGS_MAGNET_ASK, // "Ask what to do when a magnet link is detected."
	SETTINGS_MAX_FILELIST_SIZE, // "Max filelist size"
	SETTINGS_MAX_HASH_SPEED, // "Max hash speed"
	SETTINGS_MAX_TAB_ROWS, // "Max tab rows"
	SETTINGS_MINIMIZE_TRAY, // "Minimize to tray"
	SETTINGS_NAME, // "Name"
	SETTINGS_NETWORK, // "Connection settings"
	SETTINGS_NO_AWAYMSG_TO_BOTS, // "Don't send the away message to bots"
	SETTINGS_ONLY_DL_TTH_FILES, // "Only download files that have a TTH"
	SETTINGS_ONLY_HASHED, // "Note; Files appear in the share only after they've been hashed!"
	SETTINGS_ONLY_TTH, // "Search for files with TTH root only as standard"
	SETTINGS_OPEN_NEW_WINDOW, // "Open new window when using /join"
	SETTINGS_OPEN_USER_CMD_HELP, // "Always open help file with this dialog"
	SETTINGS_OPTIONS, // "Options"
	SETTINGS_OTHER_QUEUE_OPTIONS, // "Other queue options"
	SETTINGS_OUTGOING, // "Outgoing connection settings"
	SETTINGS_OVERRIDE, // "Don't allow hub/UPnP to override"
	SETTINGS_PERSONAL_INFORMATION, // "Personal Information"
	SETTINGS_PM_BEEP, // "Make an annoying sound every time a private message is received"
	SETTINGS_PM_BEEP_OPEN, // "Make an annoying sound when a private message window is opened"
	SETTINGS_PM_HISTORY, // "PM history"
	SETTINGS_POPUNDER_FILELIST, // "Open new file list windows in the background"
	SETTINGS_POPUNDER_PM, // "Open new private message windows in the background"
	SETTINGS_POPUP_BOT_PMS, // "Open private messages from the hub in their own window"
	SETTINGS_POPUP_HUB_PMS, // "Open private messages from bots in their own window"
	SETTINGS_POPUP_PMS, // "Open private messages in their own window"
	SETTINGS_PORTS, // "Ports"
	SETTINGS_PROMPT_PASSWORD, // "Popup box to input password for hubs"
	SETTINGS_PUBLIC_HUB_LIST, // "Public Hubs list"
	SETTINGS_PUBLIC_HUB_LIST_HTTP_PROXY, // "HTTP Proxy (for hublist only)"
	SETTINGS_PUBLIC_HUB_LIST_URL, // "Public Hubs list URL"
	SETTINGS_QUEUE, // "Downloads\\Queue"
	SETTINGS_RENAME_FOLDER, // "Rename"
	SETTINGS_REQUIRES_RESTART, // "Note; most of these options require that you restart DC++"
	SETTINGS_ROLLBACK, // "Rollback"
	SETTINGS_SEARCH_HISTORY, // "Search history"
	SETTINGS_SELECT_TEXT_FACE, // "Select &text style"
	SETTINGS_SELECT_WINDOW_COLOR, // "Select &window color"
	SETTINGS_SEND_UNKNOWN_COMMANDS, // "Send unknown /commands to the hub"
	SETTINGS_SFV_CHECK, // "Enable automatic SFV checking"
	SETTINGS_SHARE_HIDDEN, // "Share hidden files"
	SETTINGS_SHARE_SIZE, // "Total size:"
	SETTINGS_SHARED_DIRECTORIES, // "Shared directories"
	SETTINGS_SHOW_JOINS, // "Show joins / parts in chat by default"
	SETTINGS_SHOW_PROGRESS_BARS, // "Show progress bars for transfers"
	SETTINGS_SKIP_ZERO_BYTE, // "Skip zero-byte files"
	SETTINGS_SMALL_SEND_BUFFER, // "Use small send buffer (enable if uploads slow downloads a lot)"
	SETTINGS_SOCKS5, // "SOCKS5"
	SETTINGS_SOCKS5_IP, // "Socks IP"
	SETTINGS_SOCKS5_PORT, // "Port"
	SETTINGS_SOCKS5_RESOLVE, // "Use SOCKS5 server to resolve host names"
	SETTINGS_SOCKS5_USERNAME, // "Login"
	SETTINGS_SOUNDS, // "Sounds"
	SETTINGS_SPEEDS_NOT_ACCURATE, // "Note; because of changing download speeds, this is not 100% accurate..."
	SETTINGS_STATUS_IN_CHAT, // "View status messages in main chat"
	SETTINGS_TCP_PORT, // "TCP"
	SETTINGS_TLS_PORT, // "TLS"
	SETTINGS_TEXT_MINISLOT, // "Mini slot size"
	SETTINGS_PRIO_AUTOPRIO, // "Autoprio settings"
	SETTINGS_PRIO_HIGHEST, // "Highest prio max size"
	SETTINGS_PRIO_HIGH, // "High prio max size"
	SETTINGS_PRIO_NORMAL, // "Normal prio max size"
	SETTINGS_PRIO_LOW, // "Low prio max size"
	SETTINGS_PRIO_LOWEST, // "Set lowest prio for newly added files larger than Low prio size"
	SETTINGS_TIME_STAMPS, // "Show timestamps in chat by default"
	SETTINGS_TIME_STAMPS_FORMAT, // "Set timestamps"
	SETTINGS_TOGGLE_ACTIVE_WINDOW, // "Toggle window when selecting an active tab"
	SETTINGS_UDP_PORT, // "UDP"
	SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, // "Unfinished downloads directory"
	SETTINGS_UPLOAD_LINE_SPEED, // "Line speed (upload)"
	SETTINGS_UPLOADS, // "Sharing"
	SETTINGS_UPLOADS_MIN_SPEED, // "Automatically open an extra slot if speed is below (0 = disable)"
	SETTINGS_UPLOADS_SLOTS, // "Upload slots"
	SETTINGS_URL_HANDLER, // "Register with Windows to handle dchub:// and adc:// URL links"
	SETTINGS_URL_MAGNET, // "Register with Windows to handle magnet: URI links"
	SETTINGS_USE_CTRL_FOR_LINE_HISTORY, // "Use CTRL for line history"
	SETTINGS_USE_OEM_MONOFONT, // "Use OEM monospaced font for viewing text files"
	SETTINGS_USE_SYSTEM_ICONS, // "Use system icons when browsing files (slows browsing down a bit)"
	SETTINGS_USER_COMMANDS, // "Advanced\\User Commands"
	SETTINGS_WINDOWS, // "Appearance\\Windows"
	SETTINGS_WINDOWS_OPTIONS, // "Window options"
	SETTINGS_WRITE_BUFFER, // "Write buffer size"
	SETTINGS_ALT_SORT_ORDER, // "Sort all downloads first"
	SETTINGS_USE_TLS, // "Use TLS when remote client supports it"
	SFV_INCONSISTENCY, // "CRC32 inconsistency (SFV-Check)"
	SHARED, // "Shared"
	SHARED_FILES, // "Shared Files"
	SIZE, // "Size"
	SKIP_RENAME, // "New virtual name matches old name, skipping..."
	SLOT_GRANTED, // "Slot granted"
	SLOTS, // "Slots"
	SLOTS_SET, // "Slots set"
	SOCKS_AUTH_FAILED, // "Socks server authentication failed (bad login / password?)"
	SOCKS_AUTH_UNSUPPORTED, // "The socks server doesn't support login / password authentication"
	SOCKS_FAILED, // "The socks server failed establish a connection"
	SOCKS_NEEDS_AUTH, // "The socks server requires authentication"
	SOCKS_SETUP_ERROR, // "Failed to set up the socks server for UDP relay (check socks address and port)"
	SOURCE_TOO_OLD, // "Remote client does not fully support TTH - cannot download"
	SOURCE_TOO_SLOW, // "Source too slow"
	SOURCE_TYPE, // "Source Type"
	SPECIFY_SEARCH_STRING, // "Specify a search string"
	SPECIFY_SERVER, // "Specify a server to connect to"
	SPECIFY_URL, // "Specify a URL"
	SPEED, // "Speed"
	STATUS, // "Status"
	STORED_PASSWORD_SENT, // "Stored password sent..."
	SYSTEM_LOG, // "System Log"
	TAG, // "Tag"
	TARGET_FILENAME_TOO_LONG, // "Target filename too long"
	TCP_PORT_BUSY, // "Unable to open TCP/TLS port. File transfers will not work correctly until you change settings or turn off any application that might be using the TCP/TLS port"
	TiB, // "TiB"
	TIME, // "Time"
	TIME_LEFT, // "Time left"
	TIMESTAMPS_DISABLED, // "Timestamps disabled"
	TIMESTAMPS_ENABLED, // "Timestamps enabled"
	TOO_MUCH_DATA, // "More data was sent than was expected"
	TOTAL, // "Total: "
	TTH_ALREADY_SHARED, // "A file with the same hash already exists in your share"
	TTH_INCONSISTENCY, // "TTH inconsistency"
	TTH_ROOT, // "TTH Root"
	TYPE, // "Type"
	UDP_PORT_BUSY, // "Unable to open UDP port. Searching will not work correctly until you change settings or turn off any application that might be using the UDP port"
	UNABLE_TO_CREATE_THREAD, // "Unable to create thread"
	UNABLE_TO_OPEN_FILELIST, // "Unable to open filelist: "
	UNABLE_TO_RENAME, // "Unable to rename "
	UNABLE_TO_SEND_FILE, // "Unable to send file "
	UNKNOWN, // "Unknown"
	UNKNOWN_ADDRESS, // "Unknown address"
	UNKNOWN_COMMAND, // "Unknown command: "
	UNKNOWN_ERROR, // "Unknown error: 0x%x"
	UNSUPPORTED_FILELIST_FORMAT, // "Unsupported filelist format"
	UPLOAD_FINISHED_IDLE, // "Upload finished, idle..."
	UPLOAD_STARTING, // "Upload starting..."
	UPLOADED_BYTES, // "Uploaded %s (%.01f%%) in %s"
	UPLOADED_TO, // " uploaded to "
	UPLOADS, // "Uploads"
	UPNP_FAILED_TO_CREATE_MAPPINGS, // "Failed to create port mappings. Please set up your NAT yourself."
	UPNP_FAILED_TO_REMOVE_MAPPINGS, // "Failed to remove port mappings"
	UPNP_FAILED_TO_GET_EXTERNAL_IP, // "Failed to get external IP via  UPnP. Please set it yourself."
	USER, // "User"
	USER_CMD_CHAT, // "Chat"
	USER_CMD_COMMAND, // "Command"
	USER_CMD_CONTEXT, // "Context"
	USER_CMD_FILELIST_MENU, // "Filelist Menu"
	USER_CMD_HUB, // "Hub IP / DNS (empty = all, 'op' = where operator)"
	USER_CMD_HUB_MENU, // "Hub Menu"
	USER_CMD_ONCE, // "Send once per nick"
	USER_CMD_PARAMETERS, // "Parameters"
	USER_CMD_PM, // "PM"
	USER_CMD_PREVIEW, // "Text sent to hub"
	USER_CMD_RAW, // "Raw"
	USER_CMD_SEARCH_MENU, // "Search Menu"
	USER_CMD_TO, // "To"
	USER_CMD_TYPE, // "Command Type"
	USER_CMD_USER_MENU, // "User Menu"
	USER_CMD_WINDOW, // "Create / Modify Command"
	USER_DESCRIPTION, // "User Description"
	USER_OFFLINE, // "User offline"
	USER_WENT_OFFLINE, // "User went offline"
	USERS, // "Users"
	VIDEO, // "Video"
	VIEW_AS_TEXT, // "View as text"
	VIRTUAL_NAME, // "Virtual name"
	VIRTUAL_NAME_EXISTS, // "Virtual directory name already exists"
	VIRTUAL_NAME_LONG, // "Name under which the others see the directory"
	WAITING, // "Waiting..."
	WAITING_TO_RETRY, // "Waiting to retry..."
	WAITING_USERS, // "Waiting Users"
	WAITING_USER_ONLINE, // "Waiting (User online)"
	WAITING_USERS_ONLINE, // "Waiting (%d of %d users online)"
	YES_STR, // "Yes"
	YOU_ARE_BEING_REDIRECTED, // "You are being redirected to "
	LAST // @DontAdd
};
