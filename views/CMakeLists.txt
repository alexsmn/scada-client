message("Client UI Resources: ${ui_resources}")

add_executable(client_views WIN32
  ${SOURCES}
  ${SOURCES_VIEWS}
  ${ui_resources}
)

target_include_directories(client_views PUBLIC
  "framework"
)

target_link_libraries(client_views PUBLIC
  client_core
  views
)

SET_PROPERTY(TARGET client_views PROPERTY OUTPUT_NAME "client")

SET_PROPERTY(TARGET client_views APPEND PROPERTY LINK_FLAGS "-MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

SET_PROPERTY(TARGET client_views APPEND PROPERTY COMPILE_FLAGS "-DUI_VIEWS")
SET_PROPERTY(TARGET client_views PROPERTY FOLDER ${client_folder})

INSTALL(TARGETS client_views
  RUNTIME DESTINATION bin
  COMPONENT Client)
