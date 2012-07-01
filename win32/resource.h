#ifndef DCPLUSPLUS_WIN32_RESOURCE_H
#define DCPLUSPLUS_WIN32_RESOURCE_H

#include "../help/resource.h"

#define VS_VERSION_INFO 1

#define IDI_DCPP				100
#define IDI_PUBLICHUBS				101
#define IDI_SEARCH					102
#define IDI_FAVORITE_HUBS				103
#define IDI_PRIVATE					104
#define IDI_DIRECTORY				105
#define IDI_HUB						106
#define IDI_NOTEPAD					107
#define IDI_QUEUE					108
#define IDI_FINISHED_DL				109
#define IDI_FINISHED_UL				110
#define IDI_ADLSEARCH				111
#define IDI_USERS					112
#define IDI_SPY						113
#define IDI_NET_STATS				114
#define IDI_MAGNET					116
#define IDI_HUB_OFF					117
#define IDI_PRIVATE_OFF					118
#define IDI_GROUPED_BY_FILES 119
#define IDI_GROUPED_BY_USERS 120
#define IDI_EXIT 121
#define IDI_CHAT 122
#define IDI_HELP 123
#define IDI_OPEN_DL_DIR 124
#define IDI_OPEN_FILE_LIST 125
#define IDI_RECONNECT 126
#define IDI_SETTINGS 127
#define IDI_TRAY_PM 128
#define IDI_TRUSTED 129
#define IDI_SECURE 130
#define IDI_RECENTS 131
#define IDI_WHATS_THIS 132
#define IDI_DOWNLOAD 133
#define IDI_UPLOAD 134
#define IDI_CHANGELOG 135
#define IDI_DONATE 136
#define IDI_GET_STARTED 137
#define IDI_INDEXING 138
#define IDI_LINKS 139
#define IDI_REFRESH 140
#define IDI_SLOTS 141
#define IDI_OK 142
#define IDI_CANCEL 143
#define IDI_LEFT 144
#define IDI_RIGHT 145
#define IDI_USER 146
#define IDI_USER_AWAY 147
#define IDI_USER_BOT 148
#define IDI_USER_NOCON 149
#define IDI_USER_NOSLOT 150
#define IDI_USER_OP 151
#define IDI_UP 152
#define IDI_FILE 153
#define IDI_EXEC 154
#define IDI_FAVORITE_USER_ON 155
#define IDI_FAVORITE_USER_OFF 156
#define IDI_GRANT_SLOT_ON 157
#define IDI_GRANT_SLOT_OFF 158
#define IDI_SLOTS_FULL 159
#define IDI_ADVANCED 160
#define IDI_CLOCK 161
#define IDI_STYLES 162
#define IDI_BW_LIMITER 163
#define IDI_CONN_GREY 164
#define IDI_CONN_BLUE 165
#define IDI_EXPERT 166
#define IDI_FAVORITE_DIRS 167
#define IDI_LOGS 168
#define IDI_NOTIFICATIONS 169
#define IDI_PROXY 170
#define IDI_TABS 171
#define IDI_WINDOWS 172
#define IDI_BALLOON 173
#define IDI_SOUND 174
#define IDI_ULIMIT 175
#define IDI_DLIMIT 176
#define IDI_DEBUG 177
#define IDI_PLUGINS 178


/* PluginPage */
#define IDH_PLUGIN_PAGE 11668
#define IDH_PLUGINS_INFO 11669
#define IDH_PLUGINS_INSTALLED 11670
#define IDH_PLUGIN_ADD 11671
#define IDH_PLUGIN_CONFIGURE 11672
#define IDH_PLUGIN_UP 11673
#define IDH_PLUGIN_DOWN 11674
#define IDH_PLUGIN_REMOVE 11675
// Stuff that uses multiple id's

#define ID_SHELLCONTEXTMENU_MIN				3000
#define ID_SHELLCONTEXTMENU_MAX				3999

// reserved for help ids
#define IDH_BEGIN 10000
#define IDH_END 11999

// WM_APP messages
#define ID_UPDATECOLOR (WM_APP + 0)

#endif
