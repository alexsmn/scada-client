scada_module(client_base)

# No PCH in the modules pilot (see core docs: scan-deps cannot read it).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(client_base REUSE_FROM scada_base)
endif()

target_link_libraries(client_base PUBLIC scada_base)

target_sources(client_base PRIVATE
  "file_settings_store.cpp"
  "file_settings_store.h"
  "e2e_test_hooks.cpp"
  "e2e_test_hooks.h"
)

# Modules-pilot consumer: file_settings_store.cpp imports scada.base (see
# SCADA_USE_BASE_MODULE there).
scada_module_import_pilot(client_base
  IMPORTS scada_base_module
  DEFINES SCADA_USE_BASE_MODULE)

scada_add_module_facade(client_base FILES scada_client_base.cppm
  IMPORTS scada_base_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()

set(CLIENT_UI_COMMON_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/common/client_utils.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/common/client_utils.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../resources/common_resources.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../resources/icon_strips.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../app/string_const.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/web_util.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/web_util.h"
)

add_library(client_ui_common_qt ${CLIENT_UI_COMMON_SOURCES}
  # Qt-only helpers over client-app resources (res/client.qrc icon paths keyed
  # by common_resources.h ids); deliberately outside aui/, which must stay free
  # of client-repo resource couplings.
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/qt/client_utils_qt.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/qt/client_utils_qt.h"
)
target_precompile_headers(client_ui_common_qt REUSE_FROM scada_base)
target_link_libraries(client_ui_common_qt PRIVATE
  aui_qt
  client_common
  client_events_qt
  client_profile_qt
  client_services_qt
  # `client_utils.cpp` exposes coroutine helpers and uses awaitable adapters.
  transport
)

if("wt" IN_LIST CLIENT_UI_CONFIGS)
  add_library(client_ui_common_wt ${CLIENT_UI_COMMON_SOURCES})
  target_precompile_headers(client_ui_common_wt REUSE_FROM scada_base)
  target_link_libraries(client_ui_common_wt PRIVATE
    aui_wt
    client_common
    client_events_wt
    client_profile_wt
    client_services_wt
    # `client_utils.cpp` exposes coroutine helpers and uses awaitable adapters.
    transport
  )
endif()

add_library(client_ui_dragdrop_qt
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/dragdrop/item_drag_data.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/dragdrop/item_drag_data.h"
)
target_precompile_headers(client_ui_dragdrop_qt REUSE_FROM scada_base)
target_link_libraries(client_ui_dragdrop_qt PRIVATE
  aui_qt
  client_common
)

if("wt" IN_LIST CLIENT_UI_CONFIGS)
  add_library(client_ui_dragdrop_wt
    "${CMAKE_CURRENT_SOURCE_DIR}/../ui/dragdrop/item_drag_data.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ui/dragdrop/item_drag_data.h"
  )
  target_precompile_headers(client_ui_dragdrop_wt REUSE_FROM scada_base)
  target_link_libraries(client_ui_dragdrop_wt PRIVATE
    aui_wt
    client_common
  )
endif()

add_library(client_ui_progress_qt
  "${CMAKE_CURRENT_SOURCE_DIR}/../modules/node_service_progress_tracker/node_service_progress_tracker.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../modules/node_service_progress_tracker/node_service_progress_tracker.h"
)
target_precompile_headers(client_ui_progress_qt REUSE_FROM scada_base)
target_link_libraries(client_ui_progress_qt PRIVATE
  client_common
)

if("wt" IN_LIST CLIENT_UI_CONFIGS)
  add_library(client_ui_progress_wt
    "${CMAKE_CURRENT_SOURCE_DIR}/../modules/node_service_progress_tracker/node_service_progress_tracker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../modules/node_service_progress_tracker/node_service_progress_tracker.h"
  )
  target_precompile_headers(client_ui_progress_wt REUSE_FROM scada_base)
  target_link_libraries(client_ui_progress_wt PRIVATE
    client_common
  )
endif()

target_sources(client_base_unittests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../ui/common/client_utils_unittest.cpp"
)
target_link_libraries(client_base_unittests PRIVATE
  client_ui_common_qt
  node_service
)
