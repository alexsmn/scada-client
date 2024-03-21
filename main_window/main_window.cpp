#include "main_window/main_window.h"

#include "aui/key_codes.h"
#include "base/promise_executor.h"
#include "controller/contents_model.h"
#include "controller/contents_observer.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "core/node_command_context.h"
#include "filesystem/file_manager.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "main_window/selection_commands.h"
#include "main_window/tab_popup_menu.h"
#include "main_window/view_manager.h"
#include "profile/profile.h"

MainWindow::MainWindow(MainWindowContext&& context,
                       DialogService& dialog_service)
    : MainWindowContext{std::move(context)},
      commands_{main_commands_factory_(*this, dialog_service)},
      context_menu_model_{context_menu_factory_(*this, *commands_)},
      tab_popup_menu_{std::make_unique<TabPopupMenu>(*commands_)} {}

void MainWindow::Init(ViewManager& view_manager) {
  view_manager_ = &view_manager;

  const Page* page = nullptr;
  const auto& pages = profile_.pages;
  if (auto i = profile_.pages.find(GetPrefs().page_id); i != pages.end()) {
    page = &i->second;
  } else if (!pages.empty()) {
    page = main_window_manager_.FindFirstNotOpenedPage();
  }

  if (!page) {
    page = &profile_.CreatePage();
  }

  OpenPage(*page);
}

MainWindow::~MainWindow() {}

void MainWindow::CleanupForTesting() {
  active_view_ = nullptr;
  active_data_view_ = nullptr;

  view_manager_->ClosePage();
}

void MainWindow::BeforeClose() {
  SaveCurrentPage();

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
  if (active_view_ == view) {
    return;
  }

  if (active_view_) {
    if (auto* selection_model =
            active_view_->controller().GetSelectionModel()) {
      selection_model->change_handler = nullptr;
    }
    selection_commands_->SetContext(nullptr, nullptr, nullptr, nullptr);
  }

  active_view_ = view;

  if (active_view_) {
    auto* selection_model = active_view_->controller().GetSelectionModel();
    selection_commands_->SetContext(this, &GetDialogService(),
                                    &active_view_->controller(),
                                    selection_model);
    if (selection_model) {
      selection_model->change_handler = [this] { OnSelectionChanged(); };
    }
  }

  if (view && !view->window_info().is_pane()) {
    SetActiveDataView(view);
  }

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
    ContentsModel* contents =
        active_data_view_ ? active_data_view_->controller().GetContentsModel()
                          : nullptr;
    if (contents) {
      contents->contents_observer = this;
      if (!view_manager_->is_closing_page()) {
        OnContentsChanged(contents->GetContainedItems());
        set = true;
      }
    }
    if (!set) {
      OnContentsChanged({});
    }
  }
}

OpenedView* MainWindow::FindViewToRecycle(unsigned type) {
  const auto& views = view_manager_->views();

  auto i = std::ranges::find_if(views, [type](OpenedView* opened_view) {
    return opened_view->window_info().command_id == type &&
           opened_view->window_info().can_insert_item() &&
           !opened_view->locked();
  });

  return i != views.end() ? *i : nullptr;
}

scada::status_promise<OpenedView*> MainWindow::OpenView(
    const WindowDefinition& window_def,
    bool make_active) {
  promise<void> download_promise =
      window_def.path.empty()
          ? make_resolved_promise()
          : IgnoreResult(file_manager_.DownloadFileFromServer(window_def.path));

  return download_promise
      // TODO: Fix captured `this` and run under the local executor.
      .then([this, window_def, make_active] {
        const auto* window_info = FindWindowInfoByName(window_def.type);
        OpenedView* after_view = window_info && !window_info->is_pane()
                                     ? active_data_view()
                                     : nullptr;
        return view_manager_->OpenView(window_def, make_active, after_view);
      });
}

void MainWindow::OnViewClosed(OpenedView& view) {
  if (&view == active_data_view_) {
    SetActiveDataView(nullptr);
  }

  LOG(INFO) << "Window " << view.window_info().title << " closed.";

  if (view_manager_->is_closing_page()) {
    return;
  }

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

void MainWindow::DeleteCurrentPage() {
  auto& page = current_page();
  profile_.pages.erase(page.id);

  // Select first not opened page.
  const Page* select_page = main_window_manager_.FindFirstNotOpenedPage();
  if (!select_page) {
    select_page = &profile_.CreatePage();
  }

  OpenPage(*select_page);
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

OpenedView* MainWindow::FindOpenedViewByFilePath(
    const std::filesystem::path& path) {
  for (auto* opened_view : view_manager_->views()) {
    if (opened_view->window_def().path == path)
      return opened_view;
  }
  return nullptr;
}

OpenedView* MainWindow::FindOpenedViewByType(const WindowInfo& window_info) {
  return view_manager_->FindViewByType(window_info);
}

void MainWindow::SaveCurrentPage() {
  LOG(INFO) << "Save page " << view_manager_->current_page().id;

  view_manager_->SavePage();

  auto& pages = profile_.pages;
  pages[view_manager_->current_page().id] = view_manager_->current_page();
}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(
    WindowDefinition& window_def) {
  return opened_view_factory_(*this, window_def);
}

void MainWindow::OnViewTitleUpdated(OpenedView& view,
                                    const std::u16string& title) {
  view_manager_->SetViewTitle(view, title);
}

void MainWindow::ActivateView(OpenedView& view) {
  view_manager_->ActivateView(view);
}

void MainWindow::CloseView(OpenedView& view) {
  if (view_manager_->is_closing_page())
    return;

  view_manager_->CloseView(view);
}

void MainWindow::SetCurrentPageTitle(std::u16string_view title) {
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

void MainWindow::ExecuteDefaultNodeCommand(const NodeRef& node) {
  aui::KeyModifiers key_modifiers{};
  if (::GetAsyncKeyState(VK_SHIFT)) {
    key_modifiers |= aui::ShiftModifier;
  }
  if (::GetAsyncKeyState(VK_CONTROL)) {
    key_modifiers |= aui::ControlModifier;
  }

  node_command_handler_(NodeCommandContext{this, node, key_modifiers});
}
