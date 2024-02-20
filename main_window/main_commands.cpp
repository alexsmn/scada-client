#include "main_window/main_commands.h"

#include "aui/dialog_service.h"
#include "aui/prompt_dialog.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/about/about_dialog.h"
#include "components/debugger/debugger.h"
#include "components/web/web_component.h"
#include "controller/window_info.h"
#include "events/node_event_provider.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager.h"
#include "profile/profile.h"
#include "scada/session_service.h"
#include "services/local_events.h"
#include "services/speech.h"

#include <windows.h>

#include <atlres.h>
#include <shellapi.h>

#if defined(UI_QT)
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

static bool Profile::*GetOption(UINT id) {
  for (int i = 0; options[i].id; ++i)
    if (options[i].id == id)
      return options[i].option;
  return nullptr;
}

void OpenPublicFolder() {
  base::FilePath path;
  if (!base::PathService::Get(client::DIR_PUBLIC, &path))
    return;
  ShellExecuteW(nullptr, L"open", path.value().c_str(), nullptr, nullptr,
                SW_SHOWNORMAL);
}

}  // namespace

MainCommands::MainCommands(MainCommandsContext&& context)
    : MainCommandsContext{std::move(context)},
      command_context_{main_window_, dialog_service_} {}

MainCommands::~MainCommands() {}

CommandHandler* MainCommands::GetCommandHandler(unsigned command_id) {
  auto* active_view = main_window_.GetActiveView();
  if (active_view) {
    if (auto* handler = active_view->commands->GetCommandHandler(command_id)) {
      return handler;
    }
  }

  switch (command_id) {
    case ID_APP_ABOUT:
    case ID_HELP_MANUAL:
    case ID_ACKNOWLEDGE_ALL:
    case ID_VIEW_PUBLIC_FOLDER:
    case ID_WINDOW_NEW:
    case ID_PAGE_NEW:
    case ID_PAGE_RENAME:
    case ID_PAGE_DELETE:
      return this;

    case ID_VIEW_ADD_TO_FAVOURITES:
    case ID_VIEW_CHANGE_TITLE:
    case ID_VIEW_CLOSE:
#if defined(UI_QT)
    case ID_WINDOW_SPLIT_HORZ:
    case ID_WINDOW_SPLIT_VERT:
#endif
      return active_view ? this : nullptr;

      /*case ID_PRINT:
        return active_view && active_view->window_info().printable() ? this
                                                                     :
        nullptr;*/

    case ID_TOOLBAR_TOP:
    case ID_TOOLBAR_LEFT:
    case ID_TOOLBAR_HIDDEN:
    case ID_VIEW_STATUS_BAR:
      return this;

    case ID_LOGIN:
    case ID_LOGOFF:
      return this;

#ifdef NDEBUG
    case ID_NODES_VIEW:
      // Hide NodeView for Release build.
      return nullptr;
#endif

#if defined(ID_ABOUT_QT)
    case ID_ABOUT_QT:
      return this;
#endif

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

  if (GetOption(command_id)) {
    return this;
  }

  if (const auto* command = main_commands_.FindCommand(command_id)) {
    return this;
  }

  return nullptr;
}

bool MainCommands::IsCommandEnabled(unsigned command_id) const {
  auto* active_view = main_window_.GetActiveView();

  switch (command_id) {
    case ID_ACKNOWLEDGE_ALL:
      return !node_event_provider_.unacked_events().empty() ||
             !local_events_.events().empty();

    case ID_OPT_SPEECH:
      return speech_.is_ok();

    case ID_VIEW_ADD_TO_FAVOURITES:
    case ID_VIEW_CHANGE_TITLE:
      return active_view && !active_view->window_info().is_pane();

    case ID_LOGIN:
      return !session_service_.IsConnected();
    case ID_LOGOFF:
      return session_service_.IsConnected();
  }

  if (auto* command = main_commands_.FindCommand(command_id)) {
    return !command->enabled_handler ||
           command->enabled_handler(command_context_);
  }

  return true;
}

bool MainCommands::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_TOOLBAR_LEFT:
    case ID_TOOLBAR_TOP:
    case ID_TOOLBAR_HIDDEN:
      return main_window_.GetPrefs().toolbar_position == command_id;
  }

  if (const WindowInfo* window_info = FindWindowInfo(command_id)) {
    return (window_info->flags & WIN_SING) &&
           main_window_.FindOpenedViewByType(*window_info);
  }

  if (bool Profile::*option = GetOption(command_id))
    return profile_.*option;

  if (const auto* command = main_commands_.FindCommand(command_id)) {
    return command->checked_handler &&
           command->checked_handler(command_context_);
  }

  return false;
}

void MainCommands::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
#if !defined(UI_WT)
    case ID_HELP_MANUAL: {
      WindowDefinition def(kWebWindowInfo);
      def.title = u"Документация";
      def.path = std::filesystem::path(
          FILE_PATH_LITERAL("http://www.telecontrol.ru/workplace_manual"));
      main_window_.OpenView(def, true);
      return;
    }
#endif

    case ID_APP_ABOUT:
      ShowAboutDialog(dialog_service_);
      return;

    case ID_ACKNOWLEDGE_ALL:
      node_event_provider_.AcknowledgeAllEvents();
      local_events_.AcknowledgeAll();
      return;

    case ID_WINDOW_NEW:
      main_window_manager_.CreateMainWindow();
      return;

    case ID_PAGE_NEW: {
      main_window_.SavePage();
      Page& page = profile_.CreatePage();
      main_window_.OpenPage(page);
      return;
    }

    case ID_PAGE_RENAME:
      RenameCurrentPage();
      return;

    case ID_PAGE_DELETE: {
      auto& page = main_window_.current_page();
      profile_.pages.erase(page.id);
      // Select first not opened page.
      Page* select_page = main_window_manager_.FindFirstNotOpenedPage();
      if (!select_page)
        select_page = &profile_.CreatePage();
      main_window_.OpenPage(*select_page);
      return;
    }

    case ID_VIEW_CHANGE_TITLE:
      ShowRenameWindowDialog();
      return;

    case ID_TOOLBAR_TOP:
    case ID_TOOLBAR_LEFT:
    case ID_TOOLBAR_HIDDEN:
      main_window_.GetPrefs().toolbar_position = command_id;
      main_window_.SetToolbarPosition(command_id);
      return;

    case ID_VIEW_PUBLIC_FOLDER:
      OpenPublicFolder();
      return;

    case ID_LOGIN:
    case ID_LOGOFF:
      login_handler_(command_id == ID_LOGIN);
      return;

#if defined(UI_QT)
    case ID_WINDOW_SPLIT_HORZ:
    case ID_WINDOW_SPLIT_VERT:
      if (auto* active_view = main_window_.GetActiveView())
        main_window_.SplitView(*active_view,
                               command_id == ID_WINDOW_SPLIT_HORZ);
      return;

    case ID_ABOUT_QT:
      return QMessageBox::aboutQt(dialog_service_.GetParentWidget());
#endif

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
    main_window_.OpenView(WindowDefinition(*win_info), true);
    return;
  }

  // Check option command.
  if (bool Profile::*option = GetOption(command_id)) {
    profile_.*option = !(profile_.*option);
    return;
  }

  if (const auto* command = main_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context_);
    }
    return;
  }

  assert(false);
}

promise<> MainCommands::RenameCurrentPage() {
  return RunPromptDialog(dialog_service_, u"Имя:", u"Переименование",
                         main_window_.current_page().title)
      .then([this](const std::u16string& title) {
        main_window_.SetPageTitle(title);
      });
}

promise<> MainCommands::ShowRenameWindowDialog() {
  auto* view = main_window_.GetActiveView();
  if (!view)
    return MakeRejectedPromise();

  if (view->window_info().is_pane())
    return MakeRejectedPromise();

  return RunPromptDialog(dialog_service_, u"Имя:", u"Переименовать",
                         view->GetWindowTitle())
      // TODO: Capture weak pointer.
      .then(std::bind_front(&OpenedView::SetUserTitle, view));
}
