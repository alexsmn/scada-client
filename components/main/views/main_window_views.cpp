#include "components/main/views/main_window_views.h"

#include "base/win/win_util2.h"
#include "common_resources.h"
#include "components/main/action_manager.h"
#include "components/main/main_commands.h"
#include "components/main/opened_view.h"
#include "components/main/views/main_menu_model.h"
#include "components/main/views/native_main_window.h"
#include "components/main/views/status_bar_controller.h"
#include "components/main/views/toolbar_controller.h"
#include "components/main/views/view_manager_views.h"
#include "controller.h"
#include "core/session_service.h"
#include "services/page.h"
#include "services/profile.h"
#include "ui/events/event_utils.h"
#include "ui/views/background.h"
#include "views/client_utils_views.h"
#include "window_info.h"

static const int kToolbarWidth = 90;
static const int kToolbarHeight = 27;

MainWindowViews::MainWindowViews(MainWindowContext&& context)
    : MainWindow{std::move(context), dialog_service_} {
  CreateToolbar();

  view_manager_.reset(new ViewManagerViews{*this, *this});

  set_parent_owned(false);
  // TODO: Use theme color.
  set_background(new views::ColorBackground(SkColorSetRGB(227, 227, 227)));

  const unsigned menu_id =
      session_service_.HasPrivilege(scada::Privilege::Configure)
          ? IDR_MAINFRAME
          : IDR_MAIN_USER;
  main_menu_ = std::make_unique<MainMenu>(
      *this, action_manager_, favourites_, file_cache_, profile_,
      dialog_service_, main_window_manager_, *view_manager_, menu_id);

  status_bar_controller_ =
      std::make_unique<StatusBarController>(StatusBarControllerContext{
          session_service_, event_manager_, node_service_});

  main_window_ = new NativeMainWindow{
      NativeMainWindowContext{this, *main_menu_, *status_bar_controller_}};
  auto& prefs = GetPrefs();
  main_window_->Init(prefs.bounds, prefs.maximized);

  dialog_service_.dialog_owning_window = GetWindowHandle();

  LoadAccelerators();

  view_manager_->Init();
  Init(*view_manager_);

  Layout();
}

MainWindowViews::~MainWindowViews() {
  assert(main_window_);

  BeforeClose();
  view_manager_.reset();

  MainWindowDef& prefs = GetPrefs();
  main_window_->GetPrefs(prefs.bounds, prefs.maximized);
  main_window_->Close();
}

void MainWindowViews::OnNativeWindowClosed() {
  status_bar_controller_.reset();
  main_menu_.reset();
}

void MainWindowViews::CreateToolbar() {
  toolbar_.reset(new views::Toolbar);
  toolbar_->set_controller(this);
  toolbar_->set_vertical(GetPrefs().toolbar_position != ID_TOOLBAR_TOP);
  AddChildView(toolbar_.get());

  toolbar_controller_ = std::make_unique<ToolbarController>(
      action_manager_, *toolbar_, *main_commands_);
}

gfx::NativeView MainWindowViews::GetWindowHandle() const {
  return main_window_->m_hWnd;
}

void MainWindowViews::OnSelectionChanged() {
  if (!view_manager_->is_closing_page())
    toolbar_controller_->UpdateCommands();
}

void MainWindowViews::OnShowTabPopupMenu(OpenedView& view,
                                         const gfx::Point& point) {
  CMenu menu;
  menu.CreatePopupMenu();
  menu.AppendMenu(MFT_STRING, ID_VIEW_ADD_TO_FAVOURITES, L"В избранное");
  menu.AppendMenu(MFT_STRING, ID_VIEW_CHANGE_TITLE, L"Переименовать");
  menu.AppendMenu(MFT_STRING | MFT_SEPARATOR);
  menu.AppendMenu(MFT_STRING, ID_VIEW_CLOSE, L"Закрыть");

  ShowPopupMenu(GetWindowHandle(), menu, point, false);
}

void MainWindowViews::OnExecuteToolbarCommand(views::Toolbar& sender,
                                              unsigned command_id) {
  ExecuteWindowsCommand(command_id);
}

void MainWindowViews::SetWindowFlashing(bool flashing) {
  main_window_->SetFlashing(flashing);
}

void MainWindowViews::Layout() {
  gfx::Rect bounds(0, 0, width(), height());
  if (toolbar_.get() && toolbar_->visible()) {
    toolbar_->SetBoundsRect(
        toolbar_->is_vertical()
            ? gfx::Rect(bounds.x(), bounds.y(), kToolbarWidth, bounds.height())
            : gfx::Rect(bounds.x(), bounds.y(), bounds.width(),
                        kToolbarHeight));
    if (toolbar_->is_vertical())
      bounds.Inset(kToolbarWidth, 0, 0, 0);
    else
      bounds.Inset(0, kToolbarHeight, 0, 0);
  }

  if (view_manager_)
    view_manager_->GetView().SetBoundsRect(bounds);
}

void MainWindowViews::LoadAccelerators() {
  HACCEL accelerator_table = AtlLoadAccelerators(IDR_MAINFRAME);
  DCHECK(accelerator_table);

  // We have to copy the table to access its contents.
  int count = CopyAcceleratorTable(accelerator_table, 0, 0);
  if (count == 0) {
    // Nothing to do in that case.
    return;
  }

  ACCEL* accelerators = static_cast<ACCEL*>(malloc(sizeof(ACCEL) * count));
  CopyAcceleratorTable(accelerator_table, accelerators, count);

  views::FocusManager* focus_manager = GetFocusManager();
  DCHECK(focus_manager);

  // Let's fill our own accelerator table.
  for (int i = 0; i < count; ++i) {
    ui::Accelerator accelerator(
        static_cast<ui::KeyboardCode>(accelerators[i].key),
        ui::GetModifiersFromACCEL(accelerators[i]));
    accelerator_table_[accelerator] = accelerators[i].cmd;

    // Also register with the focus manager.
    focus_manager->RegisterAccelerator(
        accelerator, ui::AcceleratorManager::kNormalPriority, this);
  }

  // We don't need the Windows accelerator table anymore.
  free(accelerators);
}

bool MainWindowViews::AcceleratorPressed(const ui::Accelerator& accelerator) {
  std::map<ui::Accelerator, int>::const_iterator iter =
      accelerator_table_.find(accelerator);
  DCHECK(iter != accelerator_table_.end());
  int command_id = iter->second;

  /*chrome::BrowserCommandController* controller =
  browser_->command_controller(); if (!controller->block_command_execution())
    UpdateAcceleratorMetrics(accelerator, command_id);
  return chrome::ExecuteCommand(browser_.get(), command_id);*/

  return ExecuteWindowsCommand(command_id);
}

bool MainWindowViews::CanHandleAccelerators() const {
  return true;
}

base::string16 MainWindowViews::GetWindowTitle() const {
  base::string16 server =
      base::SysNativeMBToWide(session_service_.GetHostName());
  if (server.empty()) {
    static base::string16 local_server_string = win_util::LoadResourceString(
        WTL::ModuleHelper::GetResourceInstance(), IDS_LOCAL_SERVER);
    server = local_server_string;
  }

  static base::string16 application_title = win_util::LoadResourceString(
      WTL::ModuleHelper::GetResourceInstance(), IDR_MAINFRAME);
  base::string16 page = view_manager_->current_page().GetTitle();

  static base::string16 title_format_string = win_util::LoadResourceString(
      WTL::ModuleHelper::GetResourceInstance(), IDS_MAIN_WINDOW_TITLE);
  return base::StringPrintf(title_format_string.c_str(),
                            application_title.c_str(), page.c_str(),
                            server.c_str());
}

void MainWindowViews::UpdateToolbarPosition() {
  toolbar_->set_vertical(GetPrefs().toolbar_position == ID_TOOLBAR_LEFT);
  toolbar_->SetVisible(GetPrefs().toolbar_position != ID_TOOLBAR_HIDDEN);
  Layout();
}

bool MainWindowViews::ExecuteWindowsCommand(int command_id) {
  CommandHandler* handler = main_commands_->GetCommandHandler(command_id);
  if (handler) {
    handler->ExecuteCommand(command_id);
    return true;
  }
  return false;
}

void MainWindowViews::UpdateTitle() {
  main_window_->UpdateTitle();
}
