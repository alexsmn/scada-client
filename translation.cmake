if(NOT Qt6LinguistTools_FOUND)
  find_package(Qt6 COMPONENTS LinguistTools QUIET)
endif()

set(CLIENT_QT_TRANSLATIONS "" CACHE INTERNAL "QT translations")

macro(client_qt_translations TARGET)
  message("client_qt_translations(${TARGET} ${ARGN})")
  if(COMMAND qt_create_translation)
    qt_create_translation(TRANSLATIONS_OUT ${TARGET_SOURCES} ${TRANSLATIONS})
  endif()
  set(CLIENT_QT_TRANSLATIONS ${CLIENT_QT_TRANSLATIONS} ${ARGN} CACHE INTERNAL "QT translation")
endmacro()

function(client_qt_translations_build)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  if(COMMAND qt_create_translation)
    qt_create_translation(TRANSLATIONS_OUT ${TARGET_SOURCES} ${TRANSLATIONS})
  endif()
  if(COMMAND qt_add_translation)
    # qt_add_translation(TRANSLATIONS_OUT ${CLIENT_QT_TRANSLATIONS})
    add_custom_command(
      OUTPUT "client.ts"
      DEPENDS ${CLIENT_QT_TRANSLATIONS}
      COMMAND "lconvert -i ${CLIENT_QT_TRANSLATIONS} -o client.ts"
      COMMENT "Combine .ts files into client.ts")

    qt_add_translation(TRANSLATIONS_OUT ${CLIENT_QT_TRANSLATIONS})
  endif()
endfunction()

#[[
function(client_qt_translations TARGET)
  set(TRANSLATIONS ${ARGN})
  message("client_qt_translations(${TARGET} ${TRANSLATIONS})")
  find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  message("client_qt_translations(${TARGET} ${TRANSLATIONS}) = ${TARGET_SOURCES}")
  qt_create_translation(TRANSLATIONS_OUT ${TARGET_SOURCES} ${TRANSLATIONS})
  qt_add_translation(TRANSLATIONS_OUT ${TRANSLATIONS})
  target_sources(${TARGET} PRIVATE ${TRANSLATIONS} ${TRANSLATIONS_OUT})
endfunction()
]]
