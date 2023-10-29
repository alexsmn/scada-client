#[[
  client_module(name
    [CONFIG qt wt ...]
  )
]]
function(client_module MODULE_NAME)
  cmake_parse_arguments(ARG "" "" "CONFIG" ${ARGN})

  if (NOT ARG_CONFIG)
    set(ARG_CONFIG "qt;wt")
  endif()

  foreach(CONFIG ${ARG_CONFIG})
    scada_module(${MODULE_NAME}_${CONFIG})
    scada_module_sources(${MODULE_NAME}_${CONFIG} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG}")

    if(CONFIG STREQUAL "qt")
      set_target_properties(${MODULE_NAME}_${CONFIG} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
      )
    endif()
  endforeach()
endfunction()

function(client_module_include_directories MODULE_NAME)
  set(LINK_TYPES PRIVATE PUBLIC INTERFACE)
  cmake_parse_arguments(ARG "" "" "${LINK_TYPES}" ${ARGN})

  foreach(CONFIG qt wt)
    if(TARGET ${MODULE_NAME}_${CONFIG})
      foreach(LINK_TYPE ${LINK_TYPES})
        if(ARG_${LINK_TYPE})
          target_include_directories(${MODULE_NAME}_${CONFIG} ${LINK_TYPE} ${ARG_${LINK_TYPE}})
        endif()
      endforeach()
    endif()
  endforeach()
endfunction()

#[[
  client_module_sources(my_module PUBLIC a b c)
]]
function(client_module_sources MODULE_NAME)
  set(LINK_TYPES PRIVATE PUBLIC INTERFACE)
  cmake_parse_arguments(ARG "" "" "${LINK_TYPES}" ${ARGN})

  foreach(CONFIG qt wt)
    if(TARGET ${MODULE_NAME}_${CONFIG})
      foreach(LINK_TYPE ${LINK_TYPES})
        if(ARG_${LINK_TYPE})
          foreach(DIR ${ARG_${LINK_TYPE}})
            scada_module_sources(${MODULE_NAME}_${CONFIG} ${LINK_TYPE} ${DIR} "${DIR}/${CONFIG}")
          endforeach()
        endif()
      endforeach()
    endif()
  endforeach()
endfunction()

#[[
  client_module_link_libraries_helper(my_module PUBLIC a b c)
  client_module_link_libraries_helper(my_module_qt CONFIG qt PUBLIC a b c)
]]
function(client_module_link_libraries_helper MODULE_NAME)
  set(LINK_TYPES PRIVATE PUBLIC INTERFACE)
  cmake_parse_arguments(ARG "REQUIRED" "CONFIG" "${LINK_TYPES}" ${ARGN})

  # Undefined `CONFIG` is handled properly below.

  foreach(LINK_TYPE ${LINK_TYPES})
    if(ARG_${LINK_TYPE})
      foreach(LIB ${ARG_${LINK_TYPE}})
        if(TARGET ${LIB}_${ARG_CONFIG})
          target_link_libraries(${MODULE_NAME} ${LINK_TYPE} ${LIB}_${ARG_CONFIG})
        elseif(TARGET ${LIB})
          target_link_libraries(${MODULE_NAME} ${LINK_TYPE} ${LIB})
        elseif(REQUIRED)
          message(FATAL_ERROR "Client module ${MODULE_NAME} cannot link to the required ${LINK_TYPE} library ${LIB}")
        endif()
      endforeach()
    endif()
  endforeach()
endfunction()

#[[
  client_module_link_libraries(my_module PUBLIC a b c)
]]
function(client_module_link_libraries MODULE_NAME)
  if(TARGET ${MODULE_NAME})
    client_module_link_libraries_helper(${MODULE_NAME} ${ARGN})
  endif()
  if(TARGET ${MODULE_NAME}_unittests)
    client_module_link_libraries_helper(${MODULE_NAME}_unittests ${ARGN})
  endif()

  foreach(CONFIG qt wt)
    if(TARGET ${MODULE_NAME}_${CONFIG})
      client_module_link_libraries_helper(${MODULE_NAME}_${CONFIG} CONFIG ${CONFIG} ${ARGN} REQUIRED)
    endif()
    if(TARGET ${MODULE_NAME}_${CONFIG}_unittests)
      client_module_link_libraries_helper(${MODULE_NAME}_${CONFIG}_unittests CONFIG ${CONFIG} ${ARGN} REQUIRED)
    endif()
  endforeach()
endfunction()
