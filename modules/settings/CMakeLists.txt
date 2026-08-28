client_module(client_settings CONFIG qt)

# Export the modules/ root so the shell can include "settings/qt/...".
client_module_include_directories(client_settings PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_settings
  PUBLIC
    Qt6::Widgets
  PRIVATE
    # MenuModel, Translate and FoldForSearch -- the whole dependency set. The
    # preferences themselves live in the profile and in QSettings, and the panel
    # reaches both through the commands the menu model carries, so this module
    # links neither, and it knows nothing about the shell it covers.
    aui
)

# The tests drive the catalogue directly and the panel through a real
# SimpleMenuModel, so the binary needs aui + a QApplication (aui_test_qt).
if(TARGET client_settings_qt_unittests)
  target_link_libraries(client_settings_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
