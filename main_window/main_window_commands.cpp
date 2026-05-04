#include "main_window/main_window_commands.h"

#include "aui/dialog_service.h"
#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "ui/common/client_utils.h"
#include "resources/common_resources.h"
#include "modules/about/about_dialog.h"
#include "modules/web/web_component.h"
#include "controller/command_registry.h"
#include "controller/window_info.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/opened_view/opened_view_commands.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"
#include "scada/session_service.h"
#include "services/speech_service.h"

#include <Windows.h>
#include <shellapi.h>
#include <filesystem>
#include <stdexcept>

#include <atlres.h>
#include <shellapi.h>

#if defined(UI_QT)
#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#endif

namespace {

struct Option {
  unsigned id;
  bool Profile::*option;
};

const Option options[] = {
    {ID_SHOW_WRITEOK, &Profile::show_write_ok},
    {ID_SHOW_EVENTS, &Profile::event_auto_show},
    {ID_HIDE_EVENTS, &Profile::event_auto_hide},
    {ID_WRITE_CONFIRMATION, &Profile::control_confirmation},
    {ID_OPT_SPEECH, &Profile::speech_enabled},
    {ID_EVENT_FLASH_WINDOW, &Profile::event_flash_window},
    {ID_EVENT_PLAY_SOUND, &Profile::event_play_sound},
    {0, nullptr}};

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
  settings.setValue("LocaleName", QString::fromStdString(std::string(locale_name)));
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
  return {
      .command_id = command_id,
      .title = std::u16string{title},
      .menu_group = MenuGroup::MAIN_WINDOW_SETTINGS,
      .execute_handler =
          [executor = std::move(executor),
           locale_name](const GlobalCommandContext& context) {
            ApplyLanguageSelection(executor, context, locale_name);
          },
      .checked_handler = [is_russian](const GlobalCommandContext&) {
        return IsRussianLocale(GetSelectedLocaleName()) == is_russian;
      }};
}
#endif

void OpenPublicFolder() {
  std::filesystem::path path;
  if (!base::PathService::Get(client::DIR_PUBLIC, &path)) {
    return;
  }

  ShellExecuteW(/*hwnd=*/nullptr, /*lpOperation=*/L"open",
                /*lpFile=*/path.wstring().c_str(), /*lpParameters=*/nullptr,
                /*lpDirectory=*/nullptr,
                /*nShowCmd=*/SW_SHOWNORMAL);
}

BasicCommand<GlobalCommandContext> MakeOptionCommand(
    unsigned command_id,
    std::u16string_view title,
    Profile& profile,
    bool MainWindowDef::*option) {
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

Awaitable<void> ShowRenameWindowDialogAsync(AnyExecutor executor,
                                            OpenedViewInterface& view,
                                            DialogService& dialog_service,
                                            std::u16string current_view_title) {
  auto title = co_await RunPromptDialog(
      dialog_service, Translate("Name:"), Translate("Rename"),
      current_view_title);
  // TODO: Capture weak pointer.
  view.SetWindowTitle(title);
  co_return;
}

void ShowRenameWindowDialog(AnyExecutor executor,
                            MainWindowInterface& main_window,
                            DialogService& dialog_service) {
  auto* view = main_window.GetActiveView();
  if (!view || view->GetWindowInfo().is_pane()) {
    return;
  }

  CoSpawn(executor,
          [executor, view, &dialog_service,
           current_view_title = view->GetWindowTitle()] {
            return ShowRenameWindowDialogAsync(executor, *view, dialog_service,
                                               current_view_title);
          });
}

}  // namespace

MainWindowCommands::MainWindowCommands(MainWindowCommandsContext&& context)
    : MainWindowCommandsContext{std::move(context)} {
  global_commands_.AddCommand(
      {.command_id = ID_ACKNOWLEDGE_ALL,
       .execute_handler =
           [this](const GlobalCommandContext&) {
             node_event_provider_.AcknowledgeAllEvents();
             local_events_.AcknowledgeAll();
           },
       .enabled_handler =
           [this](const GlobalCommandContext&) {
             return !node_event_provider_.unacked_events().empty() ||
                    !local_events_.events().empty();
           }});

  global_commands_.AddCommand(
      {.command_id = ID_WINDOW_NEW,
       .execute_handler =
           [this](const GlobalCommandContext&) {
             main_window_manager_.CreateMainWindow();
           }});

  global_commands_.AddCommand(
      {.command_id = ID_VIEW_CHANGE_TITLE,
       .execute_handler =
           [executor = executor_](const GlobalCommandContext& context) {
             ShowRenameWindowDialog(executor, context.main_window,
                                    context.dialog_service);
           },
       .enabled_handler =
           [](const GlobalCommandContext& context) {
             auto* active_view = context.main_window.GetActiveView();
             return active_view && !active_view->GetWindowInfo().is_pane();
           },
       .available_handler =
           [](const GlobalCommandContext& context) {
             return context.main_window.GetActiveView() != nullptr;
           }});

  global_commands_.AddCommand(
      {.command_id = ID_VIEW_PUBLIC_FOLDER,
       .execute_handler =
           [](const GlobalCommandContext&) { OpenPublicFolder(); }});

  global_commands_.AddCommand(
      {.command_id = ID_LOGIN,
       .execute_handler =
           [this](const GlobalCommandContext&) { login_handler_(true); },
       .enabled_handler =
           [this](const GlobalCommandContext&) {
             return !session_service_.IsConnected();
           }});

  global_commands_.AddCommand(
      {.command_id = ID_LOGOFF,
       .execute_handler =
           [this](const GlobalCommandContext&) { login_handler_(false); },
       .enabled_handler =
           [this](const GlobalCommandContext&) {
             return session_service_.IsConnected();
           }});

#if defined(UI_QT)
  global_commands_.AddCommand(
      {.command_id = ID_WINDOW_SPLIT_HORZ,
       .execute_handler =
           [](const GlobalCommandContext& context) {
             if (auto* active_view = context.main_window.GetActiveView()) {
               context.main_window.SplitView(*active_view, true);
             }
           },
       .available_handler =
           [](const GlobalCommandContext& context) {
             return context.main_window.GetActiveView() != nullptr;
           }});

  global_commands_.AddCommand(
      {.command_id = ID_WINDOW_SPLIT_VERT,
       .execute_handler =
           [](const GlobalCommandContext& context) {
             if (auto* active_view = context.main_window.GetActiveView()) {
               context.main_window.SplitView(*active_view, false);
             }
           },
       .available_handler =
           [](const GlobalCommandContext& context) {
             return context.main_window.GetActiveView() != nullptr;
           }});
#endif

  for (const auto& opt : options) {
    if (!opt.id) {
      continue;
    }

    global_commands_.AddCommand(
        {.command_id = opt.id,
         .menu_group = MenuGroup::MAIN_WINDOW_SETTINGS,
         .execute_handler =
             [this, option = opt.option](const GlobalCommandContext&) {
               profile_.*option = !(profile_.*option);
             },
         .enabled_handler =
             [this, command_id = opt.id](const GlobalCommandContext&) {
               return command_id != ID_OPT_SPEECH || speech_service_.is_ok();
             },
         .checked_handler =
             [this, option = opt.option](const GlobalCommandContext&) {
               return profile_.*option;
             }});
  }

  global_commands_.AddCommand(
      MakeOptionCommand(ID_VIEW_TOOLBAR, Translate("Toolbar"), profile_,
                        &MainWindowDef::toolbar));

  global_commands_.AddCommand(MakeOptionCommand(ID_VIEW_STATUS_BAR,
                                                Translate("Status Bar"), profile_,
                                                &MainWindowDef::status_bar));

  global_commands_.AddCommand(
      {.command_id = ID_APP_ABOUT,
       .execute_handler = [](const GlobalCommandContext& context) {
         ShowAboutDialog(context.dialog_service);
       }});

#if defined(UI_QT)
  global_commands_.AddCommand(
      {.command_id = ID_ABOUT_QT,
       .execute_handler = [](const GlobalCommandContext& context) {
         QMessageBox::aboutQt(context.dialog_service.GetParentWidget());
       }});

  global_commands_.AddCommand(MakeLanguageCommand(
      executor_, ID_LANGUAGE_ENGLISH, Translate("English"), "en",
      /*is_russian=*/false));
  global_commands_.AddCommand(MakeLanguageCommand(
      executor_, ID_LANGUAGE_RUSSIAN, Translate("Russian"), "ru_RU",
      /*is_russian=*/true));
#endif

#if !defined(UI_WT)
  global_commands_.AddCommand(
      {.command_id = ID_HELP_MANUAL,
       .execute_handler = [executor = executor_](
                              const GlobalCommandContext& context) {
         WindowDefinition def(kWebWindowInfo);
         def.title = Translate("Documentation");
         def.path = std::filesystem::path(
             L"http://www.telecontrol.ru/workplace_manual");
         CoSpawn(executor, [&main_window = context.main_window,
                            def = std::move(def)]() -> Awaitable<void> {
           co_await main_window.OpenView(def, true);
         });
       }});
#endif
}

MainWindowCommands::~MainWindowCommands() {}

MainWindowCommandHandler::MainWindowCommandHandler(
    MainWindowCommandHandlerContext&& context)
    : MainWindowCommandHandlerContext{std::move(context)},
      command_context_{main_window_, dialog_service_} {}

MainWindowCommandHandler::~MainWindowCommandHandler() {}

CommandHandler* MainWindowCommandHandler::GetCommandHandler(
    unsigned command_id) {
  auto* active_view = main_window_.GetActiveView();
  if (active_view) {
    // TODO: Refactor to remove the static cast.
    if (auto* handler = static_cast<OpenedView*>(active_view)
                            ->commands->GetCommandHandler(command_id)) {
      return handler;
    }
  }

  switch (command_id) {
    case ID_OPEN_TABLE:
      command_id = ID_TABLE_VIEW;
      break;
    case ID_OPEN_GRAPH:
      command_id = ID_GRAPH_VIEW;
      break;
    case ID_TIMED_DATA_VIEW:
      command_id = ID_TIMED_DATA_VIEW;
      break;
    case ID_OPEN_EVENTS:
      command_id = ID_EVENT_JOURNAL_VIEW;
      break;
  }

#ifdef NDEBUG
  if (command_id == ID_NODES_VIEW) {
    // Hide NodeView for Release build.
    return nullptr;
  }
#endif

  if (const WindowInfo* win_info = FindWindowInfo(command_id)) {
    if (!win_info->createable()) {
      return nullptr;
    }
    if (win_info->requires_admin_rights() &&
        !session_service_.HasPrivilege(scada::Privilege::Configure)) {
      return nullptr;
    }
    return this;
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    if (command_id == ID_VIEW_ADD_TO_FAVOURITES && !active_view) {
      return nullptr;
    }
    if (!command->available_handler ||
        command->available_handler(command_context_)) {
      return this;
    }
  }

  return nullptr;
}

bool MainWindowCommandHandler::IsCommandEnabled(unsigned command_id) const {
  if (command_id == ID_VIEW_ADD_TO_FAVOURITES) {
    auto* active_view = main_window_.GetActiveView();
    return active_view && !active_view->GetWindowInfo().is_pane();
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    return !command->enabled_handler ||
           command->enabled_handler(command_context_);
  }

  return true;
}

bool MainWindowCommandHandler::IsCommandChecked(unsigned command_id) const {
  if (const WindowInfo* window_info = FindWindowInfo(command_id)) {
    return (window_info->flags & WIN_SING) &&
           main_window_.FindViewByType(window_info->name);
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    return command->checked_handler &&
           command->checked_handler(command_context_);
  }

  return false;
}

void MainWindowCommandHandler::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_OPEN_TABLE:
      command_id = ID_TABLE_VIEW;
      break;
    case ID_OPEN_GRAPH:
      command_id = ID_GRAPH_VIEW;
      break;
    case ID_TIMED_DATA_VIEW:
      command_id = ID_TIMED_DATA_VIEW;
      break;
    case ID_OPEN_EVENTS:
      command_id = ID_EVENT_JOURNAL_VIEW;
      break;
  }

  // Check create window command.
  if (const WindowInfo* win_info = FindWindowInfo(command_id)) {
    assert(win_info->createable());
    /*if (win_info->flags & WIN_SING) {
      OpenedView* view = view_manager_->FindViewByType(win_info->type);
      if (view) {
        view_manager_->CloseView(*view);
        return;
      }
    }*/
    CoSpawn(executor_, [this, def = WindowDefinition(*win_info)]()
                         -> Awaitable<void> {
      co_await main_window_.OpenView(def, true);
    });
    return;
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context_);
    }
    return;
  }

  assert(false);
}
