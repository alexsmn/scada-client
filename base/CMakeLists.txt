scada_module(client_base)

target_precompile_headers(client_base REUSE_FROM scada_base)

target_link_libraries(client_base PUBLIC scada_base)

set(CLIENT_UI_COMMON_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/common/client_utils.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/common/client_utils.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../resources/common_resources.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../app/string_const.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../web/web_util.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../web/web_util.h"
)

add_library(client_ui_common_qt ${CLIENT_UI_COMMON_SOURCES})
target_precompile_headers(client_ui_common_qt REUSE_FROM scada_base)
target_link_libraries(client_ui_common_qt PRIVATE
  aui_qt
  client_common
  client_events_qt
  client_profile_qt
  client_services_qt
)

add_library(client_ui_common_wt ${CLIENT_UI_COMMON_SOURCES})
target_precompile_headers(client_ui_common_wt REUSE_FROM scada_base)
target_link_libraries(client_ui_common_wt PRIVATE
  aui_wt
  client_common
  client_events_wt
  client_profile_wt
  client_services_wt
)

add_library(client_ui_dragdrop_qt
  "${CMAKE_CURRENT_SOURCE_DIR}/../aui/dragdrop/item_drag_data.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../aui/dragdrop/item_drag_data.h"
)
target_precompile_headers(client_ui_dragdrop_qt REUSE_FROM scada_base)
target_link_libraries(client_ui_dragdrop_qt PRIVATE
  aui_qt
  client_common
)

add_library(client_ui_dragdrop_wt
  "${CMAKE_CURRENT_SOURCE_DIR}/../aui/dragdrop/item_drag_data.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../aui/dragdrop/item_drag_data.h"
)
target_precompile_headers(client_ui_dragdrop_wt REUSE_FROM scada_base)
target_link_libraries(client_ui_dragdrop_wt PRIVATE
  aui_wt
  client_common
)

add_library(client_ui_progress_qt
  "${CMAKE_CURRENT_SOURCE_DIR}/../services/ui/node_service_progress_tracker.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../services/ui/node_service_progress_tracker.h"
)
target_precompile_headers(client_ui_progress_qt REUSE_FROM scada_base)
target_link_libraries(client_ui_progress_qt PRIVATE
  client_common
)

add_library(client_ui_progress_wt
  "${CMAKE_CURRENT_SOURCE_DIR}/../services/ui/node_service_progress_tracker.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../services/ui/node_service_progress_tracker.h"
)
target_precompile_headers(client_ui_progress_wt REUSE_FROM scada_base)
target_link_libraries(client_ui_progress_wt PRIVATE
  client_common
)
