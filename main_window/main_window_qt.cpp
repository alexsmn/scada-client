#include "main_window/main_window_qt.h"

#include "aui/models/menu_model.h"
#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model.h"
#include "aui/qt/status_bar.h"
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
#include "controller/qt/selection_panel_registry.h"
#include "controller/selection_model.h"
#include "controller/series_model.h"
#include "controller/window_info.h"
#include "filesystem/file_cache.h"
#include "inspector/qt/inspector_panel.h"
#include "main_window/activity_bar_qt.h"
#include "main_window/alarm_state_cluster_qt.h"
#include "main_window/breadcrumb_qt.h"
#include "main_window/command_palette_launcher.h"
#include "main_window/command_shortcuts_qt.h"
#include "main_window/context_bar_qt.h"
#include "main_window/main_menu/main_menu_model.h"
#include "main_window/main_window_command_router.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_util.h"
#include "main_window/menu_bar_qt.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/overview_page.h"
#include "main_window/pane_mode_controller.h"
#include "main_window/rail_pages_controller.h"
#include "main_window/rail_utilities_controller.h"
#include "main_window/selection_command_router.h"
#include "main_window/status_bar/progress_controller_qt.h"
#include "main_window/view_manager.h"
#include "main_window/window_definition_builder.h"
#include "modules/inspector/limit_band.h"
#include "modules/write/write_availability.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"
#include "scada/standard_node_ids.h"
#include "settings/qt/settings_panel.h"
#include "ui/common/client_utils.h"
#include "ui/qt/client_utils_qt.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>
#include <QShortcut>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>

#include <ranges>
#include <unordered_set>

namespace {

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

// Drives PaneModeController through this window: pane names resolve to
// WindowInfo here, and the rail is the controller's output.
class MainWindow::PaneModeHostImpl final : public PaneModeHost {
 public:
  explicit PaneModeHostImpl(MainWindow& window) : window_{window} {}

  bool IsPaneOpen(std::string_view pane_type) override {
    return window_.FindViewByType(pane_type) != nullptr;
  }

  bool CanOpenPane(std::string_view pane_type) override {
    const WindowInfo* info = FindWindowInfoByName(pane_type);
    return info && window_.commands().GetCommandHandler(info->command_id);
  }

  void ClosePane(std::string_view pane_type) override {
    if (const WindowInfo* info = FindWindowInfoByName(pane_type))
      window_.ClosePane(*info);
  }

  void OpenPane(std::string_view pane_type) override {
    if (const WindowInfo* info = FindWindowInfoByName(pane_type))
      window_.OpenPaneSync(*info, /*activate=*/false);
  }

  void FrontPane(std::string_view pane_type) override {
    if (OpenedViewInterface* pane = window_.FindViewByType(pane_type))
      window_.ActivateView(*pane);
  }

  std::string_view ActivePaneType() override {
    OpenedView* active = window_.GetActiveView();
    return active ? std::string_view{active->window_info().name}
                  : std::string_view{};
  }

  const Page& CurrentPage() override { return window_.current_page(); }

  std::string& PersistedModeKey() override {
    return window_.GetPrefs().pane_mode;
  }

  void ShowModeAvailable(PaneModeId id, bool available) override {
    window_.activity_bar_->SetModeAvailable(id, available);
  }

  void ShowActiveMode(std::optional<PaneModeId> id) override {
    window_.activity_bar_->SetActiveMode(id);
  }

 private:
  MainWindow& window_;
};

MainWindow::MainWindow(MainWindowContext&& context)
    : BaseMainWindow{std::move(context), dialog_service_} {
  const MainWindowDef& prefs = GetPrefs();

  setGeometry(prefs.bounds.isNull() ? GetDefaultBounds(this) : prefs.bounds);

  dialog_service_.parent_widget = this;

  view_manager_ = std::make_unique<ViewManager>(
      *this, *static_cast<ViewManagerDelegate*>(this));
  AttachViewManager(*view_manager_);

  CreateMenuBar();
  // The workbench chrome: activity rail, top context bar, Inspector and the
  // specialist docks (shell.md §2). Structural rather than cosmetic, so
  // Settings → Colour scheme recolours it but never adds or removes it.
  CreateActivityBar();
  rail_pages_ =
      std::make_unique<RailPagesController>(RailPagesControllerContext{
          .activity_bar_ = *activity_bar_,
          .page_switcher_ = {.executor_ = executor_,
                             .profile_ = profile_,
                             .main_window_ = *this,
                             .main_window_manager_ = main_window_manager_,
                             .dialog_service_ = dialog_service_},
          .execute_command_ =
              [this](unsigned command_id) { ExecuteShellCommand(command_id); },
          .menu_parent_ = *this});
  CreateContextBar();
  CreateInspectorPanel();
  CreateSelectionPanels();
  TabifySpecialistDocks();
  palette_ =
      std::make_unique<CommandPaletteLauncher>(CommandPaletteLauncherContext{
          .executor_ = executor_,
          .command_manager_ = ui_command_registry_.command_manager(),
          .resolve_command_ =
              [this](unsigned command_id) {
                return ResolveViewCommand(command_id);
              },
          .open_tag_ =
              [this](const scada::NodeId& node_id,
                     const std::u16string& title) { OpenTag(node_id, title); },
          .parent_ = *this,
          .node_service_ = node_service_,
          .session_service_ = session_service_});
  // Ctrl+C, Ctrl+V and Delete, window-wide; enabled by OnSelectionChanged.
  command_shortcuts_ = std::make_unique<CommandShortcuts>(
      ui_command_registry_.command_manager(),
      [this](unsigned command_id) { return ResolveViewCommand(command_id); },
      *this);
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
  RebuildMenuBar(*menuBar(), *main_menu_model_);

  // Init() opened the restored page, which conformed it to the active mode.
  // Bring the rail in line with what is actually on screen.
  if (pane_modes_)
    pane_modes_->RefreshMarker();
  // The first pass that can see a populated command registry, so it is also
  // what decides whether the admin-gated Users utility is offered at all.
  RefreshUtilityMarker();

  change_profile_connection_ = profile_.AddChangeObserver([this] {
    const MainWindowDef& prefs = GetPrefs();
    statusBar()->setVisible(prefs.status_bar);
  });
}

MainWindow::~MainWindow() {
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
  if (rail_pages_)
    rail_pages_->Refresh();
  // Same hook, same reason: the breadcrumb's first segment is the page title.
  RefreshBreadcrumb();
}

void MainWindow::CreateMenuBar() {
  main_menu_model_ = main_menu_factory_(*this, dialog_service_, *view_manager_,
                                        *commands_, *context_menu_model_);

  auto* menu_bar = new QMenuBar(this);
  setMenuBar(menu_bar);
  PopulateMenuBar(*menu_bar, *main_menu_model_);
}

void MainWindow::CreateStatusBar() {
  auto* status_bar = new scada::aui::StatusBar{*status_bar_model_, this};
  setStatusBar(status_bar);
  status_bar->setVisible(GetPrefs().status_bar);

  progress_controller_ =
      std::make_unique<ProgressController>(*status_bar, progress_host_);
}

void MainWindow::CreateContextBar() {
  // The status-bar model exposes the counts as separate aggregates; the alarm
  // total is the unacknowledged count (see EventStatusProvider::GetTileCounts).
  context_bar_ = new ContextBar(
      [this] {
        return events::SeverityTileCounts{
            .critical = status_bar_model_->GetSeverityCount(
                scada::aui::SeverityLevel::kCritical),
            .warning = status_bar_model_->GetSeverityCount(
                scada::aui::SeverityLevel::kWarning),
            .unacknowledged = status_bar_model_->GetAlarmCount()};
      },
      [this](const QString& initial_text) { ShowCommandPalette(initial_text); },
      this);
  context_bar_connection_ = status_bar_model_->SubscribePanesChanged(
      [this](int, int) { context_bar_->RefreshAlarmState(); });

  // Bound window-wide rather than on the field, so Ctrl-K opens the palette
  // wherever focus is.
  auto* palette_shortcut = new QShortcut(ContextBar::PaletteKey(), this);
  connect(palette_shortcut, &QShortcut::activated, this,
          [this] { ShowCommandPalette(); });

  addToolBar(Qt::TopToolBarArea, context_bar_);
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
  pane_mode_host_ = std::make_unique<PaneModeHostImpl>(*this);
  pane_modes_ = std::make_unique<PaneModeController>(*pane_mode_host_);

  // The third rail zone: the pinned utilities.
  rail_utilities_ = std::make_unique<RailUtilitiesController>(
      *activity_bar_,
      [this](unsigned command_id) { ExecuteShellCommand(command_id); },
      [this](unsigned command_id) { return ResolveViewCommand(command_id); });

  // The rail IS the toolbar now (shell.md §9, partial), rather than a custom
  // widget stuffed into one. Two consequences worth naming:
  //
  // Its context menu is deliberately left at the default. The wrapper this
  // replaced set `Qt::PreventContextMenu`, which meant the standard
  // "show/hide toolbar" menu QMainWindow offers for every toolbar and dock
  // could not be reached — the rail was the one piece of chrome an operator
  // could not put away. Being a real toolbar is what makes that free, and
  // suppressing the menu again would throw away the main reason for the
  // conversion. Page buttons keep their own `CustomContextMenu`, so a
  // right-click on a page still gets the page menu rather than this one.
  //
  // The window's own title is what `windowTitle` shows; the toolbar needs one
  // too, because that is the label QMainWindow puts in that menu.
  activity_bar_->setWindowTitle(
      QString::fromStdU16String(Translate("Activity bar")));
  addToolBar(Qt::LeftToolBarArea, activity_bar_);
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

void MainWindow::SetPaneMode(PaneModeId id) {
  if (pane_modes_)
    pane_modes_->SetMode(id);
}

bool MainWindow::SelectPaneModeForPane(std::string_view window_type) {
  return pane_modes_ && pane_modes_->SelectModeForPane(window_type);
}

void MainWindow::ShowSettings() {
  // The shell holds its menu as the MenuModel interface, and the settings items
  // are a detail of the real model. A test harness can install a different one,
  // so this asks rather than asserts: no menu model of ours means no
  // preferences to describe, which is a shell without settings, not a bug to
  // panic on.
  auto* menu_model = dynamic_cast<MainMenuModel*>(main_menu_model_.get());
  if (!menu_model)
    return;

  if (!settings_panel_) {
    settings_panel_ = new SettingsPanel{this, menu_model->settings_model()};
    // Re-measure whenever a setting is applied. `Status Bar` is a row on
    // that surface, so an operator can switch off the very strip the
    // panel stops above — and hiding a `QStatusBar` re-lays this window out
    // without resizing it, so nothing the panel watches for itself would
    // notice. A choice row can move the strip too: a locale changes what it
    // draws and a widget style changes its metrics.
    connect(settings_panel_, &SettingsPanel::SettingApplied, this,
            [this] { settings_panel_->SetBottomInset(StatusStripInset()); });
  }

  settings_panel_->Open(StatusStripInset());
}

int MainWindow::StatusStripInset() const {
  // The strip stays visible under the panel — it reports the session, the
  // connection and the server, none of which stops being true while
  // preferences are open. Zero when the operator has switched it off, which
  // hands the panel the whole window.
  const QStatusBar* strip = statusBar();
  return strip && strip->isVisible() ? strip->height() : 0;
}

void MainWindow::RefreshUtilityMarker() {
  if (!rail_utilities_)
    return;
  OpenedView* active = GetActiveView();
  rail_utilities_->Refresh(active ? std::string_view{active->window_info().name}
                                  : std::string_view{});
}

void MainWindow::OnViewClosed(OpenedView& view) {
  // WindowInfo is static, so the name outlives the view.
  const std::string_view pane_type = view.window_info().name;
  BaseMainWindow::OnViewClosed(view);
  if (pane_modes_)
    pane_modes_->OnPaneClosed(pane_type, view_manager_->is_closing_page());
}

void MainWindow::OnActiveViewChanged(OpenedView* view) {
  BaseMainWindow::OnActiveViewChanged(view);
  if (pane_modes_ && view)
    pane_modes_->OnPaneActivated(view->window_info().name);
  RefreshUtilityMarker();
  RefreshBreadcrumb();
}

void MainWindow::RefreshBreadcrumb() {
  if (!context_bar_)
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
  context_bar_->SetBreadcrumb(segments);
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
  palette_->Show(initial_text);
}

void MainWindow::SetWindowFlashing(bool flashing) {
  // MainWindowModule::OnEvents calls in on every event dispatch, so act on the
  // edge rather than re-asking on each one.
  if (window_flashing_ == flashing) {
    return;
  }
  window_flashing_ = flashing;
  if (!flashing) {
    // Deliberately nothing. Qt offers no public cancel — QWindow::alert sets
    // the platform alert state, and only activation or the duration timer
    // clears it — and none is needed: the alert is already suppressed while the
    // window is active, so the operator ends it by the same act that shows them
    // the events.
    return;
  }
  // No-op while this window is the active one, which is what makes it an
  // attention request rather than a decoration. Duration 0 means "until the
  // operator activates the window".
  // https://doc.qt.io/qt-6/qapplication.html#alert
  QApplication::alert(this, /*msec=*/0);
}

QString MainWindow::ControlUnavailableReason() {
  // Asked only while the Control button is disabled. The node answers for its
  // own shape through the same rule the write command's gates use
  // (GetWriteBlock); the command's only other gate is the session's Control
  // permission on process nodes (HasProcessPermission), so a node that would
  // accept control can be blocked by nothing else.
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

SeriesModel* MainWindow::ActiveSeriesModel() {
  OpenedView* active = GetActiveView();
  return active ? active->controller().GetSeriesModel() : nullptr;
}

std::optional<InspectorSeriesView> MainWindow::ActiveSeriesView() {
  SeriesModel* series = ActiveSeriesModel();
  if (!series || !series->HasSeries())
    return std::nullopt;

  return InspectorSeriesView{
      .color = series->GetSeriesColor().qcolor(),
      .own_pane = series->IsSeriesOnOwnPane(),
      .dots = series->AreSeriesDotsShown(),
      .stepped = series->IsSeriesStepped(),
  };
}

std::vector<InspectorOpenAction> MainWindow::OpenViewActions() {
  std::vector<InspectorOpenAction> actions;
  // Registration order, which is the order the toolbar and the context menu
  // draw them in — the three surfaces list the same views in the same sequence.
  for (const CommandDescriptor* command :
       ui_command_registry_.command_manager().commands()) {
    if (command->category != CATEGORY_OPEN)
      continue;
    // A command the current selection does not accept is left out rather than
    // drawn disabled: the section is a set of destinations, and a dead button
    // in a three-across grid reads as a view that is broken rather than one
    // this subject has no version of. Nothing selected disables all seven,
    // which is what empties the list and hides the section.
    CommandHandler* handler = ResolveViewCommand(command->command_id);
    if (!handler || !handler->IsCommandEnabled(command->command_id))
      continue;
    actions.push_back(InspectorOpenAction{
        .title = QString::fromStdU16String(command->GetShortTitle()),
        .icon = command->image_id != 0 ? QIcon(LoadPixmap(command->image_id))
                                       : QIcon{},
        .command_id = command->command_id,
    });
  }
  return actions;
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
          },
      // The panel reads the limit bands but cannot fetch them: they hang off
      // the selected node as property children, and nothing a selection does
      // makes them resident. The shell owns the executor, so it owns the fetch
      // and hands the panel a redraw.
      .load_limits =
          [this](const NodeRef& node, std::function<void()> redraw) {
            CoSpawn(executor_,
                    [node, redraw = std::move(redraw)]() -> Awaitable<void> {
                      co_await FetchLimitBands(node);
                      redraw();
                    });
          },
      // The Open section: the seven CATEGORY_OPEN selection commands, filtered
      // to the ones that accept the current selection. Read out of the command
      // registry rather than listed here, so a view registering an eighth Open
      // command reaches the Inspector without a second edit — and so the
      // Inspector, the Explorer context menu and the toolbar can never offer
      // different sets (docs/product/ui-mockups/authoring.md 4b).
      .open_actions = [this] { return OpenViewActions(); },
      .on_open =
          [this](unsigned command_id) {
            auto* handler = ResolveViewCommand(command_id);
            if (handler && handler->IsCommandEnabled(command_id))
              handler->ExecuteCommand(command_id);
          },
      // The series swatch is the panel's one writing field. Resolved against
      // the active view at click time, like the command handlers above, so the
      // panel holds no view pointer and a view closing under it is a no-op.
      .on_series_color_chosen =
          [this](QColor color) {
            if (SeriesModel* series = ActiveSeriesModel()) {
              series->SetSeriesColor(scada::aui::Color{color});
              // SetSeriesColor notifies through change_handler, which lands
              // back in OnSelectionChanged and re-reads the section — so the
              // ring follows the click without a second path.
            }
          }});

  auto* dock =
      new QDockWidget(QString::fromStdU16String(Translate("Inspector")), this);
  dock->setObjectName(QStringLiteral("InspectorDock"));
  dock->setWidget(inspector_);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  inspector_dock_ = dock;
}

void MainWindow::CreateSelectionPanels() {
  if (!selection_panel_registry_)
    return;

  const SelectionPanelContext context{
      .executor = executor_,
      .resolve_command =
          [this](unsigned command_id) {
            return ResolveViewCommand(command_id);
          },
      .node_service = node_service_,
      .attribute_service = attribute_service_,
      .call_node_method = call_node_method_,
      .has_call_permission = has_call_permission_};

  for (const SelectionPanelFactory& factory :
       selection_panel_registry_->factories()) {
    std::unique_ptr<SelectionPanel> panel = factory(context);
    if (!panel)
      continue;

    auto* dock =
        new QDockWidget(QString::fromStdU16String(panel->title()), this);
    dock->setObjectName(QString::fromStdString(panel->object_name()));
    dock->setWidget(&panel->widget());
    addDockWidget(Qt::RightDockWidgetArea, dock);
    // Shares the right dock with the Inspector; a panel's tab comes to the
    // front only when the operator picks it.
    if (inspector_dock_)
      tabifyDockWidget(inspector_dock_, dock);

    selection_panels_.push_back(std::move(panel));
  }
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
  for (const std::unique_ptr<SelectionPanel>& panel : selection_panels_) {
    if (auto* dock = findChild<QDockWidget*>(
            QString::fromStdString(panel->object_name()));
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
  if (pane_modes_) {
    // Conform the page to the active mode BEFORE the view manager builds it.
    // ViewManager::OpenPage creates every visible window and only then restores
    // the dock-state blob, so a pane opened afterwards would sit outside the
    // restored layout at Qt's default width.
    Page conformed = page;
    pane_modes_->ConformPage(conformed);
    BaseMainWindow::OpenPage(conformed);
  } else {
    // No rail yet (a page opened before the chrome is built), so the page's own
    // pane set is authoritative.
    BaseMainWindow::OpenPage(page);
  }

  TabifySpecialistDocks();

  // The restored dock blob carries whichever pane was fronted when the page was
  // last saved, which need not be the mode's subject — a page saved while
  // Portfolio was on top reopens showing Portfolio, not Objects. Re-assert it
  // after TabifySpecialistDocks, which raises the Inspector and so must not be
  // the last thing to touch dock stacking.
  if (pane_modes_)
    pane_modes_->FrontPrimaryPane();

  // Every page switch funnels through here — the Pages menu, the Pages pane,
  // the page commands, and the startup restore — so this is the one place the
  // marker has to be re-derived, and the one place the page list is known to
  // be stale (New and Delete both end in an OpenPage).
  if (rail_pages_)
    rail_pages_->Refresh();
  if (pane_modes_)
    pane_modes_->RefreshMarker();

  // The page's panes exist now, so every tree in it has queued its own first
  // level. Only now may the palette's browse start.
  if (palette_)
    palette_->StartTagSearchBrowse();
}

void MainWindow::OnSelectionChanged() {
  // The breadcrumb's last segment is the selection, so it moves with it.
  RefreshBreadcrumb();

  OpenedView* active = GetActiveView();
  SelectionModel* selection =
      active ? active->controller().GetSelectionModel() : nullptr;
  if (inspector_) {
    if (selection)
      inspector_->ShowSelection(*selection);
    else
      inspector_->Clear();
    // The plotted-series section, for a view that plots something. Every
    // other view supplies no SeriesModel and the section stays absent — an
    // absent section rather than an empty one, because "this view has no
    // series" is not a fact about the selection worth a row.
    inspector_->ShowSeries(ActiveSeriesView());
  }
  // Each specialist panel decides which selections it shows; the shell only
  // hands the selection over.
  for (const std::unique_ptr<SelectionPanel>& panel : selection_panels_)
    panel->ShowSelection(selection);

  // Resolve the context menu's commands against the new selection now, not
  // first when the operator right-clicks. Resolving is what starts the node
  // fetches some commands' availability reads (the «Создать» group walks the
  // selection's parents and types through lazily loaded NodeRefs), and the
  // menu is built synchronously, so a first right-click on a freshly selected
  // node would otherwise miss those rows. The command toolbar's QAction
  // registry used to do this as a side effect on every selection change; it
  // went with the toolbar (backlog 697), and the screenshot generator's
  // menu-create-object and menu-parameters-elements captures are what noticed.
  if (context_menu_model_)
    context_menu_model_->MenuWillShow();

  // Null before the constructor has built them and after the destructor has
  // begun; a key only reaches a shortcut whose command applies right now.
  if (command_shortcuts_)
    command_shortcuts_->Refresh();
}

void MainWindow::OnShowTabPopupMenu(OpenedView& view,
                                    const scada::aui::Point& point) {
  QMenu menu;
  BuildMenu(menu, *tab_popup_menu_);
  menu.exec(point);
}

QString MainWindow::SelectionSubjectTitle() {
  OpenedView* active = GetActiveView();
  SelectionModel* selection =
      active ? active->controller().GetSelectionModel() : nullptr;
  if (!selection || selection->empty())
    return QString{};
  return QString::fromStdU16String(selection->GetTitle());
}

std::vector<MenuContribution> MainWindow::EmptyViewCommands() {
  return FindEmptyViewCommands(
      ui_command_registry_, [this](unsigned command_id) {
        CommandHandler* handler = ResolveViewCommand(command_id);
        return handler && handler->IsCommandEnabled(command_id);
      });
}

void MainWindow::OnShowNewViewMenu(const scada::aui::Point& point) {
  QMenu menu;

  // The menu names its subject, and that is the whole difference between this
  // `+` and the activity rail's: the rail's adds a *page*, this one adds a
  // *tab*, and they are never adjacent for a user to compare
  // (docs/product/ui-mockups/authoring.md 4b). Said rather than inferred.
  const std::vector<InspectorOpenAction> subject_actions = OpenViewActions();
  const QString subject = SelectionSubjectTitle();
  if (!subject_actions.empty() && !subject.isEmpty()) {
    menu.addSection(tr("New view for %1").arg(subject));
    for (const InspectorOpenAction& action : subject_actions) {
      QAction* item = menu.addAction(action.icon, action.title);
      const unsigned command_id = action.command_id;
      connect(item, &QAction::triggered, this, [this, command_id] {
        auto* handler = ResolveViewCommand(command_id);
        if (handler && handler->IsCommandEnabled(command_id))
          handler->ExecuteCommand(command_id);
      });
    }
  }

  // The Empty group. These are the commands that had no home in the shell
  // except the menu bar, which is the reason the screens give this group.
  const std::vector<MenuContribution> empty_commands = EmptyViewCommands();
  if (!empty_commands.empty()) {
    menu.addSection(tr("Empty"));
    for (const MenuContribution& contribution : empty_commands) {
      QAction* item =
          menu.addAction(QString::fromStdU16String(contribution.title));
      const unsigned command_id = contribution.command_id;
      connect(item, &QAction::triggered, this, [this, command_id] {
        auto* handler = ResolveViewCommand(command_id);
        if (handler && handler->IsCommandEnabled(command_id))
          handler->ExecuteCommand(command_id);
      });
    }
  }

  // Nothing to offer at all: no selection and no empty-view command enabled.
  // Showing an empty popup would say the button is broken rather than that
  // there is nothing to open.
  if (menu.isEmpty())
    return;

  menu.exec(point);
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

void MainWindow::SetPopupMenuInterceptor(PopupMenuInterceptor interceptor) {
  popup_menu_interceptor_ = std::move(interceptor);
}

void MainWindow::ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                               const scada::aui::Point& point,
                               bool right_click) {
  QMenu menu;
  BuildDefaultPopupMenu(menu, merge_menu, *context_menu_model_);
  // The menu is fully built by this point, so an interceptor sees exactly what
  // the operator would. Returning instead of exec()ing is the whole diversion.
  if (popup_menu_interceptor_) {
    popup_menu_interceptor_(menu);
    return;
  }
  menu.exec(point);
}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(
    WindowDefinition& window_def) {
  return opened_view_factory_(*this, window_def);
}
