client_module(client_vidicon_display_native CONFIG qt)

client_module_link_libraries(client_vidicon_display_native PUBLIC
  client_controller
)

client_module_link_libraries(client_vidicon_display_native PRIVATE
  client_filesystem
  client_vidicon_teleclient
  scada_common_vidicon
)

target_link_libraries(client_vidicon_display_native_qt_unittests PRIVATE
  scada_common_vidicon
)

if(EXISTS "${CMAKE_SOURCE_DIR}/dependencies/bin/DisplayLib.dll")
  add_custom_command(TARGET client_vidicon_display_native_qt_unittests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/dependencies/bin/DisplayLib.dll"
      "$<TARGET_FILE_DIR:client_vidicon_display_native_qt_unittests>"
  )
endif()
