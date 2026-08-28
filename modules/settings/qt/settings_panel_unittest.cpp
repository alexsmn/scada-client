#include "settings/qt/settings_panel.h"

#include "aui/models/simple_menu_model.h"
#include "aui/test/app_environment.h"

#include "main_window/standard_command_ids.h"
#include "resources/common_resources.h"

#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QTabBar>
#include <QToolButton>

#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {

using scada::aui::SimpleMenuModel;

// The same fake the catalogue tests use: a real SimpleMenuModel is driven on
// top of it, so the panel is exercised against the model class the shell
// builds.
class FakeDelegate : public SimpleMenuModel::Delegate {
 public:
  bool IsCommandIdChecked(int command_id) const override {
    return checked_.contains(command_id);
  }
  bool IsCommandIdEnabled(int command_id) const override {
    return !disabled_.contains(command_id);
  }
  std::u16string GetDisabledReasonForCommandId(int command_id) const override {
    return disabled_.contains(command_id) ? u"no reason to enable it"
                                          : std::u16string{};
  }
  void ExecuteCommand(int command_id) override {
    executed_.push_back(command_id);
    if (!checked_.erase(command_id))
      checked_.insert(command_id);
  }

  void SetChecked(int command_id) { checked_.insert(command_id); }
  void Disable(int command_id) { disabled_.insert(command_id); }
  const std::vector<int>& executed() const { return executed_; }

 private:
  std::set<int> checked_;
  std::set<int> disabled_;
  std::vector<int> executed_;
};

class SettingsPanelTest : public ::testing::Test {
 protected:
  SettingsPanelTest() {
    settings_.AddCheckItem(ID_VIEW_TOOLBAR, u"Toolbar");
    settings_.AddCheckItem(ID_VIEW_STATUS_BAR, u"Status Bar");
    settings_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    settings_.AddCheckItem(ID_EVENT_PLAY_SOUND, u"Sound Alarm on Event");
    settings_.AddCheckItem(ID_EVENT_FLASH_WINDOW,
                           u"Flash Main Window on Event");
    settings_.AddItem(ID_VIEW_PUBLIC_FOLDER, u"Open Displays Folder");

    appearance_.AddCheckItem(kClassic, u"Classic");
    appearance_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    appearance_.AddCheckItem(kDark, u"Dark");
    settings_.AddSubMenu(ID_SETTINGS_APPEARANCE, u"Colour scheme",
                         &appearance_);
  }

  static constexpr int kClassic = 90010;
  static constexpr int kDark = 90012;

  // The row widget carrying `id`, or null when the panel is not drawing it.
  // Rows are found by `objectName`, which is the catalogue's stable handle
  // rather than a translated title.
  QWidget* Row(const SettingsPanel& panel, const QString& id) {
    return panel.findChild<QWidget*>(id);
  }

  std::vector<std::string> VisibleIds(const SettingsPanel& panel) {
    std::vector<std::string> ids;
    for (const SettingRow& row : panel.visible_rows())
      ids.emplace_back(row.id);
    return ids;
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;

  FakeDelegate delegate_;
  SimpleMenuModel settings_{&delegate_};
  SimpleMenuModel appearance_{&delegate_};
};

// The four things the shared screen puts on every row, and the reason the
// catalogue exists: a menu model can carry none of them.
TEST_F(SettingsPanelTest, EveryRowCarriesATitleAChipADescriptionAndAControl) {
  SettingsPanel panel{nullptr, settings_};

  QWidget* sound = Row(panel, QStringLiteral("sound-on-events"));
  ASSERT_NE(sound, nullptr);
  // A toggle's title is its checkbox's label.
  const QList<QCheckBox*> boxes = sound->findChildren<QCheckBox*>();
  ASSERT_EQ(boxes.size(), 1);
  EXPECT_EQ(boxes.front()->text(), QStringLiteral("Sound Alarm on Event"));

  const QList<QLabel*> labels = sound->findChildren<QLabel*>();
  QStringList texts;
  for (const QLabel* label : labels)
    texts << label->text();
  // The chip names the store, and the description says what ticking it does.
  EXPECT_TRUE(texts.contains(QStringLiteral("Profile")));
  EXPECT_TRUE(std::ranges::any_of(texts, [](const QString& text) {
    return text.contains(QStringLiteral("annunciator"));
  }));
}

// A choice is a submenu in the menu vocabulary and a combo on the surface —
// the same translation the dialog made, kept because it is the right one.
TEST_F(SettingsPanelTest, AChoiceRowOffersItsSubmenuAndActivatesTheChoice) {
  delegate_.SetChecked(kClassic);
  SettingsPanel panel{nullptr, settings_};

  QWidget* row = Row(panel, QStringLiteral("colour-scheme"));
  ASSERT_NE(row, nullptr);
  auto* combo = row->findChild<QComboBox*>();
  ASSERT_NE(combo, nullptr);
  // The separator inside the submenu is not an option, and does not shift what
  // a selection activates.
  EXPECT_EQ(combo->count(), 2);
  EXPECT_EQ(combo->currentText(), QStringLiteral("Classic"));

  combo->setCurrentIndex(combo->findText(QStringLiteral("Dark")));
  emit combo->activated(combo->currentIndex());
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{kDark}));
}

// Toggles apply live: the control activates the command, and then re-reads the
// model rather than trusting that it took.
TEST_F(SettingsPanelTest, TogglingARowActivatesItsCommand) {
  SettingsPanel panel{nullptr, settings_};

  auto* box = Row(panel, QStringLiteral("toolbar"))->findChild<QCheckBox*>();
  ASSERT_NE(box, nullptr);
  EXPECT_FALSE(box->isChecked());

  box->click();
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{ID_VIEW_TOOLBAR}));
  EXPECT_TRUE(box->isChecked());
}

// Task 554's other half: the action the preferences dialog had to skip is a
// button here, in the category it belongs to.
TEST_F(SettingsPanelTest, TheDisplaysActionIsAButtonThatRunsItsCommand) {
  SettingsPanel panel{nullptr, settings_};

  QWidget* row = Row(panel, QStringLiteral("open-displays-folder"));
  ASSERT_NE(row, nullptr);
  auto* button = row->findChild<QPushButton*>();
  ASSERT_NE(button, nullptr);
  EXPECT_EQ(button->text(), QStringLiteral("Open Displays Folder"));
  // Not a checkbox, which is what drawing it as a preference would have made
  // it and why the dialog dropped it instead.
  EXPECT_TRUE(row->findChildren<QCheckBox*>().isEmpty());

  button->click();
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{ID_VIEW_PUBLIC_FOLDER}));
}

TEST_F(SettingsPanelTest, SearchNarrowsTheRowsAndTheTableOfContents) {
  SettingsPanel panel{nullptr, settings_};
  // Appearance, Events & alarms, Workspace, Displays.
  ASSERT_EQ(panel.category_list()->count(), 4);

  panel.search_field()->setText(QStringLiteral("taskbar"));

  EXPECT_EQ(VisibleIds(panel), (std::vector<std::string>{"flash-window"}));
  EXPECT_NE(Row(panel, QStringLiteral("flash-window")), nullptr);
  EXPECT_EQ(Row(panel, QStringLiteral("toolbar")), nullptr);
  // A category with nothing left in it is a destination that scrolls nowhere.
  ASSERT_EQ(panel.category_list()->count(), 1);
  EXPECT_TRUE(panel.category_list()->item(0)->text().startsWith(
      QStringLiteral("Events")));

  // Clearing the box puts everything back.
  panel.search_field()->clear();
  EXPECT_EQ(panel.visible_rows().size(), 6u);
}

TEST_F(SettingsPanelTest, ScopeTabsFilterAndOfferOnlyScopesInUse) {
  SettingsPanel panel{nullptr, settings_};

  QStringList tabs;
  for (int i = 0; i < panel.scope_tabs()->count(); ++i)
    tabs << panel.scope_tabs()->tabText(i);
  EXPECT_EQ(
      tabs,
      (QStringList{QStringLiteral("All"), QStringLiteral("This client"),
                   QStringLiteral("Profile"), QStringLiteral("This window")}));

  panel.scope_tabs()->setCurrentIndex(
      tabs.indexOf(QStringLiteral("This window")));
  EXPECT_EQ(VisibleIds(panel),
            (std::vector<std::string>{"toolbar", "status-bar"}));
}

// The panel owns no shell state, which is the whole argument for an overlay
// over a view: it is a child of the window it covers, it is in no layout, and
// closing it puts nothing back because it moved nothing.
TEST_F(SettingsPanelTest, TheOverlayCoversItsHostAndReservesTheStatusStrip) {
  QWidget host;
  host.resize(1000, 700);

  auto* panel = new SettingsPanel{&host, settings_};
  // Created hidden: constructing the panel must not put it on screen, because
  // the shell builds it once and opens it many times. `isHidden` rather than
  // `isVisible` throughout, because the host is never shown in these tests and
  // a child of an unshown window is never `isVisible` whatever it was asked.
  EXPECT_TRUE(panel->isHidden());
  // In no layout, so it displaces nothing it covers -- the whole argument for
  // an overlay over a workspace tab.
  EXPECT_EQ(host.layout(), nullptr);

  panel->Open(24);
  EXPECT_FALSE(panel->isHidden());
  EXPECT_EQ(panel->geometry(), QRect(0, 0, 1000, 700 - 24));
  // The search box takes focus, because an operator opening Settings is
  // looking for a setting and typing is how they look.
  EXPECT_EQ(panel->focusWidget(), panel->search_field());

  // It follows the window it covers; nothing else would resize it. A hidden
  // widget defers its resize event until it is shown and these tests never show
  // the host, so the event is delivered by hand -- the panel's filter is what
  // is under test, not Qt's delivery of it.
  const QSize before = host.size();
  host.resize(800, 500);
  QResizeEvent resized{host.size(), before};
  QApplication::sendEvent(&host, &resized);
  EXPECT_EQ(panel->geometry(), QRect(0, 0, 800, 500 - 24));
}

TEST_F(SettingsPanelTest, EscapeAndTheCloseButtonBothDismissIt) {
  QWidget host;
  host.resize(800, 600);
  auto* panel = new SettingsPanel{&host, settings_};

  panel->Open(0);
  ASSERT_FALSE(panel->isHidden());
  QKeyEvent escape{QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier};
  QApplication::sendEvent(panel, &escape);
  EXPECT_TRUE(panel->isHidden());

  panel->Open(0);
  auto* close =
      panel->findChild<QToolButton*>(QStringLiteral("settings-close"));
  ASSERT_NE(close, nullptr);
  close->click();
  EXPECT_TRUE(panel->isHidden());
}

// **The panel reserves space for a strip whose height is one of its own rows.**
// `Status Bar` is a `window`-scoped setting on this surface, so an operator can
// switch off the very strip the overlay stops above — and hiding a `QStatusBar`
// does not resize the window, so nothing the panel watches would fire. Until
// the panel said so, the inset went stale the moment the row was used: a band
// of exposed workbench below the panel, or a strip drawn under it.
//
// The panel cannot fix this itself, and deliberately does not try: it knows
// nothing about `QStatusBar` — the shell measures the strip and hands the
// number in. So the contract is that the panel reports having applied
// something and the shell re-supplies the inset, which is what this asserts.
TEST_F(SettingsPanelTest, ApplyingASettingLetsTheShellRestateTheInset) {
  QWidget host;
  host.resize(1000, 700);
  auto* panel = new SettingsPanel{&host, settings_};

  // Stands in for MainWindow::ShowSettings's measurement of the status strip:
  // 24px while Status Bar is on, nothing once it is off.
  int strip = 24;
  QObject::connect(panel, &SettingsPanel::SettingApplied, panel,
                   [panel, &strip] { panel->SetBottomInset(strip); });

  panel->Open(strip);
  ASSERT_EQ(panel->geometry(), QRect(0, 0, 1000, 700 - 24));

  strip = 0;
  auto* box =
      Row(*panel, QStringLiteral("status-bar"))->findChild<QCheckBox*>();
  ASSERT_NE(box, nullptr);
  box->click();

  EXPECT_EQ(delegate_.executed(), (std::vector<int>{ID_VIEW_STATUS_BAR}));
  EXPECT_EQ(panel->geometry(), QRect(0, 0, 1000, 700));
}

// Every control reports, not just the toggles: an action can move the strip
// too, and a choice certainly can — a longer locale changes what the strip
// draws, and a widget style changes its metrics.
TEST_F(SettingsPanelTest, EveryKindOfControlReportsThatItApplied) {
  SettingsPanel panel{nullptr, settings_};
  int applied = 0;
  QObject::connect(&panel, &SettingsPanel::SettingApplied, &panel,
                   [&applied] { ++applied; });

  Row(panel, QStringLiteral("toolbar"))->findChild<QCheckBox*>()->click();
  EXPECT_EQ(applied, 1);

  Row(panel, QStringLiteral("open-displays-folder"))
      ->findChild<QPushButton*>()
      ->click();
  EXPECT_EQ(applied, 2);

  auto* combo =
      Row(panel, QStringLiteral("colour-scheme"))->findChild<QComboBox*>();
  combo->setCurrentIndex(combo->findText(QStringLiteral("Dark")));
  emit combo->activated(combo->currentIndex());
  EXPECT_EQ(applied, 3);
}

// Restating the inset is not reopening: it must not steal focus back from
// whatever the operator was using, and must not rebuild the controls under
// their pointer.
TEST_F(SettingsPanelTest, RestatingTheInsetDisturbsNothingElse) {
  QWidget host;
  host.resize(800, 600);
  auto* panel = new SettingsPanel{&host, settings_};
  panel->Open(24);

  QWidget* before = Row(*panel, QStringLiteral("toolbar"));
  panel->search_field()->clearFocus();

  panel->SetBottomInset(40);

  EXPECT_EQ(panel->geometry(), QRect(0, 0, 800, 600 - 40));
  EXPECT_NE(panel->focusWidget(), panel->search_field());
  EXPECT_EQ(Row(*panel, QStringLiteral("toolbar")), before);
}

// Reopening re-reads the menu: a module can have registered a contribution, and
// the session's rights can have changed, since the panel was last on screen.
TEST_F(SettingsPanelTest, ReopeningPicksUpAMenuThatChangedMeanwhile) {
  QWidget host;
  auto* panel = new SettingsPanel{&host, settings_};
  panel->Open(0);
  ASSERT_EQ(Row(*panel, QStringLiteral("modus-topology")), nullptr);

  settings_.AddCheckItem(ID_MODUS_TOPOLOGY, u"Show Modus topology");
  panel->hide();
  panel->Open(0);

  EXPECT_NE(Row(*panel, QStringLiteral("modus-topology")), nullptr);
}

// A disabled command is a disabled control that says why, rather than a greyed
// row the operator is left guessing about.
TEST_F(SettingsPanelTest, ADisabledCommandExplainsItself) {
  delegate_.Disable(ID_EVENT_PLAY_SOUND);
  SettingsPanel panel{nullptr, settings_};

  auto* box =
      Row(panel, QStringLiteral("sound-on-events"))->findChild<QCheckBox*>();
  ASSERT_NE(box, nullptr);
  EXPECT_FALSE(box->isEnabled());
  EXPECT_EQ(box->toolTip(), QStringLiteral("no reason to enable it"));
}

}  // namespace
