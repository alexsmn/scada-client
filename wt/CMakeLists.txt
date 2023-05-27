file(GLOB MAIN_WT CONFIGURE_DEPENDS
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_WT ${MAIN_WT})

list(FILTER SOURCES EXCLUDE REGEX "/graph/")
list(FILTER UNITTESTS EXCLUDE REGEX "/graph/")
list(FILTER SOURCES EXCLUDE REGEX "/modus/")
list(FILTER SOURCES EXCLUDE REGEX "/vidicon_display/")
list(FILTER SOURCES EXCLUDE REGEX "/web/")

# Lib

set(COMPONENTS ${SOURCES} ${SOURCES_QT})
list(FILTER COMPONENTS INCLUDE REGEX [[_component\..*]])
list(REMOVE_ITEM SOURCES ${COMPONENTS})
list(REMOVE_ITEM SOURCES_QT ${COMPONENTS})

add_library(client_wt_lib
  ${SOURCES}
  ${SOURCES_WT}
)

target_sources(client_wt_lib INTERFACE ${COMPONENTS})

target_compile_definitions(client_wt_lib PUBLIC
  "-DUI_WT"
)

set_property(TARGET client_wt_lib PROPERTY FOLDER ${client_folder})

find_package(wt CONFIG REQUIRED)
target_link_libraries(client_wt_lib PUBLIC
  Wt::Wt
  node_service_v1
  node_service_v2
  scada_core_opcua
  scada_core_remote
  scada_core_vidicon
  timed_data
)

target_compile_options(client_wt_lib PUBLIC
  /wd4251
  /wd4275
)

# Exe

add_executable(client_wt ${MAIN_WT})

target_link_libraries(client_wt PUBLIC
  Wt::HTTP
)

target_link_libraries(client_wt PUBLIC client_wt_lib)

set_property(TARGET client_wt PROPERTY FOLDER ${client_folder})
set_property(TARGET client_wt PROPERTY OUTPUT_NAME "webserver")

# UT-s

add_executable(client_wt_unittests ${UNITTESTS} ${UNITTESTS_WT})
# Uses the custom runner from `test/wt/test_main_wt.cpp`.
target_link_libraries(client_wt_unittests PUBLIC client_wt_lib base_unittest Wt::Test)
include(GoogleTest)
gtest_discover_tests(client_wt_unittests)
set_property(TARGET client_wt_unittests PROPERTY FOLDER ${client_folder})
