#pragma once

#include <QDialog>

namespace scada::aui {
class MenuModel;
}

class QVBoxLayout;

// The preferences dialog — Settings > Settings... and the activity rail's
// pinned Settings utility, which share one command so the two entry points
// cannot diverge.
//
// It renders a `MenuModel` rather than a hand-written list of controls, and
// that is the whole point: the Settings menu was never a fixed set. It is
// assembled from the command registry — `MenuGroup::MAIN_WINDOW_SETTINGS`,
// `MenuGroup::DISPLAY_SETTINGS` and every module's `MainMenuId::Settings`
// contribution (Modus contributes two). A dialog listing hard-coded checkboxes
// would silently drop whatever a module added, and would drift every time one
// changed. Walking the model means the dialog and the menu can only ever agree.
//
// Settings apply live, as they did as menu items: each control activates its
// command immediately, so there is nothing to buffer and nothing to roll back.
// The button box therefore carries Close alone — an OK/Cancel pair would
// promise a transaction the commands do not implement.
class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  // `model` is the Settings submenu, owned by the main menu model and expected
  // to outlive the dialog.
  SettingsDialog(QWidget* parent, scada::aui::MenuModel& model);
  ~SettingsDialog() override;

 private:
  // Renders `model`'s items into `layout`. Recurses through in-place menus,
  // which contribute their items to the enclosing section rather than a group
  // of their own.
  //
  // `pending_rule` carries a separator the model asked for but that has not
  // been drawn yet, and is shared across the recursion so an in-place menu can
  // discharge one raised before it. See `FlushPendingRule`.
  void BuildSection(scada::aui::MenuModel& model,
                    QVBoxLayout* layout,
                    bool& pending_rule);

  // Draws a separator that is still owed, if one is and if it would actually
  // divide something. Separators are deferred rather than drawn where they
  // appear because the dialog does not render every item the menu does — a
  // plain command is skipped — so a rule can turn out to introduce a group
  // that is entirely absent. Delaying it until a control follows means a rule
  // is only ever drawn between two things the operator can see.
  void FlushPendingRule(QVBoxLayout* layout, bool& pending_rule);

  // Renders a submenu of mutually exclusive choices (Language, Style, Colour
  // scheme) as a labelled combo box. A submenu is the menu vocabulary for "pick
  // one of these"; a combo is the dialog vocabulary for the same thing.
  void AddChoiceRow(scada::aui::MenuModel& model,
                    int index,
                    QVBoxLayout* layout);

  // Renders one checkable item as a checkbox that activates its command.
  void AddCheckRow(scada::aui::MenuModel& model,
                   int index,
                   QVBoxLayout* layout);
};
