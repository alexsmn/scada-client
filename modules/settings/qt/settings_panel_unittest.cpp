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
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QTabBar>
#include <QToolButton>

#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <stdexcept>
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
    // A checkable menu item toggles; a member of a radio group *selects*, and
    // the difference is not cosmetic here. Modelling a choice submenu as a set
    // of independent toggles leaves the previously-selected row checked too,
    // and `CheckedChild` then returns whichever comes first — so a combo
    // rebuilt after a choice reads the option the operator just moved away
    // from. That looks exactly like the panel reporting before it rebuilt,
    // which is what a fake with the wrong semantics costs: it does not weaken
    // an assertion, it inverts one.
    if (const std::set<int>* group = GroupOf(command_id)) {
      for (int member : *group)
        checked_.erase(member);
      checked_.insert(command_id);
      return;
    }
    if (!checked_.erase(command_id))
      checked_.insert(command_id);
  }

  void SetChecked(int command_id) { checked_.insert(command_id); }
  void Disable(int command_id) { disabled_.insert(command_id); }
  // Declares `ids` mutually exclusive, the way a choice submenu's rows are.
  void AddRadioGroup(std::set<int> ids) { groups_.push_back(std::move(ids)); }
  const std::vector<int>& executed() const { return executed_; }

 private:
  const std::set<int>* GroupOf(int command_id) const {
    for (const std::set<int>& group : groups_) {
      if (group.contains(command_id))
        return &group;
    }
    return nullptr;
  }

  std::set<int> checked_;
  std::set<int> disabled_;
  std::vector<std::set<int>> groups_;
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

    // A separator inside the submenu on purpose: Colour scheme itself carries
    // none any more, but other choice submenus do, and the panel must not
    // render one as an option or let it shift what a selection activates.
    appearance_.AddCheckItem(kSystem, u"Follow system");
    appearance_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    appearance_.AddCheckItem(kDark, u"Dark");
    // The real AppearanceMenuModel and StyleMenuModel report exactly one
    // checked row; the fake has to as well or the combo lags a step behind.
    delegate_.AddRadioGroup({kSystem, kDark});
    settings_.AddSubMenu(ID_SETTINGS_APPEARANCE, u"Colour scheme",
                         &appearance_);
  }

  static constexpr int kSystem = 90011;
  static constexpr int kDark = 90012;

  // The row widget carrying `id`, or null when the panel is not drawing it.
  // Rows are found by `objectName`, which is the catalogue's stable handle
  // rather than a translated title. Nullable, for the tests that assert a row
  // is *absent*.
  QWidget* Row(const SettingsPanel& panel, const QString& id) {
    return panel.findChild<QWidget*>(id);
  }

  // The row, or a failure. For every test that reads one.
  //
  // Dereferencing `Row` directly aborts the whole executable on a miss, and
  // every test after it then reports nothing at all -- no failure line, no
  // summary, which reads as if those tests do not exist rather than as if they
  // failed. That is a different silence from a vacuous pass and a worse one:
  // an empty catalogue used to take this binary down at the fifth test and
  // leave the other twenty-four unaccounted for. Throwing gives gtest a
  // failure it can attribute and lets the suite finish.
  QWidget& RequireRow(const SettingsPanel& panel, const QString& id) {
    QWidget* row = Row(panel, id);
    if (!row)
      throw std::runtime_error("no settings row " + id.toStdString());
    return *row;
  }

  // The row's control, or a failure, for the same reason.
  template <class Control>
  Control& RequireControl(const SettingsPanel& panel, const QString& id) {
    auto* control = RequireRow(panel, id).findChild<Control*>();
    if (!control)
      throw std::runtime_error("settings row " + id.toStdString() +
                               " draws no control of the expected kind");
    return *control;
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

  QWidget& sound = RequireRow(panel, QStringLiteral("sound-on-events"));
  // A toggle's title is its checkbox's label.
  const QList<QCheckBox*> boxes = sound.findChildren<QCheckBox*>();
  ASSERT_EQ(boxes.size(), 1);
  EXPECT_EQ(boxes.front()->text(), QStringLiteral("Sound Alarm on Event"));

  const QList<QLabel*> labels = sound.findChildren<QLabel*>();
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
  delegate_.SetChecked(kSystem);
  SettingsPanel panel{nullptr, settings_};

  QComboBox& combo =
      RequireControl<QComboBox>(panel, QStringLiteral("colour-scheme"));
  // The separator inside the submenu is not an option, and does not shift what
  // a selection activates.
  EXPECT_EQ(combo.count(), 2);
  EXPECT_EQ(combo.currentText(), QStringLiteral("Follow system"));

  combo.setCurrentIndex(combo.findText(QStringLiteral("Dark")));
  emit combo.activated(combo.currentIndex());
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{kDark}));
}

// Toggles apply live: the control activates the command, and then re-reads the
// model rather than trusting that it took.
TEST_F(SettingsPanelTest, TogglingARowActivatesItsCommand) {
  SettingsPanel panel{nullptr, settings_};

  QCheckBox& box = RequireControl<QCheckBox>(panel, QStringLiteral("toolbar"));
  EXPECT_FALSE(box.isChecked());

  box.click();
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{ID_VIEW_TOOLBAR}));
  EXPECT_TRUE(box.isChecked());
}

// Task 554's other half: the action the preferences dialog had to skip is a
// button here, in the category it belongs to.
TEST_F(SettingsPanelTest, TheDisplaysActionIsAButtonThatRunsItsCommand) {
  SettingsPanel panel{nullptr, settings_};

  QWidget& row = RequireRow(panel, QStringLiteral("open-displays-folder"));
  QPushButton& button = RequireControl<QPushButton>(
      panel, QStringLiteral("open-displays-folder"));
  EXPECT_EQ(button.text(), QStringLiteral("Open Displays Folder"));
  // Not a checkbox, which is what drawing it as a preference would have made
  // it and why the dialog dropped it instead.
  EXPECT_TRUE(row.findChildren<QCheckBox*>().isEmpty());

  button.click();
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
  RequireControl<QCheckBox>(*panel, QStringLiteral("status-bar")).click();

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

  RequireControl<QCheckBox>(panel, QStringLiteral("toolbar")).click();
  EXPECT_EQ(applied, 1);

  RequireControl<QPushButton>(panel, QStringLiteral("open-displays-folder"))
      .click();
  EXPECT_EQ(applied, 2);

  QComboBox& combo =
      RequireControl<QComboBox>(panel, QStringLiteral("colour-scheme"));
  combo.setCurrentIndex(combo.findText(QStringLiteral("Dark")));
  emit combo.activated(combo.currentIndex());
  EXPECT_EQ(applied, 3);
}

// **Ordering, not just occurrence.** The two tests above pin that the signal
// fires; this pins where. A choice row reloads the catalogue before reporting,
// so the shell re-measures against the strings and metrics the choice just
// installed — a locale changes what the strip draws, a widget style changes its
// metrics. Emitting first would have the shell measure the outgoing appearance,
// which is right often enough to look correct and wrong exactly when it
// matters.
TEST_F(SettingsPanelTest, AChoiceRebuildsBeforeItReportsThatItApplied) {
  delegate_.SetChecked(kSystem);
  SettingsPanel panel{nullptr, settings_};

  QPointer<QComboBox> clicked =
      &RequireControl<QComboBox>(panel, QStringLiteral("colour-scheme"));

  // The observation has to be one that can ONLY be true after the rebuild.
  // Reading the combo's text is not: the test sets the index before emitting
  // `activated`, so the pre-rebuild control already reads "Dark" and the
  // assertion passes whichever order the panel used. Whether the clicked
  // control has been destroyed yet cannot be faked that way.
  bool rebuilt_before_report = false;
  QString seen_on_report;
  QObject::connect(
      &panel, &SettingsPanel::SettingApplied, &panel,
      [this, &panel, &clicked, &rebuilt_before_report, &seen_on_report] {
        rebuilt_before_report = clicked.isNull();
        auto* rebuilt = Row(panel, QStringLiteral("colour-scheme"))
                            ->findChild<QComboBox*>();
        seen_on_report =
            rebuilt ? rebuilt->currentText() : QStringLiteral("<no combo>");
      });

  clicked->setCurrentIndex(clicked->findText(QStringLiteral("Dark")));
  emit clicked->activated(clicked->currentIndex());

  EXPECT_TRUE(rebuilt_before_report)
      << "the shell was told a setting applied while the panel still showed "
         "the appearance it was replacing";
  EXPECT_EQ(seen_on_report, QStringLiteral("Dark"));
}

// The same rebuild destroys the combo that is mid-emission, which is safe but
// invisible — the deletion is three calls down inside `ReloadCatalog`, so
// nothing at the connect site shows it. Activating twice runs that path twice
// and uses the replacement, which is what would fall over if the rebuild ever
// stopped being safe to do from inside the sender's own signal.
TEST_F(SettingsPanelTest, ActivatingAChoiceTwiceUsesTheRebuiltControl) {
  delegate_.SetChecked(kSystem);
  SettingsPanel panel{nullptr, settings_};

  auto activate = [this, &panel](const QString& option) {
    QComboBox& combo =
        RequireControl<QComboBox>(panel, QStringLiteral("colour-scheme"));
    combo.setCurrentIndex(combo.findText(option));
    emit combo.activated(combo.currentIndex());
  };

  QPointer<QComboBox> first =
      &RequireControl<QComboBox>(panel, QStringLiteral("colour-scheme"));
  activate(QStringLiteral("Dark"));
  // The control that was clicked is gone, replaced by one built from the model
  // as it now reads.
  EXPECT_TRUE(first.isNull());

  activate(QStringLiteral("Follow system"));
  EXPECT_EQ(delegate_.executed(), (std::vector<int>{kDark, kSystem}));
  EXPECT_EQ(RequireControl<QComboBox>(panel, QStringLiteral("colour-scheme"))
                .currentText(),
            QStringLiteral("Follow system"));
}

// Restating the inset is not reopening: it must not steal focus back from
// whatever the operator was using, and must not rebuild the controls under
// their pointer.
TEST_F(SettingsPanelTest, RestatingTheInsetDisturbsNothingElse) {
  QWidget host;
  host.resize(800, 600);
  auto* panel = new SettingsPanel{&host, settings_};
  panel->Open(24);

  // `RequireRow` rather than `Row`: the last assertion compares the two
  // lookups, and null equals null, so it would pass for a panel that drew no
  // rows at all -- the setup silently not happening, which is the one thing an
  // equality between two lookups cannot report.
  QWidget* before = &RequireRow(*panel, QStringLiteral("toolbar"));
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

  QCheckBox& box =
      RequireControl<QCheckBox>(panel, QStringLiteral("sound-on-events"));
  EXPECT_FALSE(box.isEnabled());
  EXPECT_EQ(box.toolTip(), QStringLiteral("no reason to enable it"));
}

}  // namespace
