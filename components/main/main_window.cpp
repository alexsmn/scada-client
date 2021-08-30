#include "components/main/main_window.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/selection_commands.h"
#include "components/main/view_manager.h"
#include "contents_model.h"
#include "contents_observer.h"
#include "controller.h"
#include "selection_model.h"
#include "services/profile.h"
#include "simple_menu_command_handler.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/models/simple_menu_model.h"
#include "window_info.h"

namespace {

class TabPopupMenu : public ui::SimpleMenuModel {
 public:
  explicit TabPopupMenu(CommandHandler& commands)
      : ui::SimpleMenuModel{&handler_}, handler_{commands} {
    AddItem(ID_VIEW_ADD_TO_FAVOURITES, L"В избранное");
    AddItem(ID_VIEW_CHANGE_TITLE, L"Переименовать");
    AddSeparator(ui::NORMAL_SEPARATOR);
    AddItem(ID_VIEW_CLOSE, L"Закрыть");
  }

 private:
  SimpleMenuCommandHandler handler_;
};

}  // namespace

MainWindow::MainWindow(MainWindowContext&& context,
                       DialogService& dialog_service)
    : MainWindowContext{std::move(context)},
      commands_{main_commands_factory_(*this, dialog_service)},
      context_menu_model_{context_menu_factory_(*this, *commands_)},
      tab_popup_menu_{std::make_unique<TabPopupMenu>(*commands_)} {}

void MainWindow::Init(ViewManager& view_manager) {
  view_manager_ = &view_manager;

  Page* page = nullptr;
  auto& pages = profile_.pages;
  auto i = pages.find(GetPrefs().page_id);
  if (i != pages.end())
    page = &i->second;
  else if (!pages.empty())
    page = main_window_manager_.FindFirstNotOpenedPage();

  if (!page)
    page = &profile_.CreatePage();

  OpenPage(*page);
}

MainWindow::~MainWindow() {}

void MainWindow::BeforeClose() {
  SavePage();

  SetActiveView(nullptr);
  SetActiveDataView(nullptr);

  view_manager_->ClosePage();
}

MainWindowDef& MainWindow::GetPrefs() const {
  assert(window_id_ != 0);
  return profile_.GetMainWindow(window_id_);
}

void MainWindow::Close() {
  main_window_manager_.CloseMainWindow(window_id_);
}

void MainWindow::SetActiveView(OpenedView* view) {
  if (active_view_ == view)
    return;

  if (active_view_) {
    if (auto* selection_model = active_view_->controller().GetSelectionModel())
      selection_model->change_handler = nullptr;
    selection_commands_->SetContext(nullptr, nullptr, nullptr, nullptr);
  }

  active_view_ = view;

  if (active_view_) {
    auto* selection_model = active_view_->controller().GetSelectionModel();
    selection_commands_->SetContext(this, &GetDialogService(),
                                    &active_view_->controller(),
                                    selection_model);
    if (selection_model)
      selection_model->change_handler = [this] { OnSelectionChanged(); };
  }

  const WindowInfo* window_info = nullptr;
  if (view)
    window_info = &view->window_info();

  if (view && !view->window_info().is_pane())
    SetActiveDataView(view);

  OnSelectionChanged();
}

void MainWindow::OpenPane(const WindowInfo& window_info, bool make_active) {
  OpenView(WindowDefinition(window_info), make_active);
}

void MainWindow::ClosePane(const WindowInfo& window_info) {
  auto* opened_view = view_manager_->FindViewByType(window_info);
  if (opened_view)
    CloseView(*opened_view);
}

void MainWindow::OnActiveViewChanged(OpenedView* opened_view) {
  SetActiveView(opened_view);
}

void MainWindow::SetActiveDataView(OpenedView* view) {
  if (view == active_data_view_)
    return;

  if (active_data_view_) {
    auto* contents = active_data_view_->controller().GetContentsModel();
    if (contents)
      contents->contents_observer = nullptr;
  }

  active_data_view_ = view;

  {
    bool set = false;
    auto* contents = active_data_view_
                         ? active_data_view_->controller().GetContentsModel()
                         : nullptr;
    if (contents) {
      contents->contents_observer = this;
      if (!view_manager_->is_closing_page()) {
        OnContentsChanged(contents->GetContainedItems());
        set = true;
      }
    }
    if (!set)
      OnContentsChanged({});
  }
}

OpenedView* MainWindow::FindViewToRecycle(unsigned type) {
  for (auto* opened_view : view_manager_->views()) {
    if (opened_view->window_info().command_id == type &&
        opened_view->window_info().can_insert_item() && !opened_view->locked())
      return opened_view;
  }
  return nullptr;
}

OpenedView* MainWindow::OpenView(const WindowDefinition& def,
                                 bool make_active) {
  OpenedView* after_view =
      !def.window_info().is_pane() ? active_data_view() : nullptr;
  return view_manager_->OpenView(def, make_active, after_view);
}

void MainWindow::OnViewClosed(OpenedView& view) {
  if (&view == active_data_view_)
    SetActiveDataView(nullptr);

  LOG(INFO) << "Window " << std::wstring{view.window_info().title}
            << " closed.";

  if (view_manager_->is_closing_page())
    return;

  view.Save();

  // Don't remove window definition if window is single to allow parameter
  // storing.
  if (view.window_info().is_pane()) {
    view.window_def().visible = false;

  } else {
    // append trash
    auto& trash = profile_.trash;
    trash.AddWindow(view.window_def());
    while (trash.GetWindowCount() > 10)
      trash.DeleteWindow(0);

    auto& page = view_manager_->current_page();
    page.DeleteWindow(page.FindWindowDef(view.window_def()));
  }
}

void MainWindow::OpenPage(const Page& page) {
  LOG(INFO) << "Open page " << page.id;

  view_manager_->OpenPage(page);

  GetPrefs().page_id = page.id;

  UpdateTitle();

  OnSelectionChanged();
}

const Page& MainWindow::current_page() const {
  return view_manager_->current_page();
}

OpenedView* MainWindow::GetActiveView() {
  return active_view_;
}

OpenedView* MainWindow::GetActiveDataView() {
  return active_data_view_;
}

OpenedView* MainWindow::FindOpenedViewByFilePath(const base::FilePath& path) {
  for (auto* opened_view : view_manager_->views()) {
    if (opened_view->window_def().path == path)
      return opened_view;
  }
  return nullptr;
}

OpenedView* MainWindow::FindOpenedViewByType(const WindowInfo& window_info) {
  return view_manager_->FindViewByType(window_info);
}

void MainWindow::SavePage() {
  LOG(INFO) << "Save page " << view_manager_->current_page().id;

  view_manager_->SavePage();

  auto& pages = profile_.pages;
  pages[view_manager_->current_page().id] = view_manager_->current_page();
}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(WindowDefinition& def) {
  // Initialize defaults.
  const WindowInfo& window_info = def.window_info();
  if (def.size == gfx::Size()) {
    if (window_info.cx && window_info.cy)
      def.size = gfx::Size(window_info.cx, window_info.cy);
  }

  auto& dialog_service = GetDialogService();

  auto opened_view = std::make_unique<OpenedView>(OpenedViewContext{
      executor_, this, def, dialog_service, controller_factory_,
      *context_menu_model_, file_registry_});

  opened_view->commands = view_commands_factory_(*opened_view, dialog_service);

  return opened_view;
}

void MainWindow::OnViewTitleUpdated(OpenedView& view,
                                    const std::wstring& title) {
  view_manager_->SetViewTitle(view, title);
}

void MainWindow::ActivateView(OpenedView& view) {
  view_manager_->ActivateView(view);
}

void MainWindow::CloseView(OpenedView& view) {
  if (view_manager_->is_closing_page())
    return;

  if (view.controller().CanClose())
    view_manager_->CloseView(view);
}

void MainWindow::SetPageTitle(const std::wstring& title) {
  const_cast<Page&>(current_page()).title = title;
  UpdateTitle();
}

void MainWindow::AddContentsObserver(ContentsObserver& observer) {
  contents_observers_.AddObserver(&observer);
}

void MainWindow::RemoveContentsObserver(ContentsObserver& observer) {
  contents_observers_.RemoveObserver(&observer);
}

void MainWindow::OnContentsChanged(const std::set<scada::NodeId>& item_ids) {
  for (auto& o : contents_observers_)
    o.OnContentsChanged(item_ids);
}

void MainWindow::OnContainedItemChanged(const scada::NodeId& item_id,
                                        bool added) {
  for (auto& o : contents_observers_)
    o.OnContainedItemChanged(item_id, added);
}

void MainWindow::SplitView(OpenedView& view, bool vertically) {
  view_manager_->SplitView(view, vertically);
}
