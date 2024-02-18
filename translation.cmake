find_package(Qt5LinguistTools REQUIRED)

set(CLIENT_QT_TRANSLATIONS "" CACHE INTERNAL "QT translations")

macro(client_qt_translations TARGET)
  message("client_qt_translations(${TARGET} ${ARGN})")
  qt5_create_translation(TRANSLATIONS_OUT ${TARGET_SOURCES} ${TRANSLATIONS})
  set(CLIENT_QT_TRANSLATIONS ${CLIENT_QT_TRANSLATIONS} ${ARGN} CACHE INTERNAL "QT translation")
endmacro()

function(client_qt_translations_build)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  qt5_create_translation(TRANSLATIONS_OUT ${TARGET_SOURCES} ${TRANSLATIONS})
  # qt5_add_translation(TRANSLATIONS_OUT ${CLIENT_QT_TRANSLATIONS})
  add_custom_command(
    OUTPUT "client.ts"
    DEPENDS ${CLIENT_QT_TRANSLATIONS}
    COMMAND "lconvert -i ${CLIENT_QT_TRANSLATIONS} -o client.ts"
    COMMENT "Combine .ts files into client.ts")

  qt5_add_translation(TRANSLATIONS_OUT ${CLIENT_QT_TRANSLATIONS})
endfunction()

#[[
function(client_qt_translations TARGET)
  set(TRANSLATIONS ${ARGN})
  message("client_qt_translations(${TARGET} ${TRANSLATIONS})")
  find_package(Qt5LinguistTools REQUIRED)
  get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
  message("client_qt_translations(${TARGET} ${TRANSLATIONS}) = ${TARGET_SOURCES}")
  qt5_create_translation(TRANSLATIONS_OUT ${TARGET_SOURCES} ${TRANSLATIONS})
  qt5_add_translation(TRANSLATIONS_OUT ${TRANSLATIONS})
  target_sources(${TARGET} PRIVATE ${TRANSLATIONS} ${TRANSLATIONS_OUT})
endfunction()
]]
