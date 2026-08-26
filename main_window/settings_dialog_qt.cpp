#include "main_window/settings_dialog_qt.h"

#include "aui/models/menu_model.h"
#include "aui/translation.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

using scada::aui::MenuModel;

// Whether `index` is an item the dialog draws at all. Separators become
// section breaks and everything else a control, but an invisible item is
// invisible here too — that is how an admin-gated contribution stays hidden.
bool IsRenderable(const MenuModel& model, int index) {
  return model.IsVisibleAt(index);
}

// The child of a choice submenu that is currently selected, or -1 when none
// is — which happens for Colour scheme before the operator has picked one.
int CheckedChild(const MenuModel& submenu) {
  for (int i = 0; i < submenu.GetItemCount(); ++i) {
    if (submenu.GetTypeAt(i) == MenuModel::TYPE_SEPARATOR)
      continue;
    if (submenu.IsItemCheckedAt(i))
      return i;
  }
  return -1;
}

}  // namespace

SettingsDialog::SettingsDialog(QWidget* parent, scada::aui::MenuModel& model)
    : QDialog{parent} {
  // Names the task, not the application: the OS title bar is the title, so the
  // content area repeats neither it nor a brand mark
  // (docs/client/ux/dialogs.md §1).
  setWindowTitle(QString::fromStdU16String(Translate("Settings")));

  auto* layout = new QVBoxLayout{this};

  // Dynamic models populate here; without it Language and the module
  // contributions would render empty.
  model.MenuWillShow();
  bool pending_rule = false;
  BuildSection(model, layout, pending_rule);
  // Any rule still owed here introduced nothing and is dropped: the trailing
  // case of the same rule FlushPendingRule applies between sections.

  layout->addStretch(1);

  // Close alone, because the settings have already applied. QDialogButtonBox
  // rather than a hand-laid row so the placement follows the platform
  // (docs/client/ux/dialogs.md §2), and so the label comes from Qt's own
  // catalogue already translated.
  auto* buttons = new QDialogButtonBox{QDialogButtonBox::Close, this};
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::FlushPendingRule(QVBoxLayout* layout, bool& pending_rule) {
  if (!pending_rule)
    return;
  pending_rule = false;

  // Nothing precedes it, so it would divide the first control from the title
  // bar rather than from another group.
  if (layout->count() == 0)
    return;

  auto* rule = new QFrame{this};
  rule->setFrameShape(QFrame::HLine);
  rule->setFrameShadow(QFrame::Sunken);
  layout->addWidget(rule);
}

void SettingsDialog::BuildSection(scada::aui::MenuModel& model,
                                  QVBoxLayout* layout,
                                  bool& pending_rule) {
  for (int index = 0; index < model.GetItemCount(); ++index) {
    if (!IsRenderable(model, index))
      continue;

    switch (model.GetTypeAt(index)) {
      case MenuModel::TYPE_SEPARATOR:
        // The menu's grouping is meaningful — it is what separates the window
        // toggles from the event ones — so it survives as a rule rather than
        // being flattened away. Recorded rather than drawn: see
        // `FlushPendingRule`. Consecutive separators collapse into one, which
        // is what the menu does with them too.
        pending_rule = true;
        break;

      case MenuModel::TYPE_CHECK:
      case MenuModel::TYPE_RADIO:
        FlushPendingRule(layout, pending_rule);
        AddCheckRow(model, index, layout);
        break;

      case MenuModel::TYPE_SUBMENU:
        FlushPendingRule(layout, pending_rule);
        AddChoiceRow(model, index, layout);
        break;

      case MenuModel::TYPE_INPLACE_MENU:
        // Contributes its items to this section rather than a group of its
        // own, which is what "in-place" means in the menu too — including the
        // pending rule, which its first control discharges and an empty one
        // leaves for whatever follows.
        if (MenuModel* inplace = model.GetSubmenuModelAt(index)) {
          inplace->MenuWillShow();
          BuildSection(*inplace, layout, pending_rule);
        }
        break;

      case MenuModel::TYPE_COMMAND:
      case MenuModel::TYPE_BUTTON_ITEM:
        // A plain command in a preferences list is an action, not a
        // preference; rendering it as a checkbox would misreport it, so it is
        // deliberately skipped. `ID_VIEW_PUBLIC_FOLDER` ("Open Displays
        // Folder") is one, and it is registered with `separator_before`, so
        // skipping it must not leave that rule standing in front of the next
        // module's group — which is why the rule waits for a control instead.
        break;
    }
  }
}

void SettingsDialog::AddCheckRow(scada::aui::MenuModel& model,
                                 int index,
                                 QVBoxLayout* layout) {
  auto* box =
      new QCheckBox{QString::fromStdU16String(model.GetLabelAt(index)), this};
  box->setChecked(model.IsItemCheckedAt(index));
  box->setEnabled(model.IsEnabledAt(index));
  // Speech is disabled when the platform has no voice; saying why beats a
  // greyed row with no explanation.
  if (!model.IsEnabledAt(index)) {
    if (const std::u16string reason = model.GetDisabledReasonAt(index);
        !reason.empty()) {
      box->setToolTip(QString::fromStdU16String(reason));
    }
  }

  // The command toggles, so it is activated rather than set — the same call
  // the menu item made. Re-reading the model afterwards keeps the box honest
  // if the command refused or the profile clamped the value.
  connect(box, &QCheckBox::clicked, this, [&model, index, box] {
    model.ActivatedAt(index);
    box->setChecked(model.IsItemCheckedAt(index));
  });

  layout->addWidget(box);
}

void SettingsDialog::AddChoiceRow(scada::aui::MenuModel& model,
                                  int index,
                                  QVBoxLayout* layout) {
  MenuModel* submenu = model.GetSubmenuModelAt(index);
  if (!submenu)
    return;
  submenu->MenuWillShow();

  auto* row = new QHBoxLayout;
  row->addWidget(new QLabel{
      QString::fromStdU16String(model.GetLabelAt(index)) + QChar{':'}, this});

  auto* combo = new QComboBox{this};
  // The model index behind each entry, so a separator inside the submenu
  // cannot shift what a selection activates.
  for (int child = 0; child < submenu->GetItemCount(); ++child) {
    if (submenu->GetTypeAt(child) == MenuModel::TYPE_SEPARATOR)
      continue;
    if (!submenu->IsVisibleAt(child))
      continue;
    combo->addItem(QString::fromStdU16String(submenu->GetLabelAt(child)),
                   child);
  }

  if (const int checked = CheckedChild(*submenu); checked >= 0)
    combo->setCurrentIndex(combo->findData(checked));

  connect(combo, &QComboBox::activated, this, [submenu, combo](int entry) {
    const QVariant child = combo->itemData(entry);
    if (child.isValid())
      submenu->ActivatedAt(child.toInt());
  });

  row->addWidget(combo, 1);
  layout->addLayout(row);
}
