#include "main_window/main_window_module.h"

#include "common/master_data_services.h"
#include "components/change_password/change_password_command_builder.h"
#include "controller/controller_context.h"
#include "controller/controller_registry.h"
#include "events/event_fetcher.h"
#include "main_window/action_manager.h"
#include "main_window/actions.h"
#include "main_window/configuration_commands.h"
#include "main_window/context_menu_model.h"
#include "main_window/main_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_commands.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_module.h"
#include "main_window/opened_view_commands.h"
#include "main_window/page_commands.h"
#include "main_window/selection_commands.h"
#include "main_window/status/status_bar_model_builder.h"
#include "profile/profile.h"
#include "services/event_dispatcher.h"

MainWindowModule::MainWindowModule(MainWindowModuleContext&& context)
    : MainWindowModuleContext{std::move(context)},
      action_manager_{std::make_unique<ActionManager>()} {
  AddGlobalActions(*action_manager_, node_service_);

  main_window_manager_ =
      std::make_unique<MainWindowManager>(MainWindowManagerContext{
          .profile_ = profile_,
          .main_window_factory_ =
              [this](int window_id) {
                return main_window_factory_(MakeMainWindowContext(window_id));
              },
          .quit_handler_ = quit_handler_});

  // |main_window_manager_| must be assigned.
  main_window_manager_->Init();

  event_dispatcher_ = std::make_unique<EventDispatcher>(EventDispatcherContext{
      executor_, event_fetcher_, local_events_, profile_,
      [this](bool has_events) { OnEvents(has_events); }, *action_manager_});

  singletons_.emplace(std::make_shared<PageCommands>(
      PageCommandsContext{global_commands_, profile_, *main_window_manager_}));
}

MainWindowModule ::~MainWindowModule() {}

MainWindowContext MainWindowModule::MakeMainWindowContext(int window_id) {
  auto login_handler = [this](bool login) {
    if (login) {
      login_handler_();
    } else {
      master_data_services_.SetServices({});
    }
  };

  auto main_commands_factory = [this, login_handler](
                                   MainWindow& main_window,
                                   DialogService& dialog_service) {
    return std::make_unique<MainWindowCommands>(MainWindowCommandsContext{
        main_window, task_manager_, dialog_service, master_data_services_,
        event_fetcher_, node_service_, local_events_, favourites_, speech_,
        profile_, *main_window_manager_, login_handler, global_commands_});
  };

  auto main_menu_factory =
      [this](MainWindow& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& global_commands,
             aui::MenuModel& context_menu_model) {
        return std::make_unique<MainMenuModel>(MainMenuContext{
            .executor_ = executor_,
            .main_window_manager_ = *main_window_manager_,
            .main_window_ = main_window,
            .favourites_ = favourites_,
            .file_cache_ = file_cache_,
            .admin_ =
                master_data_services_.HasPrivilege(scada::Privilege::Configure),
            .profile_ = profile_,
            .view_manager_ = view_manager,
            .command_handler_ = global_commands,
            .dialog_service_ = dialog_service,
            .context_menu_model_ = context_menu_model,
            .commands_ = global_commands_});
      };

  auto context_menu_factory = [this](MainWindow& main_window,
                                     CommandHandler& command_handler) {
    return std::make_unique<ContextMenuModel>(main_window, *action_manager_,
                                              command_handler);
  };

  auto configuration_commands = std::make_shared<ConfigurationCommands>(
      selection_commands_, executor_, timed_data_service_,
      master_data_services_, profile_, local_events_, task_manager_);

  singletons_.emplace(configuration_commands);
  configuration_commands->Register();

  selection_commands_.AddCommand(
      ChangePasswordCommandBuilder{.local_events_ = local_events_,
                                   .profile_ = profile_,
                                   .session_service_ = master_data_services_}
          .Build());

  selection_commands_object_ =
      std::make_shared<SelectionCommands>(SelectionCommandsContext{
          executor_, task_manager_, master_data_services_, event_fetcher_,
          file_cache_, profile_, *main_window_manager_, node_service_,
          selection_commands_});

  auto status_bar_model =
      StatusBarModelBuilder{executor_,      master_data_services_,
                            event_fetcher_, local_events_,
                            node_service_,  profile_}
          .Build();

  auto connection_info_provider = [this] {
    return master_data_services_.GetHostName();
  };

  return MainWindowContext{
      executor_, *action_manager_, window_id, node_command_handler_,
      file_manager_, *main_window_manager_, profile_,
      /*opened_view_factory=*/
      std::bind_front(&MainWindowModule::CreateOpenedView, this),
      main_commands_factory, selection_commands_object_,
      std::move(status_bar_model), context_menu_factory, main_menu_factory,
      connection_info_provider, progress_host_};
}

void MainWindowModule::OnEvents(bool has_events) {
  for (auto& p : main_window_manager_->main_windows()) {
    auto& main_window = *p.second;
    bool events_shown =
        main_window.FindOpenedViewByType(kEventWindowInfo) != nullptr;
    if (has_events != events_shown) {
      if (has_events && profile_.event_auto_show) {
        main_window.OpenPane(kEventWindowInfo, /*activate=*/false);
      } else if (!has_events && profile_.event_auto_hide) {
        main_window.ClosePane(kEventWindowInfo);
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
    LOG(ERROR) << "Window type " << window_def.type << " not found.";
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

  auto opened_view_commands =
      std::make_unique<OpenedViewCommands>(OpenedViewCommandsContext{
          executor_, selection_commands_object_, task_manager_,
          master_data_services_, master_data_services_, event_fetcher_,
          master_data_services_, timed_data_service_, node_service_,
          *action_manager_, local_events_, file_cache_, profile_,
          *main_window_manager_, create_tree_});

  // Must be called after `OpenedView::Init` is called, so it creates the
  // controller.
  opened_view_commands->SetContext(opened_view.get(),
                                   &main_window.GetDialogService());

  opened_view->commands = std::move(opened_view_commands);

  return opened_view;
}
