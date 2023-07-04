macro(client_module MODULE_NAME)
  cmake_parse_arguments(ARG "" "" "UI" ${ARGN})

  message("client_module(${MODULE_NAME})")

  file(GLOB_RECURSE ${MODULE_NAME}_PATHS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.rc"
    "${CMAKE_CURRENT_LIST_DIR}/*.ui"
  )

  foreach(PATH ${${MODULE_NAME}_PATHS})
    IF(${PATH} MATCHES "_unittest" OR ${PATH} MATCHES "/test/")
      IF(${PATH} MATCHES "/qt/")
        LIST(APPEND ${MODULE_NAME}_UNITTESTS_QT ${PATH})
      ELSEIF(${PATH} MATCHES "/wt/")
        LIST(APPEND ${MODULE_NAME}_UNITTESTS_WT ${PATH})
      ELSE()
        LIST(APPEND ${MODULE_NAME}_UNITTESTS ${PATH})
      ENDIF()
    ELSE()
      IF(${PATH} MATCHES "/qt/")
        LIST(APPEND ${MODULE_NAME}_SOURCES_QT ${PATH})
      ELSEIF(${PATH} MATCHES "/wt/")
        LIST(APPEND ${MODULE_NAME}_SOURCES_WT ${PATH})
      ELSE()
        LIST(APPEND ${MODULE_NAME}_SOURCES ${PATH})
      ENDIF()
    ENDIF()
  endforeach()

  add_library(${MODULE_NAME}_qt
    ${${MODULE_NAME}_SOURCES}
    ${${MODULE_NAME}_SOURCES_QT})

  add_library(${MODULE_NAME}_wt
    ${${MODULE_NAME}_SOURCES}
    ${${MODULE_NAME}_SOURCES_WT})

  # UTs.

  scada_module_unittests(${MODULE_NAME}_qt SOURCES ${${MODULE_NAME}_UNITTESTS} ${${MODULE_NAME}_UNITTESTS_QT})
  scada_module_unittests(${MODULE_NAME}_wt SOURCES ${${MODULE_NAME}_UNITTESTS} ${${MODULE_NAME}_UNITTESTS_WT})

endmacro()

macro(client_module_link MODULE_NAME LINK_TYPE)
  message("client_module_link(${MODULE_NAME} ${LINK_TYPE} ${ARGN})")

  foreach(TARGET ${ARGN})
    if(TARGET ${TARGET})
      list(APPEND ${MODULE_NAME}_${LINK_TYPE}_LINK ${TARGET})
    endif()
    if(TARGET ${TARGET}_qt)
      list(APPEND ${MODULE_NAME}_QT_${LINK_TYPE}_LINK ${TARGET}_qt)
    endif()
    if(TARGET ${TARGET}_wt)
      list(APPEND ${MODULE_NAME}_WT_${LINK_TYPE}_LINK ${TARGET}_wt)
    endif()
  endforeach()

  target_link_libraries(${MODULE_NAME}_qt ${LINK_TYPE}
    ${${MODULE_NAME}_${LINK_TYPE}_LINK}
    ${${MODULE_NAME}_QT_${LINK_TYPE}_LINK})

  target_link_libraries(${MODULE_NAME}_wt ${LINK_TYPE}
    ${${MODULE_NAME}_${LINK_TYPE}_LINK}
    ${${MODULE_NAME}_WT_${LINK_TYPE}_LINK})
endmacro()
