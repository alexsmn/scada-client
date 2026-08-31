#include "settings/settings_catalog.h"

#include "aui/models/menu_model.h"
#include "aui/models/simple_menu_model.h"
#include "main_window/standard_command_ids.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using scada::aui::MenuModel;
using scada::aui::SimpleMenuModel;

// Stands in for the command registry the real Settings menu is assembled from:
// it answers the checked/enabled/visible queries and records what a row
// activated. A real `SimpleMenuModel` is driven on top of it rather than a mock
// menu, so the catalogue is exercised against the same model class the shell
// builds -- which is the whole reason it can join to one.
class FakeDelegate : public SimpleMenuModel::Delegate {
 public:
  bool IsCommandIdChecked(int command_id) const override {
    return checked_.contains(command_id);
  }
  bool IsCommandIdEnabled(int command_id) const override {
    return !disabled_.contains(command_id);
  }
  bool IsCommandIdVisible(int command_id) const override {
    return !hidden_.contains(command_id);
  }
  void ExecuteCommand(int command_id) override {
    executed_.push_back(command_id);
    // The real option commands toggle, so the model's answer changes under the
    // caller exactly as it would in the shell.
    if (!checked_.erase(command_id))
      checked_.insert(command_id);
  }

  void SetChecked(int command_id) { checked_.insert(command_id); }
  void Disable(int command_id) { disabled_.insert(command_id); }
  void Hide(int command_id) { hidden_.insert(command_id); }
  const std::vector<int>& executed() const { return executed_; }

 private:
  std::set<int> checked_;
  std::set<int> disabled_;
  std::set<int> hidden_;
  std::vector<int> executed_;
};

// The `MainMenuId::Settings` model as `MainMenuModel::Rebuild` assembles it:
// the two window toggles, the module contributions in their registered order,
// the Modus pair, and the three choice submenus. Same commands, same order,
// same separators.
//
// A hand-built mirror rather than the real menu, because standing up
// `MainMenuModel` needs the whole shell -- but the mirror is what
// `CatalogDescribesEverySettingsMenuCommand` checks, so a drift between it and
// the shell shows up as an undescribed command rather than as a silent pass.
class SettingsMenuFixture : public ::testing::Test {
 protected:
  SettingsMenuFixture() {
    settings_.AddCheckItem(ID_VIEW_TOOLBAR, u"Toolbar");
    settings_.AddCheckItem(ID_VIEW_STATUS_BAR, u"Status Bar");

    settings_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    settings_.AddCheckItem(ID_WRITE_CONFIRMATION, u"Control Confirmation");
    settings_.AddCheckItem(ID_SHOW_WRITEOK, u"Control Success Message");
    settings_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    settings_.AddCheckItem(ID_SHOW_EVENTS, u"Show Events on Arrival");
    settings_.AddCheckItem(ID_HIDE_EVENTS, u"Hide Events on Acknowledge");
    settings_.AddCheckItem(ID_EVENT_FLASH_WINDOW,
                           u"Flash Main Window on Event");
    settings_.AddCheckItem(ID_EVENT_PLAY_SOUND, u"Sound Alarm on Event");
    settings_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    settings_.AddItem(ID_VIEW_PUBLIC_FOLDER, u"Open Displays Folder");

    settings_.AddCheckItem(ID_MODUS_TOPOLOGY, u"Show Modus topology");
    settings_.AddCheckItem(ID_MODUS_RUNTIME_RENDERER,
                           u"Use Modus runtime renderer");

    language_.AddCheckItem(ID_LANGUAGE_ENGLISH, u"English");
    language_.AddCheckItem(ID_LANGUAGE_RUSSIAN, u"Russian");
    style_.AddCheckItem(kStyleFusion, u"Fusion");
    style_.AddCheckItem(kStyleMacos, u"macOS");
    appearance_.AddCheckItem(kAppearanceSystem, u"Follow system");
    appearance_.AddCheckItem(kAppearanceDark, u"Dark");

    settings_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    settings_.AddSubMenu(ID_SETTINGS_LANGUAGE, u"Language", &language_);
    settings_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    settings_.AddSubMenu(ID_SETTINGS_STYLE, u"Style", &style_);
    settings_.AddSubMenu(ID_SETTINGS_APPEARANCE, u"Colour scheme",
                         &appearance_);
  }

  // Ids for the submenu options. They are dynamic in the shell (the style list
  // comes from QStyleFactory), so any distinct value does here.
  static constexpr int kStyleFusion = 90001;
  static constexpr int kStyleMacos = 90002;
  static constexpr int kAppearanceSystem = 90011;
  static constexpr int kAppearanceDark = 90012;

  std::vector<SettingRow> Catalog() { return BuildSettingsCatalog(settings_); }

  // Nullable, for the tests that assert a row is *absent*.
  const SettingRow* Find(const std::vector<SettingRow>& rows,
                         std::string_view id) {
    auto i = std::ranges::find(rows, id, &SettingRow::id);
    return i == rows.end() ? nullptr : &*i;
  }

  // For the tests that read a row. Dereferencing `Find` directly would abort
  // the whole executable on a miss, and every test after it would report
  // nothing at all -- no failure line, no summary, which reads as if those
  // tests do not exist rather than as if they failed. That is a different
  // silence from a vacuous pass and a worse one, and it is what an empty
  // catalogue used to produce here. Throwing instead gives gtest a failure it
  // can attribute and lets the rest of the suite run.
  const SettingRow& Require(const std::vector<SettingRow>& rows,
                            std::string_view id) {
    const SettingRow* row = Find(rows, id);
    if (!row)
      throw std::runtime_error("no catalogue row " + std::string{id});
    return *row;
  }

  std::vector<std::string> Ids(const std::vector<SettingRow>& rows) {
    std::vector<std::string> ids;
    for (const SettingRow& row : rows)
      ids.emplace_back(row.id);
    return ids;
  }

  FakeDelegate delegate_;
  SimpleMenuModel settings_{&delegate_};
  SimpleMenuModel language_{&delegate_};
  SimpleMenuModel style_{&delegate_};
  SimpleMenuModel appearance_{&delegate_};
};

// **What else covers this file, measured rather than assumed.** Emptying
// `BuildSettingsCatalog` and running the whole client suite fails 25 tests here
// and one elsewhere: `ScreenshotGenerator.CaptureSettingsPanel`, whose content
// assertions (a search field, more than one category, more than one scope tab,
// an action row) reject an empty surface. That capture is the **only** exercise
// of this catalogue against the real `MainMenuModel` and the real commands --
// everything below drives a `SimpleMenuModel` over `FakeDelegate`. After a fake
// in this very file was found reporting radio rows as independent toggles, that
// distinction is worth keeping: a fake can be wrong in a way no test built on
// it can see, and the capture is the guard that does not share the assumption.
//
// Three tests here correctly do NOT fail on an empty catalogue, and should not
// be "fixed" to: `CatalogDescribesEverySettingsMenuCommand` asks the opposite
// question (does the menu publish anything the catalogue misses), and the
// panel's two overlay-mechanics tests are about geometry and dismissal, which
// hold for a surface with no rows on it.

// The contract in the direction that matters most: a preference a module adds
// to the Settings menu reaches the operator only once the catalogue describes
// it, so an undescribed command is a row that would silently not exist.
TEST_F(SettingsMenuFixture, CatalogDescribesEverySettingsMenuCommand) {
  EXPECT_TRUE(UndescribedSettingsCommands(settings_).empty());
}

// ... and the same check has to be able to fail, or it says nothing. A module
// contributing a new toggle is exactly this shape.
TEST_F(SettingsMenuFixture, AnUndescribedContributionIsReported) {
  constexpr int kNewContribution = 91234;
  settings_.AddCheckItem(kNewContribution, u"Some New Preference");

  EXPECT_EQ(UndescribedSettingsCommands(settings_),
            (std::vector<unsigned>{kNewContribution}));
  // And it draws no row, rather than one with no description or store.
  EXPECT_EQ(Catalog().size(), SettingsCatalogCommandIds().size());
}

// Screen order, not menu order. The menu lists Style before Colour scheme; the
// shared screen lists Colour scheme before Widget style, and the catalogue is
// what decides.
TEST_F(SettingsMenuFixture, RowsAreInScreenOrderNotMenuOrder) {
  EXPECT_EQ(
      Ids(Catalog()),
      (std::vector<std::string>{
          "language", "colour-scheme", "widget-style", "sound-on-events",
          "show-events", "hide-events", "flash-window", "control-confirmation",
          "control-success", "toolbar", "status-bar", "modus-topology",
          "modus-renderer", "open-displays-folder"}));
}

// A row's title is the command's own, so the surface and the menu cannot say
// different things about the same preference.
TEST_F(SettingsMenuFixture, TitlesComeFromTheMenuRatherThanTheCatalog) {
  const std::vector<SettingRow> rows = Catalog();
  EXPECT_EQ(Require(rows, "flash-window").title, u"Flash Main Window on Event");
  EXPECT_EQ(Require(rows, "colour-scheme").title, u"Colour scheme");
}

// Every row carries a sentence. A preference whose effect cannot be stated is
// one the operator cannot make a decision about.
TEST_F(SettingsMenuFixture, EveryRowCarriesADescription) {
  const std::vector<SettingRow> rows = Catalog();
  // A per-row assertion in a loop reports nothing when there are no rows, so
  // this test would be green against a catalogue that came back empty -- the
  // failure it is least able to notice and the one that would matter most.
  ASSERT_EQ(rows.size(), SettingsCatalogCommandIds().size());

  for (const SettingRow& row : rows)
    EXPECT_FALSE(row.description.empty()) << row.id;
}

// The storage scope is the question a two-client workbench has to answer, and
// the three stores really do differ: the appearance choice is machine-local,
// the event options follow the account, and the window chrome is per window.
TEST_F(SettingsMenuFixture, ScopesMatchWhereTheValueIsActuallyStored) {
  const std::vector<SettingRow> rows = Catalog();
  EXPECT_EQ(Require(rows, "widget-style").scope, SettingScope::kClient);
  EXPECT_EQ(Require(rows, "sound-on-events").scope, SettingScope::kProfile);
  EXPECT_EQ(Require(rows, "toolbar").scope, SettingScope::kWindow);
  EXPECT_EQ(Require(rows, "status-bar").scope, SettingScope::kWindow);
  EXPECT_EQ(Require(rows, "open-displays-folder").scope, SettingScope::kAction);
}

// Task 554's other half. `Open Displays Folder` is a command, which the
// preferences dialog skipped on purpose because a command drawn as a checkbox
// misreports it; the surface has room to keep it as a button in the category it
// belongs to.
TEST_F(SettingsMenuFixture, TheDisplaysActionHasAPlace) {
  const std::vector<SettingRow> rows = Catalog();
  const SettingRow& action = Require(rows, "open-displays-folder");
  EXPECT_EQ(action.category, SettingCategory::kDisplays);
  EXPECT_EQ(action.control, SettingControl::kAction);

  // And activating it runs the command rather than toggling anything.
  action.model->ActivatedAt(action.index);
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{ID_VIEW_PUBLIC_FOLDER}));
}

// A build without the Modus module publishes neither Modus command, and the
// Displays category then holds the action alone. Absence, not a disabled row.
TEST_F(SettingsMenuFixture, ACommandTheShellDoesNotPublishHasNoRow) {
  SimpleMenuModel without_modus{&delegate_};
  without_modus.AddCheckItem(ID_VIEW_TOOLBAR, u"Toolbar");
  without_modus.AddItem(ID_VIEW_PUBLIC_FOLDER, u"Open Displays Folder");

  EXPECT_EQ(Ids(BuildSettingsCatalog(without_modus)),
            (std::vector<std::string>{"toolbar", "open-displays-folder"}));
}

// An invisible item is invisible here too -- which is how an admin-gated
// contribution stays hidden rather than becoming a control the shell refuses.
TEST_F(SettingsMenuFixture, AHiddenItemDrawsNoRow) {
  delegate_.Hide(ID_EVENT_FLASH_WINDOW);
  // Falsifiable because `TitlesComeFromTheMenuRatherThanTheCatalog` asserts the
  // same `Find` call on the same id is non-null with the row visible: a lookup
  // that could never find anything would fail there rather than passing here.
  EXPECT_EQ(Find(Catalog(), "flash-window"), nullptr);
}

TEST_F(SettingsMenuFixture, CategoriesGroupInScreenOrderAndDropEmptyOnes) {
  const std::vector<SettingCategoryGroup> groups = GroupSettingRows(Catalog());

  std::vector<SettingCategory> categories;
  std::vector<size_t> sizes;
  for (const SettingCategoryGroup& group : groups) {
    categories.push_back(group.category);
    sizes.push_back(group.rows.size());
  }
  EXPECT_EQ(categories,
            (std::vector<SettingCategory>{
                SettingCategory::kAppearance, SettingCategory::kEventsAlarms,
                SettingCategory::kControl, SettingCategory::kWorkspace,
                SettingCategory::kDisplays}));
  EXPECT_EQ(sizes, (std::vector<size_t>{3, 4, 2, 2, 3}));

  // A category with nothing left in it is a table-of-contents entry that
  // scrolls nowhere, so it is dropped rather than drawn empty.
  const std::vector<SettingRow> window_only =
      FilterSettingRows(Catalog(), u"", SettingScope::kWindow);
  const std::vector<SettingCategoryGroup> narrowed =
      GroupSettingRows(window_only);
  ASSERT_EQ(narrowed.size(), 1u);
  EXPECT_EQ(narrowed.front().category, SettingCategory::kWorkspace);
}

TEST_F(SettingsMenuFixture, SearchMatchesTitleDescriptionAndCategory) {
  const std::vector<SettingRow> rows = Catalog();

  // Title.
  EXPECT_EQ(Ids(FilterSettingRows(rows, u"topology", std::nullopt)),
            (std::vector<std::string>{"modus-topology"}));
  // Description -- "taskbar" appears in no title.
  EXPECT_EQ(Ids(FilterSettingRows(rows, u"taskbar", std::nullopt)),
            (std::vector<std::string>{"flash-window"}));
  // Category name, which is how an operator looks for a display setting whose
  // title never says the word.
  EXPECT_EQ(Ids(FilterSettingRows(rows, u"displays", std::nullopt)),
            (std::vector<std::string>{"modus-topology", "modus-renderer",
                                      "open-displays-folder"}));
}

TEST_F(SettingsMenuFixture, SearchIsCaseInsensitiveAndOrderIndependent) {
  const std::vector<SettingRow> rows = Catalog();
  EXPECT_EQ(Ids(FilterSettingRows(rows, u"MODUS runtime", std::nullopt)),
            (std::vector<std::string>{"modus-renderer"}));
  EXPECT_EQ(Ids(FilterSettingRows(rows, u"runtime modus", std::nullopt)),
            (std::vector<std::string>{"modus-renderer"}));
  // Every term has to match, so one that matches nothing rules the row out.
  EXPECT_TRUE(FilterSettingRows(rows, u"modus nonesuch", std::nullopt).empty());
  // An empty query is not a filter.
  EXPECT_EQ(FilterSettingRows(rows, u"   ", std::nullopt).size(), rows.size());
}

TEST_F(SettingsMenuFixture, ScopeTabsOfferOnlyScopesSomeRowUses) {
  EXPECT_EQ(
      VisibleSettingScopes(Catalog()),
      (std::vector<SettingScope>{SettingScope::kClient, SettingScope::kProfile,
                                 SettingScope::kWindow}));

  // A tab that can only ever come back empty is worse than no tab: with the
  // choice rows gone there is no machine-local setting left to offer one for.
  SimpleMenuModel profile_only{&delegate_};
  profile_only.AddCheckItem(ID_EVENT_PLAY_SOUND, u"Sound Alarm on Event");
  EXPECT_EQ(VisibleSettingScopes(BuildSettingsCatalog(profile_only)),
            (std::vector<SettingScope>{SettingScope::kProfile}));
}

// `kAction` is not a store, so it is never a tab and is counted apart from the
// settings. A single total would report a button as a preference the operator
// holds.
TEST_F(SettingsMenuFixture, ActionsAreCountedApartAndAreNeverAScopeTab) {
  const SettingRowCounts counts = CountSettingRows(Catalog());
  EXPECT_EQ(counts.settings, 13u);
  EXPECT_EQ(counts.actions, 1u);

  EXPECT_EQ(std::ranges::count(SettingStorageScopes(), SettingScope::kAction),
            0);
}

}  // namespace
