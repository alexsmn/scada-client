# The build stamp the About dialog quotes. Generated into this module's binary
# directory rather than committed, and refreshed on EVERY build rather than at
# configure time: a stamp that is three days stale is worse than none, because
# nothing about it says so. `build_stamp.cmake` writes through
# `copy_if_different`, so a rebuild at an unchanged commit recompiles nothing.
set(_about_stamp_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(_about_stamp_header "${_about_stamp_dir}/client_build_stamp.h")
set(_about_stamp_args
  "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
  "-DTEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/build_stamp.h.in"
  "-DOUTPUT=${_about_stamp_header}")

# Once at configure time, so the header exists for the first dependency scan;
# the target below is what keeps it true afterwards.
execute_process(COMMAND "${CMAKE_COMMAND}" ${_about_stamp_args}
                        -P "${CMAKE_CURRENT_SOURCE_DIR}/build_stamp.cmake")

add_custom_target(client_about_build_stamp
  BYPRODUCTS "${_about_stamp_header}"
  COMMAND "${CMAKE_COMMAND}" ${_about_stamp_args}
          -P "${CMAKE_CURRENT_SOURCE_DIR}/build_stamp.cmake"
  COMMENT "Stamping the client build id")

client_component_module(client_component_about)

add_dependencies(client_component_about_qt client_about_build_stamp)
target_include_directories(client_component_about_qt PRIVATE "${_about_stamp_dir}")

if(TARGET client_component_about_qt_unittests)
  # The show-dialog regression test needs a QApplication (aui_test_qt);
  # AboutDialog itself pulls LoadPixmap from client_ui_common.
  target_link_libraries(client_component_about_qt_unittests PRIVATE
    aui_test_qt
    client_ui_common_qt
  )
  target_include_directories(client_component_about_qt_unittests PRIVATE
    "${_about_stamp_dir}")
endif()
