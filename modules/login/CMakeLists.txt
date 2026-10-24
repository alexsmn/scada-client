client_component_module(client_component_login)

# The login controller keeps the client's OPC UA PKI (opcua::ApplicationPki):
# its own application instance certificate and the server certificates the
# user has trusted. opcuapp's include directory must precede the common/ tree
# so `opcua/...` resolves to opcuapp (see app/qt/CMakeLists.txt).
find_package(opcuapp REQUIRED)
target_include_directories(client_component_login_qt BEFORE PRIVATE
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(client_component_login_qt PRIVATE opcuapp)

if(TARGET client_component_login_qt_unittests)
  target_include_directories(client_component_login_qt_unittests BEFORE PRIVATE
    $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
  # scada_common for common/test/scoped_temp_dir.h.
  target_link_libraries(client_component_login_qt_unittests PRIVATE
    opcuapp
    scada_common
  )
endif()
