#include "settings/settings_catalog.h"

#include "main_window/standard_command_ids.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

namespace {

// The catalogue restates each toggle's default because the menu model cannot
// say where a command started (backlog 1012). This is what keeps that copy
// honest: every value is read back from the struct that actually owns it, so a
// changed `Profile` initializer fails here rather than leaving the `Modified`
// badge lit on a fresh profile.
TEST(SettingsCatalogDefaultsTest, EveryDefaultMatchesItsOwner) {
  const Profile profile;
  const MainWindowDef window;

  EXPECT_EQ(SettingDefaultChecked(ID_SHOW_WRITEOK), profile.show_write_ok);
  EXPECT_EQ(SettingDefaultChecked(ID_SHOW_EVENTS), profile.event_auto_show);
  EXPECT_EQ(SettingDefaultChecked(ID_HIDE_EVENTS), profile.event_auto_hide);
  EXPECT_EQ(SettingDefaultChecked(ID_EVENT_FLASH_WINDOW),
            profile.event_flash_window);
  EXPECT_EQ(SettingDefaultChecked(ID_EVENT_PLAY_SOUND),
            profile.event_play_sound);
  EXPECT_EQ(SettingDefaultChecked(ID_OPT_SPEECH), profile.speech_enabled);
  EXPECT_EQ(SettingDefaultChecked(ID_WRITE_CONFIRMATION),
            profile.control_confirmation);
  EXPECT_EQ(SettingDefaultChecked(ID_MODUS_TOPOLOGY), profile.modus.topology);
  EXPECT_EQ(SettingDefaultChecked(ID_VIEW_STATUS_BAR), window.status_bar);
}

// Every described toggle is covered above. Counting them here means a toggle
// added to the catalogue without a pinned default fails until it is added to
// the test as well.
TEST(SettingsCatalogDefaultsTest, EveryToggleWithADefaultIsPinned) {
  int with_default = 0;
  for (unsigned command_id : SettingsCatalogCommandIds()) {
    if (SettingDefaultChecked(command_id))
      ++with_default;
  }
  EXPECT_EQ(with_default, 9);
}

}  // namespace
