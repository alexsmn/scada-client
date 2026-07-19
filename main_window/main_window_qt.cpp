#include "main_window/main_window_qt.h"

#include "aui/models/menu_model.h"
#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model.h"
#include "aui/qt/status_bar.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
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
#include "filesystem/file_cache.h"
#include "inspector/qt/inspector_panel.h"
#include "main_window/activity_bar_qt.h"
#include "main_window/alarm_flood.h"
#include "main_window/command_palette_qt.h"
#include "main_window/main_window_command_router.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/overview_page.h"
#include "main_window/selection_command_router.h"
#include "main_window/simple_menu_command_handler.h"
#include "main_window/status_bar/progress_controller_qt.h"
#include "main_window/tag_search_index.h"
#include "main_window/view_manager.h"
#include "main_window/window_definition_builder.h"
#include "model/devices_node_ids.h"
#include "model/security_node_ids.h"
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
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

#include <unordered_set>

#ifdef _WIN32
#include <atlapp.h>
#include <atlbase.h>
#include <atluser.h>
#endif

namespace {

inline QKeySequence ToQKeySequence(const Shortcut& shortcut) {
  return QKeySequence{static_cast<int>(shortcut.key_code()) +
                      static_cast<int>(shortcut.modifiers())};
}

#ifdef _WIN32
void BuildMenuModel(CMenuHandle menu_handle,
                    aui::MenuModel& context_menu_model,
                    aui::SimpleMenuModel& menu_model,
                    std::vector<std::unique_ptr<aui::MenuModel>>& submenus) {
  for (int i = 0; i < menu_handle.GetMenuItemCount(); ++i) {
    wchar_t title[64] = {};

    CMenuItemInfo menu_info;
    menu_info.fMask |=
        MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_SUBMENU | MIIM_STATE;
    menu_info.cch = std::size(title);
    menu_info.dwTypeData = title;
    menu_handle.GetMenuItemInfo(i, TRUE, &menu_info);

    if (menu_info.hSubMenu) {
      auto submenu_model =
          std::make_unique<aui::SimpleMenuModel>(menu_model.delegate());
      BuildMenuModel(menu_info.hSubMenu, context_menu_model, *submenu_model,
                     submenus);
      menu_model.AddSubMenu(menu_info.wID, UtfConvert<char16_t>(title),
                            submenu_model.get());
      submenus.emplace_back(std::move(submenu_model));

    } else if (menu_info.fType & MFT_SEPARATOR) {
      menu_model.AddSeparator(aui::NORMAL_SEPARATOR);

    } else if (menu_info.fState & MFS_CHECKED) {
      menu_model.AddCheckItem(menu_info.wID, UtfConvert<char16_t>(title));

    } else if (menu_info.wID == ID_ITEM_COMMANDS) {
      menu_model.AddInplaceMenu(&context_menu_model);

    } else {
      menu_model.AddItem(menu_info.wID, UtfConvert<char16_t>(title));
    }
  }
}
#endif

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
void CollectMenuCommandIds(aui::MenuModel& model,
                           std::unordered_set<int>& command_ids) {
  model.MenuWillShow();
  for (int i = 0; i < model.GetItemCount(); ++i) {
    switch (model.GetTypeAt(i)) {
      case aui::MenuModel::TYPE_SUBMENU:
      case aui::MenuModel::TYPE_INPLACE_MENU:
        if (auto* submenu_model = model.GetSubmenuModelAt(i))
          CollectMenuCommandIds(*submenu_model, command_ids);
        break;
      case aui::MenuModel::TYPE_SEPARATOR:
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
                           aui::MenuModel* merge_menu,
                           aui::MenuModel& context_menu_model) {
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

constexpr CommandContextId kToolbarContexts[] = {
    CommandContextId::Global,
    CommandContextId::Selection,
    CommandContextId::OpenedView,
    CommandContextId::Controller,
};

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
  // the active UX theme, which both the app (app/qt/main.cpp) and the headless
  // screenshot generator set together with the palette when the experimental UX
  // is enabled.
  if (scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy) {
    CreateActivityBar();
    CreateContextBar();
    CreateInspectorPanel();
    CreateDiagnosticsPanel();
    CreateUserAccessPanel();
    CreateTransmissionRulePanel();
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
}

void MainWindow::CreateMenuBar() {
  /*auto* settings_menu = new QMenu(tr("Settings"), this);
  auto* style_menu = new QMenu(tr("Style"), this);
  for (auto& style : QStyleFactory::keys())
    style_menu->addAction(style,
                          [this, style] { QApplication::setStyle(style); });
  settings_menu->addMenu(style_menu);*/

  main_menu_model_ = main_menu_factory_(*this, dialog_service_, *view_manager_,
                                        *commands_, *context_menu_model_);

  auto* menu_bar = new QMenuBar(this);
  setMenuBar(menu_bar);

  for (int i = 0; i < main_menu_model_->GetItemCount(); ++i) {
    auto* submenu = menu_bar->addMenu(
        QString::fromStdU16String(main_menu_model_->GetLabelAt(i)));
    auto* submenu_model = main_menu_model_->GetSubmenuModelAt(i);
    base::Check(submenu_model);
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

  // A dedicated Settings menu, appended after the model-driven menus, so the
  // experimental-reshell opt-in is reachable even in the legacy look (the
  // Ux/Experimental QSetting otherwise has no UI). Written through QSettings so
  // it round-trips its own key encoding — unlike editing the plist by hand.
  auto* settings_menu =
      menuBar()->addMenu(QString::fromStdU16String(Translate("Settings")));
  auto* ux_action = settings_menu->addAction(
      QString::fromStdU16String(Translate("Experimental UX")));
  ux_action->setCheckable(true);
  ux_action->setChecked(QSettings{}.value("Ux/Experimental", false).toBool());
  connect(ux_action, &QAction::toggled, this,
          [this](bool enabled) { OnToggleExperimentalUx(enabled); });
}

void MainWindow::OnToggleExperimentalUx(bool enabled) {
  QSettings settings;
  settings.setValue("Ux/Experimental", enabled);
  settings.sync();
  QMessageBox::information(
      this, QString::fromStdU16String(Translate("Experimental UX")),
      QString::fromStdU16String(
          Translate("Restart the client to apply the interface change.")));
}

void MainWindow::RebuildMenuBar() {
  const auto top_level_actions = menuBar()->actions();
  // The appended Settings menu is not part of the model, so allow one extra.
  base::Check(top_level_actions.size() >= main_menu_model_->GetItemCount());

  for (int i = 0; i < main_menu_model_->GetItemCount(); ++i) {
    auto* submenu = top_level_actions[i]->menu();
    auto* submenu_model = main_menu_model_->GetSubmenuModelAt(i);
    base::Check(submenu);
    base::Check(submenu_model);
    submenu->clear();
    BuildMenu(*submenu, *submenu_model);
  }
}

void MainWindow::CreateStatusBar() {
  auto* status_bar = new aui::StatusBar{*status_bar_model_, this};
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

  // Brand lockup (left).
  auto* brand = new QLabel(context_bar_);
  brand->setText(QStringLiteral("  Telecontrol SCADA  "));
  brand->setStyleSheet(QStringLiteral("font-weight:700;"));
  context_bar_->addWidget(brand);

  auto* left_spacer = new QWidget(context_bar_);
  left_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  context_bar_->addWidget(left_spacer);

  // Command/search entry point (centre). Read-only: it is an affordance that
  // opens the command palette (click or Ctrl+K); typing happens in the palette.
  command_search_ = new QLineEdit(context_bar_);
  command_search_->setPlaceholderText(
      QString::fromStdU16String(Translate("Search tags, objects, commands…")));
  command_search_->setReadOnly(true);
  command_search_->setFixedWidth(360);
  command_search_->installEventFilter(this);
  context_bar_->addWidget(command_search_);

  auto* palette_shortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this);
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

  // Live severity KPI tiles: unacknowledged-alarm counts per level, coloured
  // from the severity single source (bold when active, plain when calm).
  kpi_critical_ = new QLabel(context_bar_);
  kpi_warning_ = new QLabel(context_bar_);
  for (QLabel* tile : {kpi_critical_, kpi_warning_}) {
    tile->setMargin(2);
    context_bar_->addWidget(tile);
  }

  // Context cluster (right): a curated who/where subset of the status-bar panes
  // (user / connection / server / endpoint) — not the whole status strip. The
  // alarm counts live in the KPI tiles, so the cluster stays identity/location.
  const int pane_count = status_bar_model_->GetPaneCount();
  for (int i = 0; i < pane_count; ++i) {
    if (!status_bar_model_->IsContextBarPane(i))
      continue;
    auto* label = new QLabel(context_bar_);
    label->setMargin(2);
    context_panes_.push_back(label);
    context_pane_indices_.push_back(i);
    context_bar_->addWidget(label);
  }

  auto refresh_kpi = [this](QLabel* tile, scada::aui::SeverityLevel level,
                            const char* name) {
    const int count = status_bar_model_->GetSeverityCount(level);
    tile->setText(QStringLiteral("%1 %2")
                      .arg(QString::fromStdU16String(Translate(name)))
                      .arg(count));
    const std::optional<aui::Color> color = scada::aui::SeverityColor(level);
    // Bold + coloured while alarms are active, plain when the count is zero.
    tile->setStyleSheet(count > 0 && color
                            ? QStringLiteral("color:%1;font-weight:700;")
                                  .arg(color->qcolor().name())
                            : QString{});
  };

  auto refresh = [this, refresh_kpi] {
    for (int k = 0; k < static_cast<int>(context_panes_.size()); ++k) {
      const int pane = context_pane_indices_[k];
      context_panes_[k]->setText(
          QString::fromStdU16String(status_bar_model_->GetPaneText(pane)));
      const std::optional<aui::Color> color =
          status_bar_model_->GetPaneColor(pane);
      context_panes_[k]->setStyleSheet(
          color ? QStringLiteral("color:%1;font-weight:600;")
                      .arg(color->qcolor().name())
                : QString{});
    }
    refresh_kpi(kpi_critical_, scada::aui::SeverityLevel::kCritical,
                "Critical");
    refresh_kpi(kpi_warning_, scada::aui::SeverityLevel::kWarning, "Warning");

    // Flood escalation: a single prominent state pill when the unacknowledged
    // count crosses the flood threshold, so a flood reads as a state, not a
    // scroll.
    const int alarm_count = status_bar_model_->GetAlarmCount();
    const bool flood = IsAlarmFlood(alarm_count);
    flood_indicator_->setVisible(flood);
    if (flood) {
      flood_indicator_->setText(
          QStringLiteral(" %1 (%2) ")
              .arg(QString::fromStdU16String(Translate("Alarm flood")))
              .arg(alarm_count));
      const std::optional<aui::Color> color =
          scada::aui::SeverityColor(scada::aui::SeverityLevel::kCritical);
      // White reads on the saturated critical fill across every theme.
      flood_indicator_->setStyleSheet(
          QStringLiteral(
              "background:%1;color:#ffffff;border-radius:9px;font-weight:700;")
              .arg(color ? color->qcolor().name() : QStringLiteral("#e85a52")));
    }
  };
  refresh();
  context_bar_connection_ = status_bar_model_->SubscribePanesChanged(
      [refresh](int, int) { refresh(); });

  addToolBar(Qt::TopToolBarArea, context_bar_);
  // Force the command toolbar onto its own row below the context bar.
  addToolBarBreak(Qt::TopToolBarArea);
}

void MainWindow::CreateActivityBar() {
  // Section spec: label (English, Translate()'d), the WindowInfo name it opens
  // (empty => no view yet, shown disabled), and rail placement. Alarms carries
  // the unread badge; Administration/Settings pin to the bottom.
  struct SectionSpec {
    const char* label;
    std::string_view window_info_name;
    ActivityBar::Icon icon = ActivityBar::Icon::kNone;
    bool is_alarms = false;
    bool pinned_bottom = false;
  };
  const SectionSpec specs[] = {
      {"Overview", kOverviewSectionId, ActivityBar::Icon::kOverview},
      {"Alarms", "EventJournal", ActivityBar::Icon::kAlarms,
       /*is_alarms=*/true},
      {"Trends", "Graph", ActivityBar::Icon::kTrends},
      {"Substations", "Modus", ActivityBar::Icon::kSubstations},
      {"Tables", "Table", ActivityBar::Icon::kTables},
      {"Administration", "", ActivityBar::Icon::kAdministration, false,
       /*pinned_bottom=*/true},
      {"Settings", "", ActivityBar::Icon::kSettings, false,
       /*pinned_bottom=*/true},
  };

  std::vector<ActivityBar::Section> sections;
  for (const SectionSpec& spec : specs) {
    ActivityBar::Section section;
    section.label = Translate(spec.label);
    section.window_info_name = std::string{spec.window_info_name};
    section.icon_kind = spec.icon;
    section.is_alarms = spec.is_alarms;
    section.pinned_bottom = spec.pinned_bottom;
    // A section is live only if it opens the Overview page or its view type is
    // registered; otherwise it is shown disabled (with a "coming soon"
    // tooltip).
    section.enabled = spec.window_info_name == kOverviewSectionId ||
                      (!spec.window_info_name.empty() &&
                       FindWindowInfoByName(spec.window_info_name) != nullptr);
    sections.push_back(std::move(section));
  }

  activity_bar_ = new ActivityBar(
      this, std::move(sections),
      [this](const std::string& name) { ActivateSection(name); });

  auto* rail = new QToolBar(this);
  rail->setObjectName(QStringLiteral("ActivityRail"));
  rail->setMovable(false);
  rail->setFloatable(false);
  rail->setContextMenuPolicy(Qt::PreventContextMenu);
  rail->addWidget(activity_bar_);
  addToolBar(Qt::LeftToolBarArea, rail);

  // The alarm badge tracks the same unacknowledged count as the status strip,
  // which refreshes the status-bar model on every event change.
  auto refresh_badge = [this] {
    activity_bar_->SetAlarmCount(status_bar_model_->GetAlarmCount());
  };
  refresh_badge();
  activity_bar_connection_ = status_bar_model_->SubscribePanesChanged(
      [refresh_badge](int, int) { refresh_badge(); });
}

void MainWindow::OpenOverviewPage() {
  OpenPage(MakeOverviewPage());
}

void MainWindow::ActivateSection(const std::string& window_info_name) {
  if (window_info_name == kOverviewSectionId) {
    OpenOverviewPage();
    activity_bar_->SetActiveSection(window_info_name);
    return;
  }

  const WindowInfo* info = FindWindowInfoByName(window_info_name);
  if (!info)
    return;
  CoSpawn(executor_,
          [this, def = WindowDefinition{*info}]() mutable -> Awaitable<void> {
            co_await OpenView(def, /*make_active=*/true);
          });
  activity_bar_->SetActiveSection(window_info_name);
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
        return ResolveCommandHandler(ui_command_registry_.command_manager(),
                                     command_id, kToolbarContexts, *commands_);
      },
      std::move(extras));
  palette->setAttribute(Qt::WA_DeleteOnClose);
  if (!initial_text.isEmpty())
    palette->PresetFilter(initial_text);
  palette->show();
  palette->raise();
  palette->activateWindow();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched != command_search_)
    return QMainWindow::eventFilter(watched, event);

  // Clicking the read-only context-bar search field opens the command palette.
  if (event->type() == QEvent::MouseButtonRelease) {
    ShowCommandPalette();
    return true;
  }
  // Typing a printable character opens the palette seeded with it, so the
  // field reads as a real search box even though the palette owns the input.
  if (event->type() == QEvent::KeyPress) {
    auto* key_event = static_cast<QKeyEvent*>(event);
    const QString text = key_event->text();
    const bool has_command_modifier =
        key_event->modifiers() &
        (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    if (!has_command_modifier && !text.isEmpty() && text.at(0).isPrint()) {
      ShowCommandPalette(text);
      return true;
    }
  }
  return QMainWindow::eventFilter(watched, event);
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
    QObject::connect(
        action, &QAction::triggered, [this, command_id](bool checked) {
          auto* handler =
              ResolveCommandHandler(ui_command_registry_.command_manager(),
                                    command_id, kToolbarContexts, *commands_);
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

void MainWindow::CreateInspectorPanel() {
  // The control action reuses the selection-scoped write/control command
  // (ID_WRITE) — the existing two-stage confirm — resolved against the active
  // selection exactly like the toolbar/menu path.
  auto resolve_write = [this]() -> CommandHandler* {
    return ResolveCommandHandler(ui_command_registry_.command_manager(),
                                 ID_WRITE, kToolbarContexts, *commands_);
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
          }});

  auto* dock =
      new QDockWidget(QString::fromStdU16String(Translate("Inspector")), this);
  dock->setObjectName(QStringLiteral("InspectorDock"));
  dock->setWidget(inspector_);
  addDockWidget(Qt::RightDockWidgetArea, dock);
  inspector_dock_ = dock;
}

void MainWindow::CreateDiagnosticsPanel() {
  // Each action reuses a selection-scoped device command, resolved against the
  // active selection exactly like the toolbar/menu path — Metrics trend
  // (ID_OPEN_DEVICE_METRICS), Reconnect (ID_ITEM_ENABLE re-enables the device),
  // and Open log (ID_OPEN_EVENTS opens the event journal).
  auto make_action = [this](unsigned command_id,
                            std::u16string label) -> DiagnosticAction {
    auto resolve = [this, command_id]() -> CommandHandler* {
      return ResolveCommandHandler(ui_command_registry_.command_manager(),
                                   command_id, kToolbarContexts, *commands_);
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
            }};
  };

  DeviceDiagnosticsPanelContext context;
  context.actions.push_back(
      make_action(ID_OPEN_DEVICE_METRICS, Translate("Metrics trend")));
  context.actions.push_back(
      make_action(ID_ITEM_ENABLE, Translate("Reconnect")));
  context.actions.push_back(make_action(ID_OPEN_EVENTS, Translate("Open log")));

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

void MainWindow::OnSelectionChanged() {
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
      // clears.
      if (selection && !selection->empty() && !selection->multiple() &&
          IsInstanceOf(selection->node(), scada::security::id::UserType)) {
        user_access_->ShowUser(selection->node());
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

void MainWindow::OnShowTabPopupMenu(OpenedView& view, const aui::Point& point) {
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

  const CommandHandler* handler =
      ResolveCommandHandler(ui_command_registry_.command_manager(), command_id,
                            kToolbarContexts, *commands_);
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

void MainWindow::ShowPopupMenu(aui::MenuModel* merge_menu,
                               unsigned resource_id,
                               const aui::Point& point,
                               bool right_click) {
  if (resource_id == 0) {
    QMenu menu;
    BuildDefaultPopupMenu(menu, merge_menu, *context_menu_model_);
    menu.exec(point);
    return;
  }

  SimpleMenuCommandHandler command_handler{commands()};
  aui::SimpleMenuModel menu_model{&command_handler};
  std::vector<std::unique_ptr<aui::MenuModel>> submenus;

#ifdef _WIN32
  {
    CMenu resource_menu;
    resource_menu.LoadMenu(resource_id);
    BuildMenuModel(resource_menu.GetSubMenu(0), *context_menu_model_,
                   menu_model, submenus);
  }
#else
  {
    QMenu menu;
    BuildDefaultPopupMenu(menu, merge_menu, *context_menu_model_);
    menu.exec(point);
    return;
  }
#endif

  QMenu menu;
  // TODO: Combine with the same above.
  if (merge_menu && merge_menu->GetItemCount() != 0) {
    BuildMenu(menu, *merge_menu);
    menu.addSeparator();
  }
  BuildMenu(menu, menu_model);
  menu.exec(point);
}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(
    WindowDefinition& window_def) {
  return opened_view_factory_(*this, window_def);
}
