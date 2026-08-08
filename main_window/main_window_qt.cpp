#include "main_window/main_window_qt.h"

#include "aui/models/menu_model.h"
#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model.h"
#include "aui/qt/status_bar.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/auto_reset.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "base/utf_convert.h"
#include "controller/action_manager.h"
#include "controller/command_manager.h"
#include "controller/command_ui_registry.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "device_diagnostics/qt/device_diagnostics_panel.h"
#include "events/alarm_flood.h"
#include "events/qt/severity_tile_strip.h"
#include "filesystem/file_cache.h"
#include "inspector/qt/inspector_panel.h"
#include "main_window/activity_bar_qt.h"
#include "main_window/breadcrumb_qt.h"
#include "main_window/command_field_qt.h"
#include "main_window/command_palette_qt.h"
#include "main_window/main_menu/main_menu_model.h"
#include "main_window/main_window_command_router.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/overview_page.h"
#include "main_window/page_icons.h"
#include "main_window/pages/page_switcher.h"
#include "main_window/selection_command_router.h"
#include "main_window/settings_dialog_qt.h"
#include "main_window/status_bar/progress_controller_qt.h"
#include "main_window/tag_search_index.h"
#include "main_window/view_manager.h"
#include "main_window/window_definition_builder.h"
#include "model/devices_node_ids.h"
#include "model/security_node_ids.h"
#include "modules/write/write_availability.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "scada/standard_node_ids.h"
#include "transmission_rules/qt/transmission_rule_inspector.h"
#include "ui/common/client_utils.h"
#include "ui/qt/client_utils_qt.h"
#include "user_access/qt/user_access_panel.h"

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>
#include <QShortcut>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

#include <ranges>
#include <unordered_set>

namespace {

inline QKeySequence ToQKeySequence(const Shortcut& shortcut) {
  return QKeySequence{static_cast<int>(shortcut.key_code()) +
                      static_cast<int>(shortcut.modifiers())};
}

QRect GetDefaultBounds(const QWidget* window) {
  QScreen* screen =
      window ? window->screen() : QGuiApplication::primaryScreen();
  if (!screen)
    screen = QGuiApplication::primaryScreen();
  const QRect desktop_bounds =
      screen ? screen->availableGeometry() : QRect{0, 0, 1024, 768};
  return {desktop_bounds.left() + desktop_bounds.width() / 8,
          desktop_bounds.top() + desktop_bounds.height() / 8,
          desktop_bounds.width() * 3 / 4, desktop_bounds.height() * 3 / 4};
}

// Recursively collects the command ids carried by |model| (including its
// submenus and inplace menus) so the appended generic context menu can skip
// them.
void CollectMenuCommandIds(scada::aui::MenuModel& model,
                           std::unordered_set<int>& command_ids) {
  model.MenuWillShow();
  for (int i = 0; i < model.GetItemCount(); ++i) {
    switch (model.GetTypeAt(i)) {
      case scada::aui::MenuModel::TYPE_SUBMENU:
      case scada::aui::MenuModel::TYPE_INPLACE_MENU:
        if (auto* submenu_model = model.GetSubmenuModelAt(i))
          CollectMenuCommandIds(*submenu_model, command_ids);
        break;
      case scada::aui::MenuModel::TYPE_SEPARATOR:
        break;
      default:
        command_ids.insert(model.GetCommandIdAt(i));
        break;
    }
  }
}

// Drops leading, trailing and consecutive separators, which can appear once
// duplicate items have been filtered out of the appended generic menu.
void RemoveRedundantSeparators(QMenu& menu) {
  QList<QAction*> actions = menu.actions();
  bool previous_was_separator = true;  // Leading separators are redundant.
  for (auto* action : actions) {
    if (action->isSeparator()) {
      if (previous_was_separator)
        menu.removeAction(action);
      else
        previous_was_separator = true;
    } else {
      previous_was_separator = false;
    }
  }
  // A trailing separator, if any, is now the last remaining separator.
  const QList<QAction*>& remaining = menu.actions();
  if (!remaining.isEmpty() && remaining.last()->isSeparator())
    menu.removeAction(remaining.last());
}

void BuildDefaultPopupMenu(QMenu& menu,
                           scada::aui::MenuModel* merge_menu,
                           scada::aui::MenuModel& context_menu_model) {
  std::unordered_set<int> merge_command_ids;
  if (merge_menu && merge_menu->GetItemCount() != 0) {
    BuildMenu(menu, *merge_menu);
    menu.addSeparator();
    CollectMenuCommandIds(*merge_menu, merge_command_ids);
  }
  // The caller-supplied |merge_menu| is authoritative for the commands it
  // lists; suppress those same commands from the generic context menu so a
  // converted view's curated entries aren't shown twice.
  BuildMenu(menu, context_menu_model, &merge_command_ids);
  RemoveRedundantSeparators(menu);
}

}  // namespace

MainWindow::MainWindow(MainWindowContext&& context)
    : BaseMainWindow{std::move(context), dialog_service_} {
  const MainWindowDef& prefs = GetPrefs();

  setGeometry(prefs.bounds.isNull() ? GetDefaultBounds(this) : prefs.bounds);

  dialog_service_.parent_widget = this;

  view_manager_ = std::make_unique<ViewManager>(
      *this, *static_cast<ViewManagerDelegate*>(this));
  AttachViewManager(*view_manager_);

  CreateMenuBar();
  // Opt-in top context bar, on its own row above the command toolbar. Gated on
  // the active UX theme, which both the app (app/qt/installed_appearance.h) and
  // the headless screenshot generator set together with the palette when the
  // experimental UX is enabled.
  //
  // Read once, here: this chrome is structural, so Settings → Colour scheme can
  // recolour a running client but cannot add or remove these widgets — it says
  // so when the operator crosses that boundary (see AppearanceMenuModel).
  if (scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy) {
    CreateActivityBar();
    WireRailPages();
    CreateContextBar();
    CreateInspectorPanel();
    CreateDiagnosticsPanel();
    CreateUserAccessPanel();
    CreateTransmissionRulePanel();
    TabifySpecialistDocks();
    // Kick off the palette's tag browse in the background so tags are ready by
    // the time the operator first opens the palette.
    if (node_service_) {
      tag_search_index_ = std::make_unique<TagSearchIndex>(
          executor_, *node_service_, scada::id::ObjectsFolder);
      tag_search_index_->EnsurePopulated();
    }
  }
  CreateToolbar();
  CreateStatusBar();

  if (!g_hide_for_testing) {
    // Must show window before loading layout to let |restoreState()| work
    // correctly.
    switch (prefs.state) {
      case MainWindowDef::State::kMaximized:
        showMaximized();
        break;
      case MainWindowDef::State::kMinimized:
        showMinimized();
        break;
      default:
        show();
        break;
    }
  }

  Init(*view_manager_);
  RebuildMenuBar();

  // Init() opened the restored page, which conformed it to the active mode.
  // Bring the rail in line with what is actually on screen.
  RefreshPaneModeMarker();
  // The first pass that can see a populated command registry, so it is also
  // what decides whether the admin-gated Users utility is offered at all.
  RefreshUtilityMarker();

  action_changed_connection_ = ui_command_registry_.action_manager().Subscribe(
      [this](Action& action, ActionChangeMask change_mask) {
        OnActionChanged(action, change_mask);
      });

  change_profile_connection_ = profile_.AddChangeObserver([this] {
    const MainWindowDef& prefs = GetPrefs();
    statusBar()->setVisible(prefs.status_bar);
    toolbar_->setVisible(prefs.toolbar);
  });
}

MainWindow::~MainWindow() {
  action_changed_connection_.disconnect();

  view_manager_->ClosePage();
  // TODO: Comment why explicit reset is needed.
  view_manager_.reset();
}

void MainWindow::UpdateTitle() {
  QString server =
      QString::fromStdU16String(FormatHostName(connection_info_provider_()));
  QString page =
      QString::fromStdU16String(view_manager_->current_page().GetTitle());
  QString title = tr("%1 (Server: %2)").arg(page).arg(server);
  setWindowTitle(title);

  // Renaming a page ends here (PageCommands -> SetCurrentPageTitle ->
  // UpdateTitle) and nowhere else, so this is the hook that keeps the Pages
  // list's label in step. PageCommands does not raise a profile change.
  RefreshRailPages();
  // Same hook, same reason: the breadcrumb's first segment is the page title.
  RefreshBreadcrumb();
}

void MainWindow::CreateMenuBar() {
  main_menu_model_ = main_menu_factory_(*this, dialog_service_, *view_manager_,
                                        *commands_, *context_menu_model_);

  auto* menu_bar = new QMenuBar(this);
  setMenuBar(menu_bar);

  for (int i = 0; i < main_menu_model_->GetItemCount(); ++i) {
    const std::u16string label = main_menu_model_->GetLabelAt(i);
    auto* submenu = menu_bar->addMenu(QString::fromStdU16String(label));
    auto* submenu_model = main_menu_model_->GetSubmenuModelAt(i);
    scada::base::Check(submenu_model);
    QObject::connect(submenu, &QMenu::aboutToShow, this,
                     [submenu, submenu_model] {
                       submenu->clear();
                       BuildMenu(*submenu, *submenu_model);
                     });
#ifdef __APPLE__
    auto* loading_action = submenu->addAction(tr("Loading..."));
    loading_action->setEnabled(false);
#endif
  }
}

void MainWindow::RebuildMenuBar() {
  const auto top_level_actions = menuBar()->actions();
  // The appended Settings menu is not part of the model, so allow one extra.
  scada::base::Check(top_level_actions.size() >=
                     main_menu_model_->GetItemCount());

  for (int i = 0; i < main_menu_model_->GetItemCount(); ++i) {
    auto* submenu = top_level_actions[i]->menu();
    auto* submenu_model = main_menu_model_->GetSubmenuModelAt(i);
    scada::base::Check(submenu);
    scada::base::Check(submenu_model);
    submenu->clear();
    BuildMenu(*submenu, *submenu_model);
  }
}

void MainWindow::CreateStatusBar() {
  auto* status_bar = new scada::aui::StatusBar{*status_bar_model_, this};
  setStatusBar(status_bar);
  status_bar->setVisible(GetPrefs().status_bar);

  progress_controller_ =
      std::make_unique<ProgressController>(*status_bar, progress_host_);
}

void MainWindow::CreateContextBar() {
  context_bar_ = new QToolBar(this);
  context_bar_->setObjectName(QStringLiteral("ContextBar"));
  context_bar_->setMovable(false);
  context_bar_->setFloatable(false);
  context_bar_->setContextMenuPolicy(Qt::PreventContextMenu);

  // No brand lockup. A native application identifies itself in the window
  // title and the About dialog, not with an in-window mark — and the one that
  // stood here was a hard-coded, space-padded, untranslatable literal
  // (docs/client/ux/shell.md §2.2, native rework).
  // Where the workspace is: page → view → selected object. The left slot the
  // brand lockup vacated, which is the slot `config-workbench.html` draws a
  // breadcrumb in. It coexists with the command field rather than replacing it
  // — shell.md §2.2 — because the two answer different questions.
  breadcrumb_ = new Breadcrumb(context_bar_);
  context_bar_->addWidget(breadcrumb_);

  auto* left_spacer = new QWidget(context_bar_);
  left_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  context_bar_->addWidget(left_spacer);

  // Command/search entry point (centre). It owns no text: clicking it, or
  // typing into it, opens the command palette, which is where the typing
  // happens. The field draws the magnifier and the shortcut hint itself.
  const QKeySequence palette_key{Qt::CTRL | Qt::Key_K};
  command_search_ = new CommandField(
      context_bar_,
      QString::fromStdU16String(Translate("Search tags, objects, commands…")),
      palette_key, [this](const QString& initial_text) {
        ShowCommandPalette(initial_text);
      });
  context_bar_->addWidget(command_search_);

  auto* palette_shortcut = new QShortcut(palette_key, this);
  connect(palette_shortcut, &QShortcut::activated, this,
          [this] { ShowCommandPalette(); });

  auto* right_spacer = new QWidget(context_bar_);
  right_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  context_bar_->addWidget(right_spacer);

  // Alarm-flood escalation pill (hidden unless a flood is active), left of the
  // per-severity tiles so it reads as the dominant state during a flood.
  flood_indicator_ = new QLabel(context_bar_);
  flood_indicator_->setMargin(2);
  flood_indicator_->setVisible(false);
  context_bar_->addWidget(flood_indicator_);

  // Live severity KPI tiles (backlog 2.3): critical / warning / unacknowledged,
  // ordered and coloured by the shared tile builder. Opt-in — the factory
  // returns nothing under the legacy theme.
  severity_tiles_ = events::MakeSeverityTileStrip(
      [this] {
        // The status-bar model exposes the counts as separate aggregates; the
        // alarm total is the unacknowledged count (see
        // EventStatusProvider::GetTileCounts).
        return events::SeverityTileCounts{
            .critical = status_bar_model_->GetSeverityCount(
                scada::aui::SeverityLevel::kCritical),
            .warning = status_bar_model_->GetSeverityCount(
                scada::aui::SeverityLevel::kWarning),
            .unacknowledged = status_bar_model_->GetAlarmCount()};
      },
      context_bar_);
  if (severity_tiles_)
    context_bar_->addWidget(severity_tiles_);

  // No identity/connection cluster here. Who/where context (user·role,
  // connection, server latency, endpoint·build) is stated once, in the status
  // strip — mirroring it in the top bar duplicated four cells verbatim and
  // costs a place to look without adding a fact (shell.md §3: don't show the
  // same metric in two prominent places). The top bar carries only alarm state,
  // which needs pre-attentive prominence.

  auto refresh = [this] {
    if (severity_tiles_)
      severity_tiles_->Refresh();

    // Flood escalation: a single prominent state pill when the unacknowledged
    // count crosses the flood threshold, so a flood reads as a state, not a
    // scroll.
    const int alarm_count = status_bar_model_->GetAlarmCount();
    const bool flood = events::IsAlarmFlood(alarm_count);
    flood_indicator_->setVisible(flood);
    if (flood) {
      flood_indicator_->setText(
          QStringLiteral(" %1 (%2) ")
              .arg(QString::fromStdU16String(Translate("Alarm flood")))
              .arg(alarm_count));
      const std::optional<scada::aui::Color> color =
          scada::aui::SeverityColor(scada::aui::SeverityLevel::kCritical);
      // A stylesheet, not the palette: the pill is a rounded fill, and
      // border-radius is one of the few things QPalette cannot express.
      //
      // The fill is a process-semantic colour, fixed by ISA-18.2 and exempt
      // from platform styling — but the text on it is derived from the fill
      // rather than baked. The dark and light critical tokens differ enough in
      // luminance that one constant cannot serve both, which the previous
      // hard-coded #ffffff did not account for.
      const QColor fill = color ? color->qcolor() : QColor{0xe8, 0x5a, 0x52};
      flood_indicator_->setStyleSheet(
          QStringLiteral(
              "background:%1;color:%2;border-radius:9px;font-weight:700;")
              .arg(fill.name(), scada::aui::ReadableTextOn(fill).name()));
    }
  };
  refresh();
  context_bar_connection_ = status_bar_model_->SubscribePanesChanged(
      [refresh](int, int) { refresh(); });

  addToolBar(Qt::TopToolBarArea, context_bar_);
  // Force the command toolbar onto its own row below the context bar.
  addToolBarBreak(Qt::TopToolBarArea);
}

namespace {

// The rail glyph for each mode. Kept beside the mode table rather than in it,
// so pane_modes stays Qt-free.
ActivityBar::Icon ModeIconKind(PaneModeId id) {
  switch (id) {
    case PaneModeId::kObjects:
      return ActivityBar::Icon::kObjects;
    case PaneModeId::kDevices:
      return ActivityBar::Icon::kDevices;
    case PaneModeId::kFiles:
      return ActivityBar::Icon::kFiles;
    case PaneModeId::kNodes:
      return ActivityBar::Icon::kNodes;
    case PaneModeId::kAdministration:
      return ActivityBar::Icon::kAdministration;
  }
  return ActivityBar::Icon::kNone;
}

// The pinned utilities at the foot of the rail. A dedicated enumeration rather
// than the command ids themselves, because those numeric values are reused
// across unrelated symbols (see resources/common_resources.h) and a rail id is
// compared, not dispatched.
enum class RailUtility {
  kSettings,
  kUsers,
};

constexpr int RailUtilityId(RailUtility utility) {
  return static_cast<int>(utility);
}

}  // namespace

void MainWindow::CreateActivityBar() {
  std::vector<ActivityBar::Mode> modes;
  for (const PaneMode& mode : GetPaneModes()) {
    modes.push_back(ActivityBar::Mode{.id = mode.id,
                                      .label = Translate(mode.label),
                                      .icon_kind = ModeIconKind(mode.id)});
  }

  activity_bar_ = new ActivityBar(this, std::move(modes),
                                  [this](PaneModeId id) { SetPaneMode(id); });

  // The third rail zone (activity-rail.html). Settings opens the preferences
  // dialog and Users the account list — both in the current page, so neither
  // disturbs the page marker.
  activity_bar_->SetUtilities(
      {ActivityBar::Utility{.utility_id = RailUtilityId(RailUtility::kSettings),
                            .label = Translate("Settings"),
                            .icon_kind = ActivityBar::Icon::kSettings},
       ActivityBar::Utility{.utility_id = RailUtilityId(RailUtility::kUsers),
                            .label = Translate("Users"),
                            .icon_kind = ActivityBar::Icon::kUsers}},
      [this](int utility_id) {
        switch (static_cast<RailUtility>(utility_id)) {
          case RailUtility::kSettings:
            ExecuteShellCommand(ID_SETTINGS_DIALOG);
            return;
          case RailUtility::kUsers:
            ExecuteShellCommand(ID_USERS_VIEW);
            return;
        }
      });

  auto* rail = new QToolBar(this);
  rail->setObjectName(QStringLiteral("ActivityRail"));
  rail->setMovable(false);
  rail->setFloatable(false);
  rail->setContextMenuPolicy(Qt::PreventContextMenu);
  rail->addWidget(activity_bar_);
  addToolBar(Qt::LeftToolBarArea, rail);
}

void MainWindow::WireRailPages() {
  page_switcher_ = std::make_unique<PageSwitcher>(
      PageSwitcherContext{.executor_ = executor_,
                          .profile_ = profile_,
                          .main_window_ = *this,
                          .main_window_manager_ = main_window_manager_,
                          .dialog_service_ = dialog_service_});

  activity_bar_->SetPageCallbacks(
      [this](int page_id) { page_switcher_->ActivatePage(page_id); },
      [this] { ExecuteShellCommand(ID_PAGE_NEW); },
      [this](int page_id, const QPoint& global_pos) {
        ShowPageContextMenu(page_id, global_pos);
      },
      [this](int page_id, int new_index) {
        page_switcher_->ReorderPage(page_id, new_index);
        // The Page menu reads the same ordered list, so it follows without
        // any further wiring.
        RefreshRailPages();
      });

  RefreshRailPages();
}

void MainWindow::RefreshRailPages() {
  if (!activity_bar_ || !page_switcher_)
    return;

  std::vector<ActivityBar::PageButton> buttons;
  int active_page_id = 0;
  for (const PageEntry& entry : page_switcher_->ListPages()) {
    buttons.push_back(
        ActivityBar::PageButton{.page_id = entry.page_id,
                                .title = entry.title,
                                .icon_key = entry.icon,
                                .opened_elsewhere = entry.opened_elsewhere});
    if (entry.current)
      active_page_id = entry.page_id;
  }

  activity_bar_->SetPages(std::move(buttons));
  activity_bar_->SetActivePage(active_page_id);
}

void MainWindow::ExecuteShellCommand(unsigned command_id) {
  // Route through the shell's command resolution rather than reimplementing
  // New / Rename / Delete: PageCommands owns them, and the Page menu and the
  // Ctrl-K palette reach them the same way.
  if (CommandHandler* handler = ResolveViewCommand(command_id)) {
    if (handler->IsCommandEnabled(command_id))
      handler->ExecuteCommand(command_id);
  }
}

std::string MainWindow::PageIconFor(int page_id) const {
  if (!page_switcher_)
    return {};
  for (const PageEntry& entry : page_switcher_->ListPages()) {
    if (entry.page_id == page_id)
      return entry.icon;
  }
  return {};
}

void MainWindow::SetPageIcon(int page_id, std::string_view key) {
  if (!page_switcher_)
    return;
  page_switcher_->SetPageIcon(page_id, key);
  // The rail reads the icon from the profile, so redraw the buttons rather
  // than mutating the one that was clicked — same path a rename takes.
  RefreshRailPages();
}

void MainWindow::ShowPageContextMenu(int page_id, const QPoint& global_pos) {
  QMenu menu{this};

  // Rename and Delete act on the *current* page (that is what ID_PAGE_RENAME
  // and ID_PAGE_DELETE mean), so switch to the right-clicked page first when
  // it is not already open. Anything else would silently rename the wrong one.
  const bool is_current = page_id == current_page().id;

  auto add = [&](unsigned command_id, const char* label, bool enabled) {
    QAction* action =
        menu.addAction(QString::fromStdU16String(Translate(label)));
    action->setEnabled(enabled);
    connect(action, &QAction::triggered, this,
            [this, command_id] { ExecuteShellCommand(command_id); });
  };

  // Opening the right-clicked page is the menu's primary action, so it leads
  // and is bold. Absent when that page is already open — "Open page" on the
  // page you are looking at would mean the revert, which is not what the
  // reader would expect from it.
  const std::vector<PageEntry> pages = page_switcher_->ListPages();
  const auto entry = std::ranges::find(pages, page_id, &PageEntry::page_id);
  if (!is_current && entry != pages.end() && !entry->opened_elsewhere) {
    QAction* open =
        menu.addAction(QString::fromStdU16String(Translate("Open page")));
    connect(open, &QAction::triggered, this,
            [this, page_id] { page_switcher_->ActivatePage(page_id); });
    menu.setDefaultAction(open);
    menu.addSeparator();
  }

  add(ID_PAGE_RENAME, "Rename", is_current);
  add(ID_PAGE_DUPLICATE, "Duplicate", is_current);

  // Reordering acts on the right-clicked page directly, the way the icon
  // submenu does: moving a page does not require having it open, and forcing a
  // switch first would be a worse way to rearrange a list. Each end of the list
  // disables its own direction rather than silently doing nothing.
  const int index =
      entry == pages.end() ? -1 : static_cast<int>(entry - pages.begin());
  auto add_move = [&](const char* label, int target, bool enabled) {
    QAction* action =
        menu.addAction(QString::fromStdU16String(Translate(label)));
    action->setEnabled(enabled);
    connect(action, &QAction::triggered, this, [this, page_id, target] {
      page_switcher_->ReorderPage(page_id, target);
      RefreshRailPages();
    });
  };
  add_move("Move up", index - 1, index > 0);
  add_move("Move down", index + 1,
           index >= 0 && index + 1 < static_cast<int>(pages.size()));

  // The icon is a property of the page, not of the current one, so unlike
  // Rename and Delete it acts on the right-clicked page directly — no need to
  // switch to it first, and no reason to disable it when another page is open.
  QMenu* icon_menu = menu.addMenu(QString::fromStdU16String(Translate("Icon")));
  const std::string current_icon = PageIconFor(page_id);

  QAction* none_action =
      icon_menu->addAction(QString::fromStdU16String(Translate("None")));
  none_action->setCheckable(true);
  none_action->setChecked(current_icon.empty());
  connect(none_action, &QAction::triggered, this,
          [this, page_id] { SetPageIcon(page_id, {}); });
  icon_menu->addSeparator();

  for (const PageIcon& icon : GetPageIcons()) {
    QAction* action =
        icon_menu->addAction(QString::fromStdU16String(Translate(icon.label)));
    action->setCheckable(true);
    action->setChecked(current_icon == icon.key);
    const std::string key{icon.key};
    connect(action, &QAction::triggered, this,
            [this, page_id, key] { SetPageIcon(page_id, key); });
  }

  // Delete sits last, behind its own separator: the destructive item is kept
  // away from the ones above it so it is not reached by muscle memory.
  menu.addSeparator();
  add(ID_PAGE_DELETE, "Delete page", is_current);

  menu.addSeparator();
  add(ID_PAGE_NEW, "New page", true);

  menu.exec(global_pos);
}

bool MainWindow::SelectPaneModeForPane(std::string_view window_type) {
  const PaneMode* mode = FindPaneModeOwningPaneType(window_type);
  if (!mode)
    return false;
  SetPaneMode(mode->id);
  return true;
}

PaneModeId MainWindow::ActivePaneMode() {
  const std::string& key = GetPrefs().pane_mode;
  if (const PaneMode* mode = FindPaneModeByKey(key)) {
    // A profile can carry a mode the current user may not open. Fall back
    // rather than presenting an empty sidebar with no way out.
    if (!mode->requires_admin || IsPaneModeAvailable(mode->id))
      return mode->id;
    return PaneModeId::kObjects;
  }
  // No mode recorded — this profile predates the rail. Infer one from the page
  // so the operator keeps the panes they had.
  return InferPaneModeFromPage(current_page());
}

bool MainWindow::IsPaneModeAvailable(PaneModeId id) {
  const PaneMode& mode = GetPaneMode(id);
  if (!mode.requires_admin)
    return true;
  // Ask the same resolution the menus use, so the rail and the More menu agree
  // by construction: a WIN_REQUIRES_ADMIN view resolves to no handler without
  // the Configure right (MainWindowCommandRouter::GetCommandHandler).
  for (std::string_view pane_type : mode.pane_types) {
    const WindowInfo* info = FindWindowInfoByName(pane_type);
    if (!info || !commands().GetCommandHandler(info->command_id))
      return false;
  }
  return true;
}

void MainWindow::SetPaneMode(PaneModeId id) {
  if (!IsPaneModeAvailable(id))
    return;

  const PaneMode& mode = GetPaneMode(id);

  // What the sidebar shows right now, in the rail's vocabulary.
  std::vector<std::string_view> open_panes;
  for (std::string_view pane_type : GetModeOwnedPaneTypes()) {
    if (FindViewByType(pane_type))
      open_panes.push_back(pane_type);
  }

  const PaneModeDelta delta = ComputePaneModeDelta(mode, open_panes);

  {
    // Closing and opening panes fires OnViewClosed / OnActiveViewChanged; let
    // the switch finish before re-deriving the marker from a half-applied set.
    scada::base::AutoReset<bool> applying{&applying_pane_mode_, true};

    // Close first, then open in declared order — AddDockView tabifies onto the
    // first dock already in the area, so opening early would tab the new panes
    // onto ones that are about to disappear.
    for (std::string_view pane_type : delta.to_close) {
      if (const WindowInfo* info = FindWindowInfoByName(pane_type))
        ClosePane(*info);
    }
    for (std::string_view pane_type : delta.to_open) {
      if (const WindowInfo* info = FindWindowInfoByName(pane_type))
        OpenPaneSync(*info, /*activate=*/false);
    }

    FrontPrimaryPane(mode);
  }

  GetPrefs().pane_mode = std::string{mode.key};
  RefreshPaneModeMarker();
}

void MainWindow::FrontPrimaryPane(const PaneMode& mode) {
  if (mode.pane_types.empty())
    return;
  if (OpenedViewInterface* primary = FindViewByType(mode.pane_types.front()))
    ActivateView(*primary);
}

void MainWindow::ApplyPaneModeToCurrentWindow() {
  if (!activity_bar_)
    return;
  SetPaneMode(ActivePaneMode());
}

void MainWindow::RefreshPaneModeMarker() {
  if (!activity_bar_)
    return;

  for (const PaneMode& mode : GetPaneModes())
    activity_bar_->SetModeAvailable(mode.id, IsPaneModeAvailable(mode.id));

  std::vector<std::string_view> open_panes;
  for (std::string_view pane_type : GetModeOwnedPaneTypes()) {
    if (FindViewByType(pane_type))
      open_panes.push_back(pane_type);
  }

  // An exact match is the honest answer; anything else and the sidebar is not
  // showing a mode, so the rail must not claim one.
  for (const PaneMode& mode : GetPaneModes()) {
    if (mode.pane_types.size() != open_panes.size())
      continue;
    if (std::ranges::equal(mode.pane_types, open_panes)) {
      activity_bar_->SetActiveMode(mode.id);
      return;
    }
  }

  // Partial match: fall back to the mode owning whatever pane is active, so a
  // manually closed sibling still leaves the rail pointing somewhere true.
  if (OpenedView* active = GetActiveView()) {
    if (const PaneMode* mode =
            FindPaneModeOwningPaneType(active->window_info().name)) {
      activity_bar_->SetActiveMode(mode->id);
      return;
    }
  }

  activity_bar_->SetActiveMode(std::nullopt);
}

void MainWindow::ShowSettingsDialog() {
  // The shell holds its menu as the MenuModel interface, and the settings
  // items are a detail of the real model. A test harness can install a
  // different one, so this asks rather than asserts: no menu model of ours
  // means no preferences to render, which is a shell without settings, not a
  // bug to panic on.
  auto* menu_model = dynamic_cast<MainMenuModel*>(main_menu_model_.get());
  if (!menu_model)
    return;

  // Modal to this window. Preferences are per-profile and apply live, so a
  // second one open beside the first would show two views of one state.
  SettingsDialog dialog{this, menu_model->settings_model()};
  dialog.exec();
}

void MainWindow::RefreshUtilityMarker() {
  if (!activity_bar_)
    return;

  // Users is admin-only, resolved through the same command router the menus
  // use so the rail cannot offer a door the shell would refuse to open. Done
  // here rather than once at construction: the rail is built before the
  // modules finish registering their views, so a one-shot check would read a
  // registry that is not populated yet and hide the button for everyone.
  activity_bar_->SetUtilityAvailable(
      RailUtilityId(RailUtility::kUsers),
      ResolveViewCommand(ID_USERS_VIEW) != nullptr);

  // Only Users can ever be marked. Settings opens a modal dialog, which is not
  // a thing the workspace can be showing — so its button is an action, and an
  // action never carries a marker.
  OpenedView* active = GetActiveView();
  const bool users_active =
      active && active->window_info().name == std::string_view{"Users"};
  activity_bar_->SetActiveUtility(
      users_active ? std::optional{RailUtilityId(RailUtility::kUsers)}
                   : std::nullopt);
}

void MainWindow::OnViewClosed(OpenedView& view) {
  const bool was_owned_pane =
      FindPaneModeOwningPaneType(view.window_info().name) != nullptr;
  BaseMainWindow::OnViewClosed(view);
  if (was_owned_pane && !applying_pane_mode_ &&
      !view_manager_->is_closing_page()) {
    RefreshPaneModeMarker();
  }
}

void MainWindow::OnActiveViewChanged(OpenedView* view) {
  BaseMainWindow::OnActiveViewChanged(view);
  if (!applying_pane_mode_ && view &&
      FindPaneModeOwningPaneType(view->window_info().name)) {
    RefreshPaneModeMarker();
  }
  RefreshUtilityMarker();
  RefreshBreadcrumb();
}

void MainWindow::RefreshBreadcrumb() {
  if (!breadcrumb_)
    return;

  // Page and view are the path; the selected object is the subject. Only the
  // ends are emphasised — the middle is context, per the mockup screens.
  OpenedView* active = GetActiveView();
  SelectionModel* selection =
      active ? active->controller().GetSelectionModel() : nullptr;

  // A multiple selection has no single subject to name, and naming the first of
  // several would be a quiet lie about what the view is pointed at.
  QString subject;
  if (selection && !selection->empty() && !selection->multiple()) {
    subject =
        QString::fromStdU16String(ToString16(selection->node().display_name()));
  }

  const Breadcrumb::Segment segments[] = {
      {.label =
           QString::fromStdU16String(view_manager_->current_page().GetTitle()),
       .strong = true},
      {.label = active ? QString::fromStdU16String(active->GetWindowTitle())
                       : QString{}},
      {.label = subject, .strong = true},
  };
  breadcrumb_->SetSegments(segments);
}

void MainWindow::OpenTag(const scada::NodeId& node_id,
                         const std::u16string& title) {
  const WindowInfo* info = FindWindowInfoByName("Table");
  if (!info)
    return;
  CoSpawn(executor_,
          [this, def = MakeWindowDefinition(
                     info, {node_id}, title)]() mutable -> Awaitable<void> {
            co_await OpenView(def, /*make_active=*/true);
          });
}

void MainWindow::ShowCommandPalette(const QString& initial_text) {
  // Address-space tags as extra palette entries; activating one opens it in a
  // table view. The browse was started at construction, so tags() is usually
  // already populated here (empty on the very first open of a fresh session).
  std::vector<CommandPalette::ExtraItem> extras;
  if (tag_search_index_) {
    const std::u16string tag_detail = Translate("tag");
    for (const TagSearchIndex::Tag& tag : tag_search_index_->tags()) {
      extras.push_back({tag.name, tag_detail,
                        [this, node_id = tag.node_id, title = tag.name] {
                          OpenTag(node_id, title);
                        }});
    }
  }

  auto* palette = new CommandPalette(
      this, ui_command_registry_.command_manager(),
      [this](unsigned command_id) -> CommandHandler* {
        return ResolveViewCommand(command_id);
      },
      std::move(extras));
  palette->setAttribute(Qt::WA_DeleteOnClose);
  if (!initial_text.isEmpty())
    palette->PresetFilter(initial_text);
  palette->show();
  palette->raise();
  palette->activateWindow();
}

void MainWindow::CreateToolbar() {
  auto& command_manager = ui_command_registry_.command_manager();
  for (auto* command_info : command_manager.commands()) {
    if (!command_info->show_in_toolbar) {
      continue;
    }

    bool collapsible = !CanExpandCommandCategory(command_info->category);
    auto* action = new QAction(
        QString::fromStdU16String(command_info->GetShortTitle()), this);
    action->setPriority(collapsible ? QAction::LowPriority
                                    : QAction::NormalPriority);
    action->setVisible(false);
    if (command_info->image_id != 0)
      action->setIcon(QIcon(LoadPixmap(command_info->image_id)));
    action->setCheckable(command_info->checkable());
    if (command_info->shortcut.has_value())
      action->setShortcut(ToQKeySequence(*command_info->shortcut));
    auto command_id = command_info->command_id;
    QObject::connect(action, &QAction::triggered,
                     [this, command_id](bool checked) {
                       auto* handler = ResolveViewCommand(command_id);
                       if (handler && handler->IsCommandEnabled(command_id))
                         handler->ExecuteCommand(command_id);
                     });
    action_map_.emplace(command_info->command_id, action);
    action_command_ids_.emplace(action, command_info->command_id);
  }

  toolbar_ = new QToolBar(this);
  toolbar_->setObjectName(QStringLiteral("CommandToolbar"));
  toolbar_->setVisible(GetPrefs().toolbar);
  toolbar_->setWindowTitle(tr("Toolbar"));
  // Icon-only buttons, sized to the 16px source icons so the toolbar stays
  // compact (the platform default icon size is larger and would upscale the
  // icons and inflate the button height). Actions without an icon fall back to
  // showing their text.
  toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
  toolbar_->setIconSize(QSize(16, 16));

  {
    // Action order is important.
    int last_category = -1;
    for (auto* command_info : command_manager.commands()) {
      if (!command_info->show_in_toolbar) {
        continue;
      }

      auto* action = FindAction(command_info->command_id);
      if (CanExpandCommandCategory(command_info->category)) {
        toolbar_->addAction(action);
        if (last_category != -1 && last_category != command_info->category) {
          toolbar_->addSeparator();
        }
      } else {
        auto& category_action = category_actions_[command_info->category];
        if (!category_action.menu) {
          auto* button = new QToolButton(toolbar_);
          auto* menu = new QMenu(this);
          auto title = GetCommandCategoryTitle(command_info->category);
          auto text = QString::fromUtf16(title.data(), title.size());
          button->setMenu(menu);
          button->setPopupMode(QToolButton::InstantPopup);
          button->setText(text);
          category_action.menu = menu;
          category_action.toolbar_action = toolbar_->addWidget(button);
          connect(menu, &QMenu::aboutToShow,
                  [this, menu] { UpdateMenuActions(*menu); });
        }
        category_action.menu->addAction(action);
      }
      last_category = command_info->category;
    }
  }

  addToolBar(Qt::TopToolBarArea, toolbar_);
}

void MainWindow::SetWindowFlashing(bool flashing) {}

QString MainWindow::ControlUnavailableReason() {
  // Asked only while the Control button is disabled. The node answers for its
  // own shape through the same rule the write command's gates use
  // (GetWriteBlock); the command's only other gate is the session's Write
  // permission, so a node that would accept control can be blocked by nothing
  // else.
  OpenedView* active = GetActiveView();
  SelectionModel* selection =
      active ? active->controller().GetSelectionModel() : nullptr;
  if (!selection || selection->empty() || selection->multiple())
    return {};

  const WriteBlock block = GetWriteBlock(selection->node());
  return QString::fromStdU16String(
      block != WriteBlock::kNone
          ? Translate(WriteBlockText(block))
          : Translate("Controlling requires the Control privilege"));
}

void MainWindow::CreateInspectorPanel() {
  // The control action reuses the selection-scoped write/control command
  // (ID_WRITE) — the existing two-stage confirm — resolved against the active
  // selection exactly like the toolbar/menu path. The event card's
  // Acknowledge action likewise reuses the journal's own selection-scoped
  // command (ID_ACKNOWLEDGE_CURRENT), and Go-to-source the selection-scoped
  // open-graph command (ID_OPEN_GRAPH) — the event selection carries the
  // alarm's source node, so the graph opens on it.
  auto resolve_write = [this]() -> CommandHandler* {
    return ResolveViewCommand(ID_WRITE);
  };
  auto resolve_acknowledge = [this]() -> CommandHandler* {
    return ResolveViewCommand(ID_ACKNOWLEDGE_CURRENT);
  };
  auto resolve_open_graph = [this]() -> CommandHandler* {
    return ResolveViewCommand(ID_OPEN_GRAPH);
  };

  inspector_ = new InspectorPanel(InspectorPanelContext{
      .on_control =
          [resolve_write] {
            CommandHandler* handler = resolve_write();
            if (handler && handler->IsCommandEnabled(ID_WRITE))
              handler->ExecuteCommand(ID_WRITE);
          },
      .is_control_enabled =
          [resolve_write] {
            CommandHandler* handler = resolve_write();
            return handler && handler->IsCommandEnabled(ID_WRITE);
          },
      .control_reason = [this] { return ControlUnavailableReason(); },
      .on_acknowledge =
          [resolve_acknowledge] {
            CommandHandler* handler = resolve_acknowledge();
            if (handler && handler->IsCommandEnabled(ID_ACKNOWLEDGE_CURRENT))
              handler->ExecuteCommand(ID_ACKNOWLEDGE_CURRENT);
          },
      .is_acknowledge_enabled =
          [resolve_acknowledge] {
            CommandHandler* handler = resolve_acknowledge();
            return handler && handler->IsCommandEnabled(ID_ACKNOWLEDGE_CURRENT);
          },
      .on_go_to_source =
          [resolve_open_graph] {
            CommandHandler* handler = resolve_open_graph();
            if (handler && handler->IsCommandEnabled(ID_OPEN_GRAPH))
              handler->ExecuteCommand(ID_OPEN_GRAPH);
          },
      .is_go_to_source_enabled =
          [resolve_open_graph] {
            CommandHandler* handler = resolve_open_graph();
            return handler && handler->IsCommandEnabled(ID_OPEN_GRAPH);
          }});

  auto* dock =
      new QDockWidget(QString::fromStdU16String(Translate("Inspector")), this);
  dock->setObjectName(QStringLiteral("InspectorDock"));
  dock->setWidget(inspector_);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  inspector_dock_ = dock;
}

void MainWindow::CreateDiagnosticsPanel() {
  // The device-wide actions each reuse a selection-scoped device command,
  // resolved against the active selection exactly like the toolbar/menu path —
  // Metrics trend (ID_OPEN_DEVICE_METRICS) and Open log (ID_OPEN_EVENTS, which
  // opens the event journal).
  //
  // "Reconnect now" is NOT one of them. It used to be wired to ID_ITEM_ENABLE,
  // which re-enables a disabled device and is a different action wearing the
  // mockup's label. It is now the protocol registry's link action: an OPC UA
  // Method on the device's parent LINK (ADR 0007), supplied per device by the
  // panel itself, because only some protocols have one and only some devices
  // have a link.
  auto make_action = [this](unsigned command_id,
                            std::u16string label) -> DiagnosticAction {
    auto resolve = [this, command_id]() -> CommandHandler* {
      return ResolveViewCommand(command_id);
    };
    return DiagnosticAction{
        .label = std::move(label),
        .execute =
            [resolve, command_id] {
              CommandHandler* handler = resolve();
              if (handler && handler->IsCommandEnabled(command_id))
                handler->ExecuteCommand(command_id);
            },
        .is_enabled =
            [resolve, command_id] {
              CommandHandler* handler = resolve();
              return handler && handler->IsCommandEnabled(command_id);
            },
        // These are gated by the selection, not by a right, so the reason an
        // operator can act on is to change what is selected.
        .disabled_reason =
            Translate("Not available for the current selection")};
  };

  DeviceDiagnosticsPanelContext context;
  context.actions.push_back(
      make_action(ID_OPEN_DEVICE_METRICS, Translate("Metrics trend")));
  context.actions.push_back(make_action(ID_OPEN_EVENTS, Translate("Open log")));
  context.call_link_method = call_node_method_;
  context.can_call = has_call_permission_;

  diagnostics_ = MakeDeviceDiagnosticsPanel(std::move(context));
  if (!diagnostics_)
    return;

  auto* dock = new QDockWidget(
      QString::fromStdU16String(Translate("Device diagnostics")), this);
  dock->setObjectName(QStringLiteral("DeviceDiagnosticsDock"));
  dock->setWidget(diagnostics_);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  // Share the right dock area with the Inspector; the device-diagnostics tab
  // comes to the front only when a device is selected.
  if (inspector_dock_)
    tabifyDockWidget(inspector_dock_, dock);
}

void MainWindow::CreateUserAccessPanel() {
  user_access_ = MakeUserAccessPanel();
  if (!user_access_)
    return;

  auto* dock = new QDockWidget(
      QString::fromStdU16String(Translate("Access rights")), this);
  dock->setObjectName(QStringLiteral("UserAccessDock"));
  dock->setWidget(user_access_);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  // Shares the right dock; fronted only when a user is selected.
  if (inspector_dock_)
    tabifyDockWidget(inspector_dock_, dock);
}

void MainWindow::CreateTransmissionRulePanel() {
  transmission_rule_ = MakeTransmissionRuleInspector();
  if (!transmission_rule_)
    return;

  auto* dock = new QDockWidget(
      QString::fromStdU16String(Translate("Transmission rule")), this);
  dock->setObjectName(QStringLiteral("TransmissionRuleDock"));
  dock->setWidget(transmission_rule_);
  // No ApplyHandler is wired here: the shell has no TaskManager, so the panel
  // presents the rule read-only. Editing rides the existing transmission grid,
  // which writes SourceAddress through its own TaskManager.
  addDockWidget(Qt::RightDockWidgetArea, dock);
  // Shares the right dock; fronted only when a transmission rule is selected.
  if (inspector_dock_)
    tabifyDockWidget(inspector_dock_, dock);
}

void MainWindow::TabifySpecialistDocks() {
  if (!inspector_dock_)
    return;

  // Opening a page restores the page's persisted QMainWindow dock state. These
  // panels carry permanent object names, so they are part of that blob, and a
  // state saved while they sat stacked vertically silently undoes the tabify
  // done when they were created — which is how four title bars ended up
  // sharing the right column, each one squeezing the panel above it. Re-tabify
  // after every restore.
  for (const char* name :
       {"DeviceDiagnosticsDock", "UserAccessDock", "TransmissionRuleDock"}) {
    if (auto* dock = findChild<QDockWidget*>(QString::fromLatin1(name));
        dock && dock != inspector_dock_) {
      tabifyDockWidget(inspector_dock_, dock);
    }
  }

  // Tabifying leaves the last dock added on top, so an empty specialist panel
  // would greet the operator. Front the Inspector, which speaks for any
  // selection.
  inspector_dock_->raise();
}

void MainWindow::OpenPage(const Page& page) {
  if (activity_bar_) {
    // Conform the page to the active mode BEFORE the view manager builds it.
    // ViewManager::OpenPage creates every visible window and only then restores
    // the dock-state blob, so a pane opened afterwards would sit outside the
    // restored layout at Qt's default width.
    Page conformed = page;
    ApplyPaneModeToPage(conformed, GetPaneMode(ActivePaneMode()));
    BaseMainWindow::OpenPage(conformed);
  } else {
    // Legacy theme: no rail, so the page's own pane set is authoritative.
    BaseMainWindow::OpenPage(page);
  }

  TabifySpecialistDocks();

  // The restored dock blob carries whichever pane was fronted when the page was
  // last saved, which need not be the mode's subject — a page saved while
  // Portfolio was on top reopens showing Portfolio, not Objects. Re-assert it
  // after TabifySpecialistDocks, which raises the Inspector and so must not be
  // the last thing to touch dock stacking.
  if (activity_bar_)
    FrontPrimaryPane(GetPaneMode(ActivePaneMode()));

  // Every page switch funnels through here — the Pages menu, the Pages pane,
  // the page commands, and the startup restore — so this is the one place the
  // marker has to be re-derived, and the one place the page list is known to
  // be stale (New and Delete both end in an OpenPage).
  RefreshRailPages();
  RefreshPaneModeMarker();
}

void MainWindow::OnSelectionChanged() {
  // The breadcrumb's last segment is the selection, so it moves with it.
  RefreshBreadcrumb();

  if (inspector_ || diagnostics_ || user_access_ || transmission_rule_) {
    OpenedView* active = GetActiveView();
    SelectionModel* selection =
        active ? active->controller().GetSelectionModel() : nullptr;
    if (inspector_) {
      if (selection)
        inspector_->ShowSelection(*selection);
      else
        inspector_->Clear();
    }
    if (diagnostics_) {
      // Only a single device selection carries diagnostics; anything else
      // clears the panel.
      if (selection && !selection->empty() && !selection->multiple() &&
          IsInstanceOf(selection->node(), scada::devices::id::DeviceType)) {
        diagnostics_->ShowDevice(selection->node(),
                                 selection->timed_data_service());
      } else {
        diagnostics_->Clear();
      }
    }
    if (user_access_) {
      // A single user-node selection fills the RBAC panel; anything else
      // clears. Without an attribute service there is no way to read what the
      // Roles grant, and the panel must not fall back to assuming a map — so
      // the selection clears rather than showing an invented breakdown.
      if (selection && !selection->empty() && !selection->multiple() &&
          attribute_service_ &&
          IsInstanceOf(selection->node(), scada::security::id::UserType)) {
        // The node carries the account's NAME; the panel reads the Roles
        // themselves from the RoleSet, and what they grant from the server's
        // published RolePermissions.
        user_access_->ShowUser(selection->node(), *node_service_,
                               *attribute_service_, executor_);
      } else {
        user_access_->Clear();
      }
    }
    if (transmission_rule_) {
      // A single transmission-item selection fills the rule inspector; anything
      // else clears.
      if (selection && !selection->empty() && !selection->multiple() &&
          IsInstanceOf(selection->node(),
                       scada::devices::id::TransmissionItemType)) {
        transmission_rule_->ShowRule(selection->node());
      } else {
        transmission_rule_->Clear();
      }
    }
  }

  for (const auto& [command_id, action] : action_map_) {
    UpdateAction(*action, command_id, ActionChangeMask::AllButTitle);
  }

  for (const auto& [_, category] : category_actions_) {
    bool has_visible_actions =
        std::ranges::any_of(category.menu->actions(), &QAction::isVisible);
    category.toolbar_action->setVisible(has_visible_actions);
  }

  bool adjacent_separator = false;
  for (auto* toolbar_action : toolbar_->actions()) {
    if (toolbar_action->isSeparator()) {
      toolbar_action->setVisible(!adjacent_separator);
      adjacent_separator = true;
    } else if (toolbar_action->isVisible()) {
      adjacent_separator = false;
    }
  }
}

void MainWindow::SetToolbarPosition(unsigned position) {}

void MainWindow::OnShowTabPopupMenu(OpenedView& view,
                                    const scada::aui::Point& point) {
  QMenu menu;
  BuildMenu(menu, *tab_popup_menu_);
  menu.exec(point);
}

QAction* MainWindow::FindAction(unsigned command_id) {
  auto i = action_map_.find(command_id);
  return i == action_map_.end() ? nullptr : i->second;
}

void MainWindow::OnActionChanged(Action& action, ActionChangeMask change_mask) {
  auto i = action_map_.find(action.command_id());
  if (i != action_map_.end())
    UpdateAction(*i->second, i->first, change_mask);
}

void MainWindow::UpdateAction(QAction& action,
                              unsigned command_id,
                              ActionChangeMask change_mask) {
  if (static_cast<unsigned>(change_mask) &
      static_cast<unsigned>(ActionChangeMask::Title)) {
    if (const auto* a =
            ui_command_registry_.action_manager().FindAction(command_id)) {
      action.setText(QString::fromStdU16String(a->GetTitle()));
    }
  }

  const CommandHandler* handler = ResolveViewCommand(command_id);
  action.setVisible(!!handler);
  if (handler) {
    bool enabled = handler->IsCommandEnabled(command_id);
    action.setEnabled(enabled);
    if (enabled)
      action.setChecked(handler->IsCommandChecked(command_id));
  }
}

void MainWindow::UpdateMenuActions(QMenu& menu) {
  for (QAction* action : menu.actions()) {
    auto i = action_command_ids_.find(action);
    if (i != action_command_ids_.end())
      UpdateAction(*action, i->second, ActionChangeMask::All);
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  MainWindowDef& prefs = GetPrefs();
  prefs.bounds = geometry();
  prefs.state = isMaximized() ? MainWindowDef::State::kMaximized
                              : MainWindowDef::State::kNormal;

  // ModusView-s must be destroyed before MainWindow destruction, to avoid an
  // exception of unknown nature.
  BeforeClose();

  main_window_manager_.OnMainWindowClosed(window_id_);

  QMainWindow::closeEvent(event);
}

void MainWindow::ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                               const scada::aui::Point& point,
                               bool right_click) {
  QMenu menu;
  BuildDefaultPopupMenu(menu, merge_menu, *context_menu_model_);
  menu.exec(point);
}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(
    WindowDefinition& window_def) {
  return opened_view_factory_(*this, window_def);
}
