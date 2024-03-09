#include "main_window/main_window_module.h"

#include "common/master_data_services.h"
#include "components/events/events_component.h"
#include "controller/controller_context.h"
#include "controller/controller_registry.h"
#include "events/event_fetcher.h"
#include "main_window/action_manager.h"
#include "main_window/actions.h"
#include "main_window/context_menu_model.h"
#include "main_window/main_commands.h"
#include "main_window/main_menu_model.h"
#include "main_window/main_window.h"
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
      PageCommandsContext{main_commands_, profile_, *main_window_manager_}));
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
    return std::make_unique<MainCommands>(MainCommandsContext{
        main_window, task_manager_, dialog_service, master_data_services_,
        event_fetcher_, node_service_, local_events_, favourites_, speech_,
        profile_, *main_window_manager_, login_handler, main_commands_});
  };

  auto main_menu_factory =
      [this](MainWindow& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& main_commands,
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
            .command_handler_ = main_commands,
            .dialog_service_ = dialog_service,
            .context_menu_model_ = context_menu_model,
            .commands_ = main_commands_});
      };

  auto context_menu_factory = [this](MainWindow& main_window,
                                     CommandHandler& main_commands) {
    return std::make_unique<ContextMenuModel>(main_window, *action_manager_,
                                              main_commands);
  };

  auto selection_commands =
      std::make_shared<SelectionCommands>(SelectionCommandsContext{
          executor_, task_manager_, master_data_services_,
          master_data_services_, event_fetcher_, timed_data_service_,
          local_events_, file_cache_, profile_, *main_window_manager_,
          node_service_, selection_commands_});

  auto view_commands_factory = [this, selection_commands](
                                   OpenedView& opened_view,
                                   DialogService& dialog_service) {
    auto opened_view_commands =
        std::make_unique<OpenedViewCommands>(OpenedViewCommandsContext{
            executor_, selection_commands, task_manager_, master_data_services_,
            master_data_services_, event_fetcher_, master_data_services_,
            timed_data_service_, node_service_, portfolio_manager_,
            *action_manager_, local_events_, favourites_, file_cache_, profile_,
            *main_window_manager_, create_tree_});

    opened_view_commands->SetContext(&opened_view, &dialog_service);

    return opened_view_commands;
  };

  auto status_bar_model =
      StatusBarModelBuilder{executor_, master_data_services_, event_fetcher_,
                            node_service_, profile_}
          .Build();

  auto connection_info_provider = [this] {
    return master_data_services_.GetHostName();
  };

  return MainWindowContext{executor_,
                           *action_manager_,
                           window_id,
                           node_command_handler_,
                           file_manager_,
                           *main_window_manager_,
                           profile_,
                           controller_factory_,
                           main_commands_factory,
                           view_commands_factory,
                           selection_commands,
                           std::move(status_bar_model),
                           context_menu_factory,
                           main_menu_factory,
                           connection_info_provider,
                           progress_host_};
}

void MainWindowModule::OnEvents(bool has_events) {
  for (auto& p : main_window_manager_->main_windows()) {
    auto& main_window = *p.second;
    bool events_shown =
        main_window.FindOpenedViewByType(kEventWindowInfo) != nullptr;
    if (has_events != events_shown) {
      if (has_events && profile_.event_auto_show)
        main_window.OpenPane(kEventWindowInfo, false);
      else if (!has_events && profile_.event_auto_hide)
        main_window.ClosePane(kEventWindowInfo);
    }

    main_window.SetWindowFlashing(has_events && profile_.event_flash_window);
  }
}
