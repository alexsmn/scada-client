if(NOT TARGET aui_qt AND NOT TARGET aui_wt)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR} scada-client-aui)
endif()
