find_package(wt CONFIG REQUIRED)

file(GLOB MAIN_WT CONFIGURE_DEPENDS
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_WT ${MAIN_WT})

list(FILTER SOURCES EXCLUDE REGEX "/graph/")
list(FILTER SOURCES EXCLUDE REGEX "/modus/")
list(FILTER SOURCES EXCLUDE REGEX "/vidicon_display/")
list(FILTER SOURCES EXCLUDE REGEX "/web/")

add_executable(client_wt ${SOURCES} ${SOURCES_WT} ${MAIN_WT})
set_property(TARGET client_wt PROPERTY FOLDER ${client_folder})
target_link_libraries(client_wt PRIVATE
  Wt::Wt
  Wt::HTTP
  address_space_node_service
  remote_node_service
  scada_core_opcua
  scada_core_remote
  scada_core_vidicon
  timed_data
  ui_base
)
# target_link_libraries(client_wt PRIVATE Wt::Wt Wt::HTTP Wt::Isapi)

target_compile_definitions(client_wt PUBLIC
  "-DUI_WT"
)
