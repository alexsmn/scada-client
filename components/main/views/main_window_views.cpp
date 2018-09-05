#include "components/main/views/main_window_views.h"

#include "base/win/win_util2.h"
#include "common_resources.h"
#include "components/main/opened_view.h"
#include "components/main/views/native_main_window.h"
#include "components/main/views/toolbar_controller.h"
#include "components/main/views/view_manager_views.h"
#include "controller.h"
#include "core/session_service.h"
#include "services/page.h"
#include "services/profile.h"
#include "simple_menu_command_handler.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/events/event_utils.h"
#include "ui/views/background.h"
#include "ui/views/controls/menu/menu_2.h"
#include "views/client_utils_views.h"
#include "window_info.h"

namespace {

const int kToolbarWidth = 90;
const int kToolbarHeight = 27;

}  // namespace

MainWindowViews::MainWindowViews(MainWindowContext&& context)
    : MainWindow{std::move(context), dialog_service_} {
  CreateToolbar();

  view_manager_.reset(new ViewManagerViews{*this, *this});

  set_parent_owned(false);
  // TODO: Use theme color.
  set_background(new views::ColorBackground(SkColorSetRGB(227, 227, 227)));

  auto main_menu_model = main_menu_factory_(
      *this, dialog_service_, *view_manager_, *commands_, *context_menu_model_);

  main_window_ = new NativeMainWindow{NativeMainWindowContext{
      this, std::move(main_menu_model), status_bar_model_, *commands_}};

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

void MainWindowViews::CreateToolbar() {
  toolbar_.reset(new views::Toolbar);
  toolbar_->set_controller(this);
  toolbar_->set_vertical(GetPrefs().toolbar_position != ID_TOOLBAR_TOP);
  AddChildView(toolbar_.get());

  assert(commands_);
  toolbar_controller_ = std::make_unique<ToolbarController>(
      action_manager_, *toolbar_, *commands_);
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
  SimpleMenuCommandHandler handler{*commands_};
  ui::SimpleMenuModel model{&handler};
  model.AddItem(ID_VIEW_ADD_TO_FAVOURITES, L"В избранное");
  model.AddItem(ID_VIEW_CHANGE_TITLE, L"Переименовать");
  model.AddSeparator(ui::NORMAL_SEPARATOR);
  model.AddItem(ID_VIEW_CLOSE, L"Закрыть");

  views::Menu2 menu{&model};
  menu.RunContextMenuAt(point);
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
  base::string16 server = FormatHostName(connection_info_provider_());

  static base::string16 application_title = win_util::LoadResourceString(
      WTL::ModuleHelper::GetResourceInstance(), IDR_MAINFRAME);
  base::string16 page = view_manager_->current_page().GetTitle();

  static base::string16 title_format_string = win_util::LoadResourceString(
      WTL::ModuleHelper::GetResourceInstance(), IDS_MAIN_WINDOW_TITLE);
  return base::StringPrintf(title_format_string.c_str(),
                            application_title.c_str(), page.c_str(),
                            server.c_str());
}

void MainWindowViews::SetToolbarPosition(unsigned position) {
  toolbar_->set_vertical(position == ID_TOOLBAR_LEFT);
  toolbar_->SetVisible(position != ID_TOOLBAR_HIDDEN);
  Layout();
}

bool MainWindowViews::ExecuteWindowsCommand(int command_id) {
  CommandHandler* handler = commands_->GetCommandHandler(command_id);
  if (handler) {
    handler->ExecuteCommand(command_id);
    return true;
  }
  return false;
}

void MainWindowViews::UpdateTitle() {
  main_window_->SetWindowText(GetWindowTitle().c_str());
}
