file(GLOB MAIN_WT CONFIGURE_DEPENDS
  "../client_application.cpp"
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_WT ${MAIN_WT})

list(FILTER SOURCES EXCLUDE REGEX "/graph/")
list(FILTER UNITTESTS EXCLUDE REGEX "/graph/")
list(FILTER SOURCES EXCLUDE REGEX "/modus/")
list(FILTER SOURCES EXCLUDE REGEX "/vidicon/")
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

find_package(wt REQUIRED)
target_link_libraries(client_wt_lib PUBLIC
  aui_wt
  client_common
  client_configuration_tree_wt
  client_controller_wt
  client_export_configuration_wt
  client_export_csv_wt
  client_filesystem_wt
  client_main_window_wt
  client_portfolio_wt
  client_print_preview_wt
  client_properties_wt
  client_services_wt
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

set_property(TARGET client_wt PROPERTY OUTPUT_NAME "webserver")

# UT-s

add_executable(client_wt_unittests ${UNITTESTS} ${UNITTESTS_WT})
# Uses the custom runner from `test/wt/test_main_wt.cpp`.
target_link_libraries(client_wt_unittests PUBLIC client_wt_lib base_unittest Wt::Test)
include(GoogleTest)
gtest_discover_tests(client_wt_unittests)
