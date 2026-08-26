#include "main_window/settings_dialog_qt.h"

#include "aui/models/simple_menu_model.h"
#include "aui/test/app_environment.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace {

constexpr int kToggleA = 1;
constexpr int kToggleB = 2;
constexpr int kThemeDark = 10;
constexpr int kThemeLight = 11;
constexpr int kCommand = 3;

// Stands in for the command registry the real Settings menu is assembled from:
// it records what a control activated and answers the checked/enabled queries.
// A real SimpleMenuModel is driven on top of it rather than a mock menu, so the
// dialog is exercised against the same model class the shell builds.
class FakeDelegate : public scada::aui::SimpleMenuModel::Delegate {
 public:
  bool IsCommandIdChecked(int command_id) const override {
    return checked_.contains(command_id);
  }
  bool IsCommandIdEnabled(int command_id) const override {
    return !disabled_.contains(command_id);
  }
  std::u16string GetDisabledReasonForCommandId(int command_id) const override {
    const auto reason = disabled_reasons_.find(command_id);
    return reason == disabled_reasons_.end() ? std::u16string{}
                                             : reason->second;
  }
  void ExecuteCommand(int command_id) override {
    executed_.push_back(command_id);
    // The real option commands toggle, so the model's answer changes under the
    // dialog exactly as it would in the shell.
    if (!checked_.erase(command_id))
      checked_.insert(command_id);
  }

  void SetChecked(int command_id) { checked_.insert(command_id); }
  void Disable(int command_id, std::u16string reason) {
    disabled_.insert(command_id);
    disabled_reasons_[command_id] = std::move(reason);
  }
  const std::vector<int>& executed() const { return executed_; }

 private:
  std::set<int> checked_;
  std::set<int> disabled_;
  std::map<int, std::u16string> disabled_reasons_;
  std::vector<int> executed_;
};

// The section rules the dialog drew. `findChildren<QFrame*>` also reaches the
// frames Qt puts inside other widgets, so the shape is what identifies a rule.
int CountRules(const QDialog& dialog) {
  const QList<QFrame*> frames = dialog.findChildren<QFrame*>();
  return static_cast<int>(std::ranges::count_if(frames, [](QFrame* frame) {
    return frame->frameShape() == QFrame::HLine;
  }));
}

class SettingsDialogTest : public ::testing::Test {
 protected:
  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;

  FakeDelegate delegate_;
};

// The dialog's whole reason for rendering a model is that the Settings menu is
// assembled from the command registry and every module's contribution. A
// checkable item has to reach the operator as a checkbox, whatever added it.
TEST_F(SettingsDialogTest, CheckableItemsBecomeCheckboxes) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Control Confirmation");
  model.AddCheckItem(kToggleB, u"Speech");
  delegate_.SetChecked(kToggleA);

  SettingsDialog dialog{nullptr, model};

  const QList<QCheckBox*> boxes = dialog.findChildren<QCheckBox*>();
  ASSERT_EQ(boxes.size(), 2);
  EXPECT_EQ(boxes[0]->text(), "Control Confirmation");
  EXPECT_TRUE(boxes[0]->isChecked());
  EXPECT_FALSE(boxes[1]->isChecked());
}

TEST_F(SettingsDialogTest, TogglingACheckboxActivatesItsCommand) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Control Confirmation");

  SettingsDialog dialog{nullptr, model};
  dialog.findChildren<QCheckBox*>()[0]->click();

  EXPECT_EQ(delegate_.executed(), (std::vector<int>{kToggleA}));
}

// The box reports what the model says afterwards, not what the click implied.
// A command that refuses, or clamps the value, must not leave the dialog
// claiming a state the profile does not hold.
TEST_F(SettingsDialogTest, CheckboxFollowsTheModelRatherThanTheClick) {
  // A delegate whose command never takes effect.
  class StubbornDelegate : public FakeDelegate {
   public:
    void ExecuteCommand(int) override {}
  } stubborn;

  scada::aui::SimpleMenuModel model{&stubborn};
  model.AddCheckItem(kToggleA, u"Control Confirmation");

  SettingsDialog dialog{nullptr, model};
  QCheckBox* box = dialog.findChildren<QCheckBox*>()[0];
  box->click();

  EXPECT_FALSE(box->isChecked());
}

// Language, Style and Colour scheme are submenus of mutually exclusive
// choices. A submenu is the menu's way of saying "pick one"; a combo box is
// the dialog's.
TEST_F(SettingsDialogTest, ChoiceSubmenuBecomesACombo) {
  scada::aui::SimpleMenuModel themes{&delegate_};
  themes.AddRadioItem(kThemeDark, u"Dark", 0);
  themes.AddRadioItem(kThemeLight, u"Light", 0);
  delegate_.SetChecked(kThemeLight);

  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddSubMenu(0, u"Colour scheme", &themes);

  SettingsDialog dialog{nullptr, model};

  const QList<QComboBox*> combos = dialog.findChildren<QComboBox*>();
  ASSERT_EQ(combos.size(), 1);
  EXPECT_EQ(combos[0]->count(), 2);
  // The checked child is what the combo shows — the marker is a projection of
  // the model, not a record of what was last picked in this dialog.
  EXPECT_EQ(combos[0]->currentText(), "Light");
}

TEST_F(SettingsDialogTest, PickingAComboEntryActivatesThatChoice) {
  scada::aui::SimpleMenuModel themes{&delegate_};
  themes.AddRadioItem(kThemeDark, u"Dark", 0);
  themes.AddRadioItem(kThemeLight, u"Light", 0);

  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddSubMenu(0, u"Colour scheme", &themes);

  SettingsDialog dialog{nullptr, model};
  QComboBox* combo = dialog.findChildren<QComboBox*>()[0];
  // activated() is the user-interaction signal; setCurrentIndex alone would
  // not stand in for a pick.
  emit combo->activated(1);

  EXPECT_EQ(delegate_.executed(), (std::vector<int>{kThemeLight}));
}

// A separator inside the submenu must not shift what an entry activates: the
// combo carries the model index, it does not assume entry N is child N.
TEST_F(SettingsDialogTest, SeparatorInAChoiceSubmenuDoesNotShiftTheMapping) {
  scada::aui::SimpleMenuModel themes{&delegate_};
  themes.AddRadioItem(kThemeDark, u"Classic", 0);
  themes.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  themes.AddRadioItem(kThemeLight, u"Light", 0);

  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddSubMenu(0, u"Colour scheme", &themes);

  SettingsDialog dialog{nullptr, model};
  QComboBox* combo = dialog.findChildren<QComboBox*>()[0];
  ASSERT_EQ(combo->count(), 2);
  emit combo->activated(1);

  EXPECT_EQ(delegate_.executed(), (std::vector<int>{kThemeLight}));
}

// The menu's grouping is meaningful — it separates the window toggles from the
// event ones — so it survives into the dialog rather than being flattened.
TEST_F(SettingsDialogTest, SeparatorsBecomeSectionRules) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Toolbar");
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model.AddCheckItem(kToggleB, u"Speech");

  SettingsDialog dialog{nullptr, model};

  EXPECT_EQ(CountRules(dialog), 1);
}

// A module can contribute a plain command to the Settings menu, and the dialog
// skips it — but the separator the menu asked for in front of it must not
// survive its skipping. `ID_VIEW_PUBLIC_FOLDER` is registered exactly this way,
// and the rule it left behind read as a heading for whatever module came next.
TEST_F(SettingsDialogTest, SeparatorBeforeASkippedCommandIsNotDrawn) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Toolbar");
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model.AddItem(kCommand, u"Open Displays Folder");

  SettingsDialog dialog{nullptr, model};

  EXPECT_EQ(CountRules(dialog), 0);
}

// The group after a skipped command still has to be divided from the one
// before it — suppressing the rule must not merge two groups into one.
TEST_F(SettingsDialogTest, SeparatorSurvivesWhenAControlFollowsTheSkippedOne) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Toolbar");
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model.AddItem(kCommand, u"Open Displays Folder");
  model.AddCheckItem(kToggleB, u"Speech");

  SettingsDialog dialog{nullptr, model};

  EXPECT_EQ(CountRules(dialog), 1);
}

// A rule with nothing after it divides nothing, which is the same defect at
// the end of the list rather than the middle.
TEST_F(SettingsDialogTest, TrailingSeparatorIsNotDrawn) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Toolbar");
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);

  SettingsDialog dialog{nullptr, model};

  EXPECT_EQ(CountRules(dialog), 0);
}

// And a rule with nothing before it divides nothing either. The Settings menu
// opens with a separator whenever the first contributing module asks for one.
TEST_F(SettingsDialogTest, LeadingSeparatorIsNotDrawn) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model.AddCheckItem(kToggleA, u"Toolbar");

  SettingsDialog dialog{nullptr, model};

  EXPECT_EQ(CountRules(dialog), 0);
}

// An in-place menu contributes to the enclosing section, so a rule raised
// before it belongs to its first control — and an empty one must not consume
// the rule that the next real group needs.
TEST_F(SettingsDialogTest, EmptyInplaceMenuDoesNotConsumeThePendingRule) {
  scada::aui::SimpleMenuModel empty{&delegate_};

  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Toolbar");
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model.AddInplaceMenu(&empty);
  model.AddCheckItem(kToggleB, u"Speech");

  SettingsDialog dialog{nullptr, model};

  EXPECT_EQ(CountRules(dialog), 1);
}

// Speech is disabled when the platform has no voice. A greyed row with no
// explanation leaves the operator guessing, so the reason rides along.
TEST_F(SettingsDialogTest, DisabledItemCarriesItsReason) {
  scada::aui::SimpleMenuModel model{&delegate_};
  model.AddCheckItem(kToggleA, u"Speech");
  delegate_.Disable(kToggleA, u"No speech engine available");

  SettingsDialog dialog{nullptr, model};

  QCheckBox* box = dialog.findChildren<QCheckBox*>()[0];
  EXPECT_FALSE(box->isEnabled());
  EXPECT_EQ(box->toolTip(), "No speech engine available");
}

}  // namespace
