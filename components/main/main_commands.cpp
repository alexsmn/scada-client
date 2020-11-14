#include "components/main/main_commands.h"

#include "base/path_service.h"
#include "client_paths.h"
#include "client_utils.h"
#include "common/event_fetcher.h"
#include "common_resources.h"
#include "components/about/about_dialog.h"
#include "components/configuration_export/excel_configuration_commands.h"
#include "components/favourites//add_favourites_dialog.h"
#include "components/main/main_window.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager.h"
#include "components/prompt/prompt_dialog.h"
#include "core/session_service.h"
#include "services/dialog_service.h"
#include "services/favourites.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/speech.h"
#include "window_info.h"

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
    {ID_MODUS2_MODE, &Profile::modus2},
    {0, NULL}};

static bool Profile::*GetOption(UINT id) {
  for (int i = 0; options[i].id; ++i)
    if (options[i].id == id)
      return options[i].option;
  return NULL;
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
    : MainCommandsContext{std::move(context)} {}

MainCommands::~MainCommands() {}

CommandHandler* MainCommands::GetCommandHandler(unsigned command_id) {
  auto* active_view = main_window_.GetActiveView();
  if (active_view) {
    CommandHandler* handler =
        active_view->commands->GetCommandHandler(command_id);
    if (handler)
      return handler;
  }

  switch (command_id) {
    case ID_APP_ABOUT:
    case ID_HELP_MANUAL:
    case ID_ACKNOWLEDGE_ALL:
    case ID_EXPORT_CONFIGURATION_TO_EXCEL:
    case ID_IMPORT_CONFIGURATION_FROM_EXCEL:
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
      return active_view ? this : NULL;

      /*case ID_PRINT:
        return active_view && active_view->window_info().printable() ? this
                                                                     : NULL;*/

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
    if (!win_info->createable())
      return NULL;
    if (win_info->requires_admin_rights() &&
        !session_service_.HasPrivilege(scada::Privilege::Configure)) {
      return NULL;
    }
    return this;
  }

  if (GetOption(command_id))
    return this;

  return NULL;
}

bool MainCommands::IsCommandEnabled(unsigned command_id) const {
  auto* active_view = main_window_.GetActiveView();

  switch (command_id) {
    case ID_ACKNOWLEDGE_ALL:
      return !event_fetcher_.unacked_events().empty() ||
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

  return true;
}

bool MainCommands::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_TOOLBAR_LEFT:
    case ID_TOOLBAR_TOP:
    case ID_TOOLBAR_HIDDEN:
      return main_window_.GetPrefs().toolbar_position == command_id;
  }

  if (const WindowInfo* win_info = FindWindowInfo(command_id)) {
    return (win_info->flags & WIN_SING) &&
           main_window_.FindOpenedViewByType(command_id);
  }

  if (bool Profile::*option = GetOption(command_id))
    return profile_.*option;

  return false;
}

void MainCommands::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_HELP_MANUAL: {
      WindowDefinition def(GetWindowInfo(ID_WEB_VIEW));
      def.title = L"Документация";
      def.path = base::FilePath(
          FILE_PATH_LITERAL("http://www.telecontrol.ru/workplace_manual"));
      main_window_.OpenView(def, true);
      return;
    }

    case ID_APP_ABOUT:
      ShowAboutDialog(dialog_service_);
      return;

    case ID_ACKNOWLEDGE_ALL:
      event_fetcher_.AcknowledgeAll();
      local_events_.AcknowledgeAll();
      return;

    case ID_EXPORT_CONFIGURATION_TO_EXCEL:
      ExportConfigurationToExcel(node_service_, dialog_service_);
      return;

    case ID_IMPORT_CONFIGURATION_FROM_EXCEL:
      ImportConfigurationFromExcel(node_service_, task_manager_,
                                   dialog_service_);
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

    case ID_PAGE_RENAME: {
      auto& page = main_window_.current_page();
      auto title = page.title;
      if (RunPromptDialog(dialog_service_, L"Имя:", L"Переименование", title))
        main_window_.SetPageTitle(title);
      return;
    }

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

    case ID_VIEW_ADD_TO_FAVOURITES:
      AddToFavourites();
      return;
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

  assert(false);
}

void MainCommands::AddToFavourites() {
  auto* view = main_window_.GetActiveView();
  if (!view)
    return;

  if (view->window_info().is_pane())
    return;

  view->Save();

  auto definition = view->window_def();
  definition.title = view->GetWindowTitle();

  ShowAddFavouritesDialog(dialog_service_,
                          {favourites_, std::move(definition)});
}

void MainCommands::ShowRenameWindowDialog() {
  auto* view = main_window_.GetActiveView();
  if (!view)
    return;

  if (view->window_info().is_pane())
    return;

  std::wstring title = view->GetWindowTitle();
  if (RunPromptDialog(dialog_service_, L"Имя:", L"Переименовать", title))
    view->SetUserTitle(title);
}
