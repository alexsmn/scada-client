scada_module(client_core)

target_precompile_headers(client_core REUSE_FROM scada_base)

target_link_libraries(client_core PUBLIC scada_base)

scada_add_module_facade(client_core FILES scada_client_core.cppm
  IMPORTS scada_base_module)
if(SCADA_CXX_MODULES)
  # The client facade set is Qt-flavored: node_command_context.h pulls in
  # aui/key_codes.h, whose contents exist only under UI_QT/UI_WT (UI_QT is
  # normally a PUBLIC define of aui_qt, which client_core does not link).
  target_compile_definitions(client_core_module PRIVATE UI_QT)
  add_subdirectory(module_test)
endif()
