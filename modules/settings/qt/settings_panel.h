#pragma once

#include "base/lifetime.h"
#include "settings/settings_catalog.h"

#include <QWidget>

#include <optional>
#include <vector>

namespace scada::aui {
class MenuModel;
}

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QTabBar;
class QVBoxLayout;

// The Settings surface — the workbench's preferences editor, drawn from
// `settings_catalog.h` rather than from the Settings menu alone.
//
// It replaced the modal `SettingsDialog` on 2026-08-28. The screen both realms
// share (`docs/product/ui-mockups/screens/settings.html`, inventory in
// `settings-rows.html`) is a surface, not a dialog: a search field across the
// top, a category table of contents where the Explorer would be, and one row
// per setting carrying its title, its description and a chip naming the store
// its value lives in. None of those four can exist over a bare menu model,
// which is why the catalogue exists and why the dialog could not simply grow
// them.
//
// **An overlay, not a workspace tab.** It covers the whole window below the
// status strip — the activity rail, the Explorer and the specialist docks, the
// tab strip, and the bottom-docked events pane — rather than taking a tab
// beside them. That is the shape the product settled on (2026-08-28,
// superseding the tab the screen draws): preferences are a mode the operator
// enters and leaves, so covering the workbench and then uncovering it says
// exactly that, where a tab would leave two navigation columns on screen and a
// half-visible workbench that is not currently the subject.
//
// The corollary is that **the panel owns no shell state**. It hides nothing,
// moves nothing and restores nothing: it is a child widget raised over its
// parent, so closing it reveals the workbench exactly as the operator left it.
// A surface that rearranged the shell would have to put it back, and putting it
// back is where that kind of surface goes wrong.
//
// **Native, not the web client's design system.** Parity with the web client is
// capability and information architecture — search, categories, rows, storage
// scopes — never appearance. Everything here is a stock Qt widget taking its
// colour from `QPalette` and its metrics from the platform style; there is no
// stylesheet and no hard-coded colour, so the panel follows the host OS and
// whichever appearance the operator has installed.
//
// Settings apply live, as they did as menu items and in the dialog: each
// control activates its command immediately, so there is nothing to buffer,
// nothing to roll back, and the close button is a close rather than an OK.
class SettingsPanel : public QWidget {
  Q_OBJECT

 public:
  // `parent` is the window the panel covers; `settings_menu` is the
  // `MainMenuId::Settings` model, owned by the main menu model and expected to
  // outlive this widget.
  SettingsPanel(QWidget* parent, scada::aui::MenuModel& settings_menu);
  ~SettingsPanel() override;

  // Re-reads the catalogue, resizes to cover `parent` down to `bottom_inset`
  // pixels above its bottom edge, and shows itself on top.
  //
  // `bottom_inset` is how the status strip stays visible: the shell knows its
  // height and the panel does not, so the shell passes it rather than the panel
  // reaching for a `QStatusBar` it has no business knowing about.
  void Open(int bottom_inset);

  // Restates the reserved strip without reopening — no catalogue reload, no
  // focus change, no rebuilt controls.
  //
  // The shell calls this from `SettingApplied`, because the strip's height is
  // itself one of the settings on this surface: `Status Bar` is a `window`
  // -scoped row, and switching it off does not resize the window, so nothing
  // the panel watches would notice. See `SettingApplied`.
  void SetBottomInset(int bottom_inset);

  // The search box, the scope tabs and the table of contents, exposed for the
  // tests and the screenshot generator so neither has to reach in by
  // `objectName`.
  QLineEdit* search_field() const { return search_field_; }
  QTabBar* scope_tabs() const { return scope_tabs_; }
  QListWidget* category_list() const { return category_list_; }

  // The rows currently drawn, after the search box and the scope tab. Exposed
  // because "which rows survived" is what almost every assertion about this
  // surface is really about.
  const std::vector<SettingRow>& visible_rows() const SCADA_LIFETIME_BOUND {
    return visible_rows_;
  }

 signals:
  // A control activated its command. The panel has already re-read the model,
  // so this is not about the control's own state — it is for the shell, which
  // may have to restate something the panel cannot see for itself.
  //
  // Today that is exactly one thing: the bottom inset. `Status Bar` and
  // `Toolbar` are rows on this surface that change the chrome underneath it,
  // and a locale or widget-style change moves the strip's metrics too, so the
  // shell re-measures and calls `SetBottomInset`.
  void SettingApplied();

 protected:
  // QWidget — Escape closes, which is what every other transient surface in the
  // shell does and what the operator will try first.
  void keyPressEvent(QKeyEvent* event) override;
  // Follows the window it covers. The panel is in no layout, so nothing else
  // would resize it.
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  // Re-reads the catalogue from the menu model. Called on every open and
  // whenever a control has activated a command, because a command can refuse or
  // clamp what it was asked to do and because one preference can change
  // another's availability.
  void ReloadCatalog();
  // Re-derives the scope tabs from the catalogue, keeping the operator's
  // selection when the scope it named is still on offer.
  void RebuildScopeTabs();
  // Redraws the list and the table of contents for the current query and scope.
  void RebuildRows();
  // Matches the panel to the covered window, minus the strip the shell
  // reserved.
  void FitToParent();

  // The scope the selected tab filters by, or none for "All".
  std::optional<SettingScope> SelectedScope() const;

  // One row: title, scope chip, description, control.
  QWidget* CreateRow(const SettingRow& row);
  QWidget* CreateChoiceControl(const SettingRow& row, QWidget* parent);
  QCheckBox* CreateToggleControl(const SettingRow& row, QWidget* parent);
  QWidget* CreateActionControl(const SettingRow& row, QWidget* parent);

  scada::aui::MenuModel& settings_menu_;

  std::vector<SettingRow> catalog_;
  std::vector<SettingRow> visible_rows_;
  std::vector<SettingScope> scopes_;

  QLineEdit* search_field_ = nullptr;
  QTabBar* scope_tabs_ = nullptr;
  QLabel* count_label_ = nullptr;
  QListWidget* category_list_ = nullptr;
  QWidget* rows_host_ = nullptr;
  QVBoxLayout* rows_layout_ = nullptr;

  int bottom_inset_ = 0;

  // Guards the re-entrancy a live surface invites: reloading the catalogue
  // rebuilds the controls, and rebuilding a control emits the signal that would
  // reload the catalogue again.
  bool reloading_ = false;
};
