#pragma once

// Command ids shared by the client's controllers, command registries, menu
// models and action definitions.
//
// This started life as a Visual C++ generated resource header for the client's
// Win32 resource script; the script is gone (see app/client_icon.rc) and these
// are now plain command identifiers. Numeric values are reused across unrelated
// symbols -- an artefact of the resource editor -- so never key a lookup table
// on the raw number without checking for collisions.
#define ID_VIEW_ADD_TO_FAVOURITES       101
#define ID_GRAPH_VIEW                   104
#define ID_TS_FORMATS_VIEW              105
#define ID_CELLS_VIEW                   106
#define ID_SIMULATION_ITEMS_VIEW        107
#define ID_HARDWARE_VIEW                108
#define ID_WIN_RENAME                   108
#define ID_USERS_VIEW                   109
// The Administration explorer pane (the rail's Administration mode).
#define ID_ADMINISTRATION_VIEW          135
#define ID_ACKNOWLEDGE_CURRENT          110
#define ID_ACKNOWLEDGE_ALL              111
#define ID_SHEET_VIEW                   112
#define ID_SUMMARY_VIEW                 113
#define ID_TIMED_DATA_VIEW              114
#define ID_WRITE                        115
#define IDB_COPY                        115
#define ID_WRITE_MANUAL                 116
#define IDB_PASTE                       116
#define ID_UNLOCK_ITEM                  117
#define ID_APPLICATION                  117
#define ID_EDIT_LIMITS                  118
#define ID_ITEM_PARAMS                  119
#define ID_TABLE_CONFIG                 120
#define ID_OBJECT_VIEW                  121
#define ID_EVENT_VIEW                   122
#define ID_EVENT_JOURNAL_VIEW           123
#define ID_FAVOURITES_VIEW              124
#define ID_PORTFOLIO_VIEW               125
#define ID_WATCH_VIEW                   126
#define ID_STATISTICS_VIEW              127
#define ID_MOVE_UP                      127
#define ID_WEB_VIEW                     128
#define ID_MOVE_DOWN                    128
#define ID_DELETE                       129
#define ID_OPEN_GRAPH                   130
#define ID_OPEN_TABLE                   131
#define ID_SETUP                        132
#define ID_PRINT                        133
#define ID_EXPORT_CSV                   134
#define ID_NEW_IEC60870_LINK101         135
#define ID_NEW_IEC60870_LINK104         136
#define ID_OPEN_SUMMARY                 137
#define ID_OPEN_DISPLAY                 138
#define ID_OPEN_EVENTS                  139
#define ID_DEV1_REFR                    140
#define ID_DEV1_SYNC                    141
#define ID_OPEN_WATCH                   142
#define ID_CHANGE_PASSWORD              145
#define ID_ITEM_ENABLE                  146
#define ID_ITEM_DISABLE                 147
#define ID_TRANSMISSION_VIEW            149
#define ID_MODUS_VIEW                   150
#define ID_HISTORICAL_EVENTS            151
#define ID_PROPERTY_VIEW                152
#define ID_TABLE_EDITOR                 153
#define ID_MODUS_TOOLBAR                154
#define ID_MODUS_STATUSBAR              155
#define ID_CURRENT_EVENTS               156
#define ID_TIME_RANGE_DAY               157
#define ID_TIME_RANGE_WEEK              158
#define ID_TIME_RANGE_MONTH             159
#define ID_TIME_RANGE_HOUR              160
#define ID_TIME_RANGE_15M               170
#define ID_EDIT                         161
#define ID_PRINT_PREVIEW                162
#define ID_VIEW_CHANGE_TITLE            164
#define ID_VIEW_CLOSE                   165
#define ID_TIME_RANGE_CUSTOM            166
#define ID_SEVERITY_HIGH                167
#define ID_SEVERITY_ALL                 168
#define ID_SEVERITY_CUSTOM              169
#define ID_NEW_PORTFOLIO                171
#define ID_PAUSE                        172
#define ID_WATCH_FRAME_TRACE            173
#define ID_NEW_SERVICE_ITEMS            174
#define ID_ADD_MULTIPLE_ITEMS           175
#define ID_VIEW_CURSOR                  176
#define ID_SAVE_AS                      177
#define ID_OPEN_DEVICE_METRICS          178
#define ID_EXCEL_REPORT_VIEW            179
#define ID_OPEN_GROUP_TABLE             180
#define ID_VIDICON_DISPLAY_VIEW         182
#define ID_ABOUT_QT                     184
#define ID_WINDOW_SPLIT_HORZ            185
#define ID_WINDOW_SPLIT_VERT            186
#define ID_AGGREGATION_END              187
#define ID_AGGREGATION_COUNT            188
#define ID_AGGREGATION_MIN              189
#define ID_AGGREGATION_MAX              190
#define ID_AGGREGATION_AVG              191
#define ID_AGGREGATION_SUM              192
#define ID_INTERVAL_1M                  193
#define ID_INTERVAL_5M                  194
#define ID_INTERVAL_15M                 195
#define ID_INTERVAL_30M                 196
#define ID_INTERVAL_1H                  197
#define ID_INTERVAL_12H                 198
#define ID_INTERVAL_1D                  199
#define ID_AGGREGATION_START            200
#define ID_GRAPH_COLOR                  201
#define ID_GRAPH_BK_COLOR               4035
#define ID_GRAPH_SETUP                  4036
#define ID_LANGUAGE_ENGLISH             4042
#define ID_LANGUAGE_RUSSIAN             4043
#define ID_EXPORT_EXCEL                 202
#define ID_DUMP_DEBUG_INFO              203
#define ID_FAVOURITES_ADD_URL           204
#define ID_FILE_SYSTEM_VIEW             205
#define ID_ADD_FILE                     328
#define ID_CREATE_FILE_DIRECTORY        329
#define IDB_PRINTER                     257
#define IDB_DELETE                      258
#define IDB_OPEN_EVENTS                 261
#define IDB_TIMED_DATA                  263
#define IDB_UNLOCK                      264
#define IDB_WRITE                       265
#define IDB_WRITE_MANUAL                266
#define IDB_SUMMARY                     267
#define IDB_RECORD_EDITOR               269
#define ID_TABLE_VIEW                   270
#define IDB_ACKNOWLEDGE_ALL             271
#define ID_NEW_PROPERTY_VIEW            327
#define ID_OK                           1012
#define ID_CANCEL                       1013
#define ID_HISTORICAL_DB_VIEW           2017
// The RoleSet: every Role and the accounts it is granted to (OPC UA Part 18
// §4.4.1). Role membership is the stored authorization model.
#define ID_ROLES_VIEW                   2018
// The password policy the server publishes (OPC UA Part 18 §5.2.2).
#define ID_PASSWORD_POLICY_VIEW         2019
// The audit trail: the event journal scoped to the AuditEventType subtree.
#define ID_AUDIT_LOG_VIEW               2020
#define ID_GRAPH_ZOOM                   4001
#define ID_EXPORT_CONFIGURATION_TO_EXCEL 4022
#define ID_IMPORT_CONFIGURATION_FROM_EXCEL 4023
#define ID_LOGIN                        4024
#define ID_LOGOFF                       4025
#define ID_VIEW_PUBLIC_FOLDER           4026
#define ID_NODES_VIEW                   4029
#define ID_SORT_NONE                    4030
#define ID_SORT_ALIAS                   4032
#define ID_POPUP_4033                   4033
#define ID_POPUP_4034                   4034
#define ID_PAGE_DELETE                  32787
#define ID_VIEW_LEGEND                  32797
#define ID_NOW                          32830
#define ID_RENAME                       32838
#define ID_HIST                         32840
#define ID_GRAPH_ADD_PANE               32856
#define ID_GRAPH_DELETE_PANE            32858
#define ID_COLORS                       32864
#define ID_CURBOX                       32874
#define ID_PAGE_RENAME                  32875
#define ID_SAVE                         32885
#define ID_COPY                         32887
#define ID_PASTE                        32889
#define ID_CUT                          32891
#define ID_SHOW_WRITEOK                 32893
#define ID_SHOW_EVENTS                  32895
#define ID_HIDE_EVENTS                  32897
#define ID_ITEM_COMMANDS                32903
#define ID_WRITE_CONFIRMATION           32921
#define ID_OPEN                         32945
#define ID_BUTTON32946                  32946
#define ID_ADD_ITEMS                    32960
#define ID_EVENT_FLASH_WINDOW           32979
#define ID_EVENT_PLAY_SOUND             32980
#define ID_GRAPH_DOTS                   32984
#define ID_GRAPH_STEPS                  32985
#define ID_GRAPH_SCROLL_BAR             32986
#define ID_SORT_NAME                    32991
#define ID_SORT_CHANNEL                 32992
#define ID_CLEAR_ALL                    32993
#define ID_BUTTON33002                  33002
#define ID_BUTTON33003                  33003
#define ID_HELP_MANUAL                  33009
#define ID__                            33011
#define ID__33013                       33013
#define ID_PAGE_NEW                     34122
#define ID_OPT_SPEECH                   34123
#define ID_UNACKNOWLEDGED_ONLY          34124
// Opens the preferences dialog. Reached from Settings > Settings... and from
// the activity rail's pinned Settings utility, which share this one command so
// the two entry points cannot diverge.
#define ID_SETTINGS_DIALOG              34125
// The rest of the rail's page context menu. Page-scoped rather than reusing
// ID_MOVE_UP / ID_MOVE_DOWN, whose numeric values (127/128) already belong to
// unrelated item commands.
#define ID_PAGE_OPEN                    34126
#define ID_PAGE_DUPLICATE               34127
#define ID_PAGE_MOVE_UP                 34128
#define ID_PAGE_MOVE_DOWN               34129
#define ID_NEW                          40000
#define ID_NEW_DISPLAY_0                40100
#define ID_NEW_REPORT_0                 40200
#define ID_PAGE_0                       40300
#define ID_COLOR_0                      40400
#define ID_WIN_0                        40500
#define ID_TRASH_0                      40600
#define ID_FAV_TABLE_0                  40800
#define ID_FAV_GRAPH_0                  40900
