scada_module(client_core)

target_precompile_headers(client_core REUSE_FROM scada_base)

target_link_libraries(client_core PUBLIC scada_base)

# The client facade set is Qt-flavored (see docs/ops/client-cxx-modules.md), and this
# facade in particular cannot exist without the Qt UI config:
# node_command_context.h pulls in aui/key_codes.h, which needs UI_QT and the
# Qt include dirs. aui_qt publishes both (client_core itself leaves them to
# its UI consumers).
if(BUILD_CLIENT_QT)
  scada_add_module_facade(client_core FILES scada_client_core.cppm
    IMPORTS scada_base_module)
  if(SCADA_CXX_MODULES)
    target_link_libraries(client_core_module PRIVATE aui_qt)
    add_subdirectory(module_test)
  endif()
endif()
