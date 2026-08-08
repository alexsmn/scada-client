add_subdirectory(screenshot_generator)

# Guards against a .ui string's translation drifting into the wrong context (or
# being marked vanished) and silently no longer shipping, which leaves the
# dialog in English while the .ts still looks translated. Needs lupdate; the
# script degrades to a skip without it, matching the build's own handling of a
# missing Qt LinguistTools.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME client_ui_translation_check
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_ui_translations.py"
      "--client-dir=${CMAKE_CURRENT_SOURCE_DIR}/..")
  set_tests_properties(client_ui_translation_check PROPERTIES TIMEOUT 120)

  # The complementary gap: a user-facing string hard-coded as a u"..." literal
  # never reaches a .ts at all, so the check above cannot see it and lupdate
  # does not recognise Translate(). Source-only, so it needs no build.
  add_test(NAME client_untranslated_string_check
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_untranslated_ui_strings.py"
      "--client-dir=${CMAKE_CURRENT_SOURCE_DIR}/..")
  set_tests_properties(client_untranslated_string_check PROPERTIES TIMEOUT 120)
endif()
