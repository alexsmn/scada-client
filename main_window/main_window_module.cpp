#include "main_window/main_window_module.h"

#include "aui/dialog_service.h"
#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "base/boost_log.h"
#include "common/master_data_services.h"
#include "controller/action_manager.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_context.h"
#include "controller/controller_registry.h"
#include "core/default_node_command_registry.h"
#include "core/global_command_context.h"
#include "events/event_fetcher.h"
#include "main_window/context_menu_model.h"
#include "main_window/event_dispatcher.h"
#include "main_window/main_menu/main_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_command_router.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_module.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "main_window/opened_view/opened_view_command_router.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "main_window/pages/page_commands.h"
#include "main_window/selection_command_router.h"
#include "main_window/standard_command_ids.h"
#include "main_window/status_bar/status_bar_model_builder.h"
#include "main_window/window_definition_builder.h"
#include "modules/node_properties/node_property_component.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"
#include "services/speech_service.h"

#if defined(UI_QT)
#include "main_window/main_window_qt.h"
#include <QApplication>
#include <QSettings>
#elif defined(UI_WT)
#include "main_window/main_window_wt.h"
#endif

#if defined(UI_WT)
#include <wt/WApplication.h>
#endif

namespace {

#if defined(UI_QT)
std::unique_ptr<MainWindow> CreateMainWindow(MainWindowContext&& context) {
  return std::make_unique<MainWindow>(std::move(context));
}
#elif defined(UI_WT)
std::unique_ptr<MainWindow> CreateMainWindow(MainWindowContext&& context) {
  assert(Wt::WApplication::instance());
  assert(Wt::WApplication::instance()->root());
  return std::make_unique<MainWindow>(*Wt::WApplication::instance()->root(),
                                      std::move(context));
}
#endif

Awaitable<void> ShowRenameWindowDialogAsync(DialogService& dialog_service,
                                            OpenedViewInterface& view,
                                            std::u16string current_view_title) {
  auto title =
      co_await RunPromptDialog(dialog_service, Translate("Name:"),
                               Translate("Rename"), current_view_title);
  // TODO: Capture weak pointer.
  view.SetWindowTitle(title);
  co_return;
}

void ShowRenameWindowDialog(AnyExecutor executor,
                            const GlobalCommandContext& context) {
  auto* view = context.main_window.GetActiveView();
  if (!view || view->GetWindowInfo().is_pane()) {
    return;
  }

  CoSpawn(executor, [&dialog_service = context.dialog_service, view,
                     current_view_title = view->GetWindowTitle()] {
    return ShowRenameWindowDialogAsync(dialog_service, *view,
                                       current_view_title);
  });
}

BasicCommand<GlobalCommandContext> MakeMainWindowOptionCommand(
    unsigned command_id,
    std::u16string_view title,
    Profile& profile,
    bool MainWindowDef::* option) {
  return {
      .command_id = command_id,
      .title = std::u16string{title},
      .menu_group = MenuGroup::MAIN_WINDOW_SETTINGS,
      .execute_handler =
          [&profile, option](const GlobalCommandContext& context) {
            MainWindowDef& prefs =
                profile.GetMainWindow(context.main_window.GetMainWindowId());
            prefs.*option = !(prefs.*option);
            profile.NotifyChange();
          },
      .checked_handler =
          [&profile, option](const GlobalCommandContext& context) {
            const MainWindowDef* prefs =
                profile.FindMainWindow(context.main_window.GetMainWindowId());
            return prefs && prefs->*option;
          }};
}

BasicCommand<GlobalCommandContext> MakeProfileOptionCommand(
    unsigned command_id,
    Profile& profile,
    bool Profile::* option,
    std::function<bool()> enabled_handler = {}) {
  return {.command_id = command_id,
          .execute_handler =
              [&profile, option](const GlobalCommandContext&) {
                profile.*option = !(profile.*option);
              },
          .enabled_handler =
              [enabled_handler =
                   std::move(enabled_handler)](const GlobalCommandContext&) {
                return !enabled_handler || enabled_handler();
              },
          .checked_handler =
              [&profile, option](const GlobalCommandContext&) {
                return profile.*option;
              }};
}

#if defined(UI_QT)
QString GetSelectedLocaleName() {
  QSettings settings;

  if (auto locale_name = settings.value("LocaleName").toString();
      !locale_name.isEmpty()) {
    return locale_name;
  }

  return QLocale::system().bcp47Name();
}

bool IsRussianLocale(QStringView locale_name) {
  return locale_name.startsWith(u"ru", Qt::CaseInsensitive);
}

void SetLocaleName(std::string_view locale_name) {
  QSettings settings;
  settings.setValue("LocaleName",
                    QString::fromStdString(std::string(locale_name)));
}

void ApplyLanguageSelection(const AnyExecutor& executor,
                            const GlobalCommandContext& context,
                            std::string_view locale_name) {
  SetLocaleName(locale_name);
  CoSpawn(executor, [executor, &context]() -> Awaitable<void> {
    auto result = co_await context.dialog_service.RunMessageBox(
        Translate("Restart the application to apply the new language now?"),
        Translate("Language"), MessageBoxMode::QuestionYesNo);
    if (result == MessageBoxResult::Yes) {
      QApplication::quit();
    }
    co_return;
  });
}

BasicCommand<GlobalCommandContext> MakeLanguageCommand(
    AnyExecutor executor,
    unsigned command_id,
    std::u16string_view title,
    std::string_view locale_name,
    bool is_russian) {
  return {.command_id = command_id,
          .title = std::u16string{title},
          .menu_group = MenuGroup::MAIN_WINDOW_SETTINGS,
          .execute_handler =
              [executor = std::move(executor),
               locale_name](const GlobalCommandContext& context) {
                ApplyLanguageSelection(executor, context, locale_name);
              },
          .checked_handler =
              [is_russian](const GlobalCommandContext&) {
                return IsRussianLocale(GetSelectedLocaleName()) == is_russian;
              }};
}
#endif

void RegisterMainWindowCommandActions(
    AnyExecutor executor,
    Profile& profile,
    SpeechService& speech_service,
    scada::SessionService& session_service,
    MainWindowManager& main_window_manager,
    std::function<void(bool login)> login_handler,
    BasicCommandRegistry<GlobalCommandContext>& global_commands) {
  global_commands.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_WINDOW_NEW}
          .set_available_handler([](const GlobalCommandContext& context) {
            return context.main_window.GetActiveView() != nullptr;
          })
          .set_execute_handler(
              [&main_window_manager](const GlobalCommandContext&) {
                main_window_manager.CreateMainWindow();
              }));

  global_commands.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_VIEW_CHANGE_TITLE}
          .set_available_handler([](const GlobalCommandContext& context) {
            return context.main_window.GetActiveView() != nullptr;
          })
          .set_enabled_handler([](const GlobalCommandContext& context) {
            auto* active_view = context.main_window.GetActiveView();
            return active_view && !active_view->GetWindowInfo().is_pane();
          })
          .set_execute_handler(
              [executor = executor](const GlobalCommandContext& context) {
                ShowRenameWindowDialog(executor, context);
              }));

  global_commands.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_LOGIN}
          .set_enabled_handler([&session_service](const GlobalCommandContext&) {
            return !session_service.IsConnected();
          })
          .set_execute_handler([login_handler](const GlobalCommandContext&) {
            login_handler(/*login=*/true);
          }));
  global_commands.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_LOGOFF}
          .set_enabled_handler([&session_service](const GlobalCommandContext&) {
            return session_service.IsConnected();
          })
          .set_execute_handler([login_handler](const GlobalCommandContext&) {
            login_handler(/*login=*/false);
          }));

#if defined(UI_QT)
  global_commands.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_WINDOW_SPLIT_HORZ}
          .set_available_handler([](const GlobalCommandContext& context) {
            return context.main_window.GetActiveView() != nullptr;
          })
          .set_execute_handler([](const GlobalCommandContext& context) {
            if (auto* active_view = context.main_window.GetActiveView()) {
              context.main_window.SplitView(*active_view,
                                            /*vertically=*/true);
            }
          }));
  global_commands.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_WINDOW_SPLIT_VERT}
          .set_available_handler([](const GlobalCommandContext& context) {
            return context.main_window.GetActiveView() != nullptr;
          })
          .set_execute_handler([](const GlobalCommandContext& context) {
            if (auto* active_view = context.main_window.GetActiveView()) {
              context.main_window.SplitView(*active_view,
                                            /*vertically=*/false);
            }
          }));
#endif

  global_commands.AddCommand(MakeMainWindowOptionCommand(
      ID_VIEW_TOOLBAR, Translate("Toolbar"), profile, &MainWindowDef::toolbar));

  global_commands.AddCommand(
      MakeMainWindowOptionCommand(ID_VIEW_STATUS_BAR, Translate("Status Bar"),
                                  profile, &MainWindowDef::status_bar));

#if defined(UI_QT)
  global_commands.AddCommand(MakeLanguageCommand(executor, ID_LANGUAGE_ENGLISH,
                                                 Translate("English"), "en",
                                                 /*is_russian=*/false));
  global_commands.AddCommand(MakeLanguageCommand(executor, ID_LANGUAGE_RUSSIAN,
                                                 Translate("Russian"), "ru_RU",
                                                 /*is_russian=*/true));
#else
  (void)executor;
#endif

  global_commands.AddCommand(MakeProfileOptionCommand(ID_SHOW_WRITEOK, profile,
                                                      &Profile::show_write_ok));
  global_commands.AddCommand(MakeProfileOptionCommand(
      ID_SHOW_EVENTS, profile, &Profile::event_auto_show));
  global_commands.AddCommand(MakeProfileOptionCommand(
      ID_HIDE_EVENTS, profile, &Profile::event_auto_hide));
  global_commands.AddCommand(MakeProfileOptionCommand(
      ID_WRITE_CONFIRMATION, profile, &Profile::control_confirmation));
  global_commands.AddCommand(MakeProfileOptionCommand(
      ID_OPT_SPEECH, profile, &Profile::speech_enabled,
      [&speech_service] { return speech_service.is_ok(); }));
  global_commands.AddCommand(MakeProfileOptionCommand(
      ID_EVENT_FLASH_WINDOW, profile, &Profile::event_flash_window));
  global_commands.AddCommand(MakeProfileOptionCommand(
      ID_EVENT_PLAY_SOUND, profile, &Profile::event_play_sound));
}

}  // namespace

MainWindowModule::MainWindowModule(MainWindowModuleContext&& context)
    : MainWindowModuleContext{std::move(context)} {
  assert(scada_services_.session_service);

  default_node_commands_.AddHandler(
      std::bind_front(&::ExecuteDefaultNodeCommand, executor_));

  main_window_manager_ =
      std::make_unique<MainWindowManager>(MainWindowManagerContext{
          .profile_ = profile_,
          .main_window_factory_ =
              [this](int window_id) {
                return CreateMainWindow(MakeMainWindowContext(window_id));
              },
          .quit_handler_ = quit_handler_});

  RegisterMainWindowCommandActions(
      executor_, profile_, speech_service_, *scada_services_.session_service,
      *main_window_manager_,
      [this](bool login) {
        if (login) {
          login_handler_();
        } else {
          // TODO: Logoff.
        }
      },
      global_commands_);

  selection_command_router_ =
      std::make_shared<SelectionCommandRouter>(SelectionCommandRouterContext{
          .selection_commands_ = selection_commands_});

  // Opens windows.
  main_window_manager_->Init();

  event_dispatcher_ = std::make_unique<EventDispatcher>(EventDispatcherContext{
      executor_, node_event_provider_, local_events_, profile_,
      [this](bool has_events) { OnEvents(has_events); },
      ui_command_registry_.action_manager()});

  singletons_.emplace(std::make_shared<PageCommands>(PageCommandsContext{
      executor_, global_commands_, profile_, *main_window_manager_}));
}

MainWindowModule::~MainWindowModule() {}

MainWindowContext MainWindowModule::MakeMainWindowContext(int window_id) {
  auto main_command_router_factory = [this](MainWindowInterface& main_window,
                                            DialogService& dialog_service) {
    assert(scada_services_.session_service);
    return std::make_unique<MainWindowCommandRouter>(
        MainWindowCommandRouterContext{executor_, main_window, dialog_service,
                                       *scada_services_.session_service,
                                       global_commands_});
  };

  auto main_menu_factory =
      [this](MainWindowInterface& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& global_commands,
             aui::MenuModel& context_menu_model) {
        assert(scada_services_.session_service);

        return std::make_unique<MainMenuModel>(MainMenuContext{
            .executor_ = executor_,
            .main_window_manager_ = *main_window_manager_,
            .main_window_ = main_window,
            .favourites_ = favourites_,
            .file_cache_ = file_cache_,
            .admin_ = scada_services_.session_service->HasPrivilege(
                scada::Privilege::Configure),
            .profile_ = profile_,
            .view_manager_ = view_manager,
            .command_handler_ = global_commands,
            .dialog_service_ = dialog_service,
            .context_menu_model_ = context_menu_model,
            .commands_ = global_commands_,
            .ui_command_registry_ = ui_command_registry_});
      };

  auto context_menu_factory = [this](MainWindowInterface& main_window,
                                     CommandHandler& command_handler) {
    return std::make_unique<ContextMenuModel>(
        main_window, ui_command_registry_.command_manager(), command_handler);
  };

  assert(scada_services_.session_service);

  auto status_bar_model =
      StatusBarModelBuilder{executor_,
                            *scada_services_.session_service,
                            node_event_provider_,
                            local_events_,
                            node_service_,
                            profile_}
          .Build();

  auto connection_info_provider = [this] {
    assert(scada_services_.session_service);
    return scada_services_.session_service->GetHostName();
  };

  return MainWindowContext{
      executor_, ui_command_registry_, window_id, node_command_handler_,
      file_manager_, *main_window_manager_, profile_,
      /*opened_view_factory=*/
      std::bind_front(&MainWindowModule::CreateOpenedView, this),
      main_command_router_factory, selection_command_router_,
      std::move(status_bar_model), context_menu_factory, main_menu_factory,
      connection_info_provider, progress_host_};
}

void MainWindowModule::OnEvents(bool has_events) {
  const auto& window_info = GetWindowInfo(ID_EVENT_VIEW);
  for (MainWindow& main_window : main_window_manager_->main_windows()) {
    bool events_shown = main_window.FindViewByType(window_info.name) != nullptr;
    if (has_events != events_shown) {
      if (has_events && profile_.event_auto_show) {
        main_window.OpenPane(window_info, /*activate=*/false);
      } else if (!has_events && profile_.event_auto_hide) {
        main_window.ClosePane(window_info);
      }
    }

    main_window.SetWindowFlashing(has_events && profile_.event_flash_window);
  }
}

std::unique_ptr<OpenedView> MainWindowModule::CreateOpenedView(
    MainWindow& main_window,
    WindowDefinition& window_def) {
  // TODO: Add a UT. Ensure the controller is properly initialized.

  const auto* window_info = FindWindowInfoByName(window_def.type);
  if (!window_info) {
    BOOST_LOG_TRIVIAL(error)
        << "Window type " << window_def.type << " not found.";
    return nullptr;
  }

  // Initialize defaults.
  if (window_def.size.empty() && !window_info->size.empty()) {
    window_def.size = window_info->size;
  }

  auto opened_view = std::make_unique<OpenedView>(OpenedViewContext{
      .executor_ = executor_,
      .main_window_ = &main_window,
      .window_info_ = *window_info,
      .window_def_ = window_def,
      .dialog_service_ = main_window.GetDialogService(),
      .controller_factory_ = controller_factory_,
      .popup_menu_handler_ =
          std::bind_front(&MainWindow::ShowPopupMenu, &main_window),
      .default_node_command_handler_ = std::bind_front(
          &MainWindow::ExecuteDefaultNodeCommand, &main_window)});

  opened_view->Init();

  assert(scada_services_.session_service);

  auto opened_view_commands =
      std::make_unique<OpenedViewCommandRouter>(OpenedViewCommandRouterContext{
          .executor_ = executor_,
          .selection_command_router_ = selection_command_router_});

  // Must be called after `OpenedView::Init` is called, so it creates the
  // controller.
  opened_view_commands->SetContext(opened_view.get());
  auto* opened_view_ptr = opened_view.get();
  auto& dialog_service = main_window.GetDialogService();
  opened_view_commands->AddCommandHandlers(
      opened_view_commands_.CreateCommandHandlers(
          OpenedViewCommandFactoryContext{
              .executor_ = executor_,
              .dialog_service_ = dialog_service,
              .session_service_ = *scada_services_.session_service,
              .node_service_ = node_service_,
              .task_manager_ = task_manager_,
              .local_events_ = local_events_,
              .profile_ = profile_,
              .create_tree_ = create_tree_,
              .controller_ = opened_view_ptr->controller(),
              .print_service_ = print_service_,
              .export_model_getter_ =
                  [opened_view_ptr] {
                    return opened_view_ptr->controller().GetExportModel();
                  },
              .time_model_getter_ =
                  [opened_view_ptr] {
                    return opened_view_ptr->controller().GetTimeModel();
                  },
              .window_title_getter_ =
                  [opened_view_ptr] {
                    return opened_view_ptr->GetWindowTitle();
                  },
              .print_view_handler_ =
                  [opened_view_ptr](PrintService& print_service) {
                    opened_view_ptr->Print(print_service);
                  },
              .created_node_handler_ =
                  [this, opened_view_ptr](NodeRef node) -> Awaitable<void> {
                auto def = co_await MakeWindowDefinitionAsync(
                    executor_, &kNodePropertyWindowInfo, node,
                    /*expand_groups=*/false);
                co_await ::OpenView(&opened_view_ptr->main_window(), def, true);
                co_return;
              }}));

  opened_view->commands = std::move(opened_view_commands);

  return opened_view;
}
