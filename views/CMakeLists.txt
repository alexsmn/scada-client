LIST(APPEND sources ${sources_views})
LIST(APPEND unittests ${unittests_views})

LIST(APPEND libs
  views
)

MESSAGE("Client UI Resources: ${ui_resources}")

ADD_EXECUTABLE(client_views WIN32
  ${sources}
  ${views_sources}
  ${ui_resources}
)

TARGET_LINK_LIBRARIES(client_views ${libs})

SET_PROPERTY(TARGET client_views PROPERTY OUTPUT_NAME "client")

SET_PROPERTY(TARGET client_views APPEND PROPERTY LINK_FLAGS "-MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

SET_PROPERTY(TARGET client_views APPEND PROPERTY COMPILE_FLAGS "-DUI_VIEWS")
SET_PROPERTY(TARGET client_views PROPERTY FOLDER ${client_folder})

INSTALL(TARGETS client_views
  RUNTIME DESTINATION bin
  COMPONENT Client)
