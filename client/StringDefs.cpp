#include "stdinc.h"
#include "DCPlusPlus.h"
string ResourceManager::strings[] = {
	"Active", "Active / Search String", "Add", "Add To Favorites", "Address already in use", "Address not available", "Automatic Directory Listing Search", "All download slots taken", "All %d users offline", "All 3 users offline", 
"All 4 users offline", "Already getting that file list", "Any", "At least", "At most", "Audio", "Auto connect / Name", "Average/s: ", "AWAY", "Away mode off", 
"Away mode on: ", "B", "Both users offline", "Browse...", "Can't download from passive user while in passive mode", "Choose folder", "Close connection", "Closing connection...", "Close", "Compressed", 
"Error during compression", "Connect", "Connected", "Connecting...", "Connecting (forced)...", "Connecting to ", "Connection", "Connection closed", "Connection refused by target machine", "Connection reset by server", 
"Connection timeout", "Copy nick to clipboard", "Could not open target file: ", "Could not open file", "Count", "CRC Checked", "Error during decompression", "Description", "Destination", "Directory", 
"Directory already shared", "Disconnected", "Disk full(?)", "Document", "Done", "Download", "Download failed: ", "Download finished, idle...", "Download starting...", "Download to...", 
"Download queue", "Download whole directory", "Download whole directory to...", "Downloaded %s (%.01f%%), %s/s, %s left", " downloaded from ", "Downloading...", "Downloading public hub list...", "Duplicate source", "Edit", "E-Mail", 
"Please enter a nickname in the settings dialog!", "Please enter a password", "Please enter a reason", "Enter search string", "Please enter a destination server", "Error opening file", "Errors", "Exact size", "Executable", "Favorite Hubs", 
"Favorite hub added", "Favorite Users", "Favorite user added", "File", "Files", "File list refreshed", "File not available", "File type", "A file with a different size already exists in the queue", "Filename", 
"Filter", "Find", "Finished Downloads", "Finished Uploads", "Force attempt", "GB", "Get file list", "Go to directory", "Grant extra slot", "High", 
"Highest", "Hit Ratio: ", "Hits: ", "Host unreachable", "Hub", "Hubs", "Address", "Hub list downloaded...", "Name", "Hub password", 
"Users", "Ignored message: ", "Invalid number of slots", "Invalid target file (missing directory, check default download directory setting)", "Ip: ", "Items", "Joins: ", "Join/part showing off", "Join/part showing on", "kB", 
"Kick user(s)", "A file of equal or larger size already exists at the target location", "Hub (last seen on if offline)", "Loading DC++, please wait...", "Low", "Lowest", "Manual connect address", "Match queue", "Matched %d file(s)", "MB", 
"&File", "ADL Search", "&Download Queue\tCtrl+D", "&Exit", "&Favorite Hubs\tCtrl+F", "Favorite &Users\tCtrl+U", "Follow last redirec&t\tCtrl+T", "Import queue from NMDC...", "&Notepad\tCtrl+N", "Open file list...", 
"&Public Hubs\tCtrl+P", "&Reconnect\tCtrl+R", "&Search\tCtrl+S", "Search spy", "Settings...", "&Help", "About DC++...", "Change Log", "DC++ discussion forum", "Donate ���/$$$ (paypal)", 
"Downloads and translations", "Frequently asked questions", "Help forum", "DC++ Homepage", "Readme / Newbie help", "Request a feature", "Report a bug", "&View", "&Status bar", "&Toolbar", 
"&Window", "Arrange icons", "Cascade", "Close disconnected", "Minimize &All", "Tile", "Move/Rename", "Move Down", "Move Up", "Network unreachable (are you connected to the internet?)", 
"Next", "New...", "Nick", "Your nick was already taken, please change to something else!", " (Nick unknown)", "Non-blocking operation still in progress", "Not connected", "Not a socket", "No", "No directory specified", 
"You're trying to download from yourself!", "No errors", "No matches", "No slots available", "No users", "No users to download from", "Normal", "Notepad", "Offline", "Online", 
"Only users with free slots", "Open", "Open folder", "Operation would block execution", "Out of buffer space", "Passive user", "Password", "Parts: ", "Path", "Paused", 
"Permission denied", "Picture", "Port: ", "Port %d is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart DC++", "Preparing file list...", "Press the follow redirect button to connect to ", "Priority", "Private message from ", "Properties", "Public Hubs", 
"Readd source", "Really exit?", "Redirect", "Redirect request received to a hub that's already connected", "Redirect user(s)", "Refresh", "Refresh user list", "Remove", "Remove all", "Remove all subdirectories before adding this one", 
"Remove user from queue", "Remove source", "Rollback inconsistency, existing file does not match the one being downloaded", "Running...", "Search", "Search for", "Search for alternates", "Search for file", "Search options", "Search spam detected from ", 
"Search Spy", "Search String", "Searching for ", "Send private message", "Server", "Set priority", "Advanced", "Use antifragmentation method for downloads", "Auto-away on minimize (and back on restore)", "Automatically follow redirects", 
"Automatically disconnect users who leave the hub (does not disconnect when hub goes down / you leave it)", "Automatically search for alternative download locations", "Automatically refresh share list every hour", "Clear search box after each search", "Command", "Ignore messages from users that are not online (effective against bots)", "Don't delete file lists when exiting", "Name", "Don't send the away message to bots", "Open the public hubs window at startup", 
"Open the download queue window at startup", "Popup messages from users that are not online (if not ignoring, messages go to main chat if enabled)", "Popup private messages", "Remove dupes completely from your share (otherwise, only their size is subtracted, but the files can be seen by others)", "Enable automatic SFV checking", "Show progress bars for transfers (uses some CPU)", "Skip zero-byte files", "Use small send buffer (enable if uploads slow downloads a lot)", "Install URL handler on startup (to handle dchub:// links)", "Appearance", 
"Set Finished Manager(s) tab bold when an entry is added", "Full row select in lists", "Filter kick and NMDC debug messages", "Minimize to tray", "Show timestamps in chat by default", "Confirm application exit", "View status messages in main chat", "Show joins / parts in chat by default", "Use system icons when browsing files (slows browsing down a bit)", "Downloads", 
"General", "Logs and Sound", "Sharing", "CRC32 inconsistency (SFV-Check)", "Shared", "Shared Files", "Size", "Max Size", "Min Size", "Slot granted", 
"Slots", "Slots set", "Socket has been shut down", "Socks server authentication failed (bad username / password?)", "The socks server doesn't support user / password authentication", "The socks server failed establish a connection", "The socks server requires authentication", "Failed to set up the socks server for UDP relay (check socks address and port)", "Source Type", "Specify a server to connect to", 
"Specify a search string", "Speed", "Status", "Target filename too long", "TB", "Time", "Timestamps disabled", "Timestamps enabled", "Total: ", "Type", 
"Unable to create thread", "Unknown", "Unknown address", "Unknown error: 0x%x", "Unsupported filelist format", "Upload finished, idle...", "Upload starting...", "Uploaded %s (%.01f%%), %s/s, %s left", " uploaded to ", "User", 
"User Description", "User offline", "User went offline", "Users", "Video", "Waiting...", "Waiting (User online)", "Waiting (%d of %d users online)", "Waiting to retry...", "What's this?", 
"Yes", "You are being redirected to "
};
string ResourceManager::names[] = {
	"Active", "ActiveSearchString", "Add", "AddToFavorites", "AddressAlreadyInUse", "AddressNotAvailable", "AdlSearch", "AllDownloadSlotsTaken", "AllUsersOffline", "All3UsersOffline", 
"All4UsersOffline", "AlreadyGettingThatList", "Any", "AtLeast", "AtMost", "Audio", "AutoConnect", "Average", "Away", "AwayModeOff", 
"AwayModeOn", "B", "BothUsersOffline", "Browse", "CantConnectInPassiveMode", "ChooseFolder", "CloseConnection", "ClosingConnection", "Close", "Compressed", 
"CompressionError", "Connect", "Connected", "Connecting", "ConnectingForced", "ConnectingTo", "Connection", "ConnectionClosed", "ConnectionRefused", "ConnectionReset", 
"ConnectionTimeout", "CopyNick", "CouldNotOpenTargetFile", "CouldNotOpenFile", "Count", "CrcChecked", "DecompressionError", "Description", "Destination", "Directory", 
"DirectoryAlreadyShared", "Disconnected", "DiscFull", "Document", "Done", "Download", "DownloadFailed", "DownloadFinishedIdle", "DownloadStarting", "DownloadTo", 
"DownloadQueue", "DownloadWholeDir", "DownloadWholeDirTo", "DownloadedLeft", "DownloadedFrom", "Downloading", "DownloadingHubList", "DuplicateSource", "Edit", "Email", 
"EnterNick", "EnterPassword", "EnterReason", "EnterSearchString", "EnterServer", "ErrorOpeningFile", "Errors", "ExactSize", "Executable", "FavoriteHubs", 
"FavoriteHubAdded", "FavoriteUsers", "FavoriteUserAdded", "File", "Files", "FileListRefreshed", "FileNotAvailable", "FileType", "FileWithDifferentSize", "Filename", 
"Filter", "Find", "FinishedDownloads", "FinishedUploads", "ForceAttempt", "Gb", "GetFileList", "GoToDirectory", "GrantExtraSlot", "High", 
"Highest", "HitRatio", "Hits", "HostUnreachable", "Hub", "Hubs", "HubAddress", "HubListDownloaded", "HubName", "HubPassword", 
"HubUsers", "IgnoredMessage", "InvalidNumberOfSlots", "InvalidTargetFile", "Ip", "Items", "Joins", "JoinShowingOff", "JoinShowingOn", "Kb", 
"KickUser", "LargerTargetFileExists", "LastHub", "Loading", "Low", "Lowest", "ManualAddress", "MatchQueue", "MatchedFiles", "Mb", 
"MenuFile", "MenuFileAdlSearch", "MenuFileDownloadQueue", "MenuFileExit", "MenuFileFavoriteHubs", "MenuFileFavoriteUsers", "MenuFileFollowRedirect", "MenuFileImportQueue", "MenuFileNotepad", "MenuFileOpenFileList", 
"MenuFilePublicHubs", "MenuFileReconnect", "MenuFileSearch", "MenuFileSearchSpy", "MenuFileSettings", "MenuHelp", "MenuHelpAbout", "MenuHelpChangelog", "MenuHelpDiscuss", "MenuHelpDonate", 
"MenuHelpDownloads", "MenuHelpFaq", "MenuHelpHelpForum", "MenuHelpHomepage", "MenuHelpReadme", "MenuHelpRequestFeature", "MenuHelpReportBug", "MenuView", "MenuViewStatusBar", "MenuViewToolbar", 
"MenuWindow", "MenuWindowArrange", "MenuWindowCascade", "MenuWindowCloseDisconnected", "MenuWindowMinimizeAll", "MenuWindowTile", "Move", "MoveDown", "MoveUp", "NetworkUnreachable", 
"Next", "New", "Nick", "NickTaken", "NickUnknown", "NonBlockingOperation", "NotConnected", "NotSocket", "No", "NoDirectorySpecified", 
"NoDownloadsFromSelf", "NoErrors", "NoMatches", "NoSlotsAvailable", "NoUsers", "NoUsersToDownloadFrom", "Normal", "Notepad", "Offline", "Online", 
"OnlyFreeSlots", "Open", "OpenFolder", "OperationWouldBlockExecution", "OutOfBufferSpace", "PassiveUser", "Password", "Parts", "Path", "Paused", 
"PermissionDenied", "Picture", "Port", "PortIsBusy", "PreparingFileList", "PressFollow", "Priority", "PrivateMessageFrom", "Properties", "PublicHubs", 
"ReaddSource", "ReallyExit", "Redirect", "RedirectAlreadyConnected", "RedirectUser", "Refresh", "RefreshUserList", "Remove", "RemoveAll", "RemoveAllSubdirectories", 
"RemoveFromAll", "RemoveSource", "RollbackInconsistency", "Running", "Search", "SearchFor", "SearchForAlternates", "SearchForFile", "SearchOptions", "SearchSpamFrom", 
"SearchSpy", "SearchString", "SearchingFor", "SendPrivateMessage", "Server", "SetPriority", "SettingsAdvanced", "SettingsAdvancedAntiFrag", "SettingsAdvancedAutoAway", "SettingsAdvancedAutoFollow", 
"SettingsAdvancedAutoKick", "SettingsAdvancedAutoSearch", "SettingsAdvancedAutoUpdateList", "SettingsAdvancedClearSearch", "SettingsAdvancedCommand", "SettingsAdvancedIgnoreOffline", "SettingsAdvancedKeepLists", "SettingsAdvancedName", "SettingsAdvancedNoAwaymsgToBots", "SettingsAdvancedOpenPublic", 
"SettingsAdvancedOpenQueue", "SettingsAdvancedPopupOffline", "SettingsAdvancedPopupPms", "SettingsAdvancedRemoveDupes", "SettingsAdvancedSfvCheck", "SettingsAdvancedShowProgressBars", "SettingsAdvancedSkipZeroByte", "SettingsAdvancedSmallSendBuffer", "SettingsAdvancedUrlHandler", "SettingsAppearance", 
"SettingsAppearanceFinishedDirty", "SettingsAppearanceFullRowSelect", "SettingsAppearanceFilterMessages", "SettingsAppearanceMinimizeTray", "SettingsAppearanceTimeStamps", "SettingsAppearanceConfirmExit", "SettingsAppearanceStatusInChat", "SettingsAppearanceShowJoins", "SettingsAppearanceUseSystemIcons", "SettingsDownloads", 
"SettingsGeneral", "SettingsLogs", "SettingsUploads", "SfvInconsistency", "Shared", "SharedFiles", "Size", "SizeMax", "SizeMin", "SlotGranted", 
"Slots", "SlotsSet", "SocketShutDown", "SocksAuthFailed", "SocksAuthUnsupported", "SocksFailed", "SocksNeedsAuth", "SocksSetupError", "SourceType", "SpecifyServer", 
"SpecifySearchString", "Speed", "Status", "TargetFilenameTooLong", "Tb", "Time", "TimestampsDisabled", "TimestampsEnabled", "Total", "Type", 
"UnableToCreateThread", "Unknown", "UnknownAddress", "UnknownError", "UnsupportedFilelistFormat", "UploadFinishedIdle", "UploadStarting", "UploadedLeft", "UploadedTo", "User", 
"UserDescription", "UserOffline", "UserWentOffline", "Users", "Video", "Waiting", "WaitingUserOnline", "WaitingUsersOnline", "WaitingToRetry", "WhatsThis", 
"Yes", "YouAreBeingRedirected"
};
