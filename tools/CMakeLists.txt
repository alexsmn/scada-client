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

  # ...and a test for rules 7 and 8, which reach across translation units to
  # resolve a `Translate(x.label)` back to the table the member is read from,
  # and a `Translate(F(x))` back to the switch the function returns from.
  # Between them they must recognise four spellings of a table in two
  # initialiser forms, and a definition that may be qualified at its call site,
  # so a pattern that stops matching leaves them reporting OK over exactly the
  # seam task 466's eleven English strings shipped through.
  add_test(NAME client_ui_translation_check_test
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_ui_translations_test.py")
  set_tests_properties(client_ui_translation_check_test PROPERTIES TIMEOUT 120)

  # The complementary gap: a user-facing string hard-coded as a literal never
  # reaches a .ts at all, so the check above cannot see it and lupdate does not
  # recognise Translate(). Source-only, so it needs no build.
  add_test(NAME client_untranslated_string_check
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_untranslated_ui_strings.py"
      "--client-dir=${CMAKE_CURRENT_SOURCE_DIR}/..")
  set_tests_properties(client_untranslated_string_check PROPERTIES TIMEOUT 120)

  # ...and a test for that checker, because its own failure mode is silence: a
  # regex that stops matching makes it report OK over a tree full of the defect
  # rather than fail. It shipped exactly that way, matching only u"..." while
  # L"...", plain "..." at a Qt widget and raw R"(...)" blocks went unseen.
  add_test(NAME client_untranslated_string_check_test
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_untranslated_ui_strings_test.py")
  set_tests_properties(client_untranslated_string_check_test PROPERTIES TIMEOUT 120)

  # The same shape of defect one layer over: a resource path the qrc does not
  # list yields a null QIcon and a blank row, with nothing failing and nothing
  # logged. The favourites glyphs shipped that way once. Source-only, so it
  # needs no build and can cover aui/, which must not take on a client
  # dependency for a unit test.
  add_test(NAME client_qrc_resource_check
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_qrc_resources.py"
      "--client-dir=${CMAKE_CURRENT_SOURCE_DIR}/..")
  set_tests_properties(client_qrc_resource_check PROPERTIES TIMEOUT 120)
endif()
