#include "main_window/main_window.h"

#include "aui/key_codes.h"
#include "base/boost_log.h"
#include "base/promise_executor.h"
#include "controller/contents_model.h"
#include "controller/contents_observer.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "filesystem/file_manager.h"
#include "main_window/initial_page.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "main_window/selection_commands.h"
#include "main_window/tab_popup_menu.h"
#include "main_window/view_manager.h"
#include "profile/profile.h"

bool BaseMainWindow::g_hide_for_testing = false;

BaseMainWindow::BaseMainWindow(MainWindowContext&& context,
                               DialogService& dialog_service)
    : MainWindowContext{std::move(context)},
      commands_{main_commands_factory_(*this, dialog_service)},
      context_menu_model_{context_menu_factory_(*this, *commands_)},
      tab_popup_menu_{std::make_unique<TabPopupMenu>(*commands_)} {}

void BaseMainWindow::Init(ViewManager& view_manager) {
  view_manager_ = &view_manager;

  const Page* page = nullptr;
  const auto& pages = profile_.pages;
  if (auto i = profile_.pages.find(GetPrefs().page_id); i != pages.end()) {
    page = &i->second;
  } else if (!pages.empty()) {
    page = main_window_manager_.FindFirstNotOpenedPage();
  }

  if (!page) {
    page = &profile_.AddPage(CreateInitialPage());
  }

  OpenPage(*page);
}

BaseMainWindow::~BaseMainWindow() {}

void BaseMainWindow::CleanupForTesting() {
  active_view_ = nullptr;
  active_data_view_ = nullptr;

  view_manager_->ClosePage();
}

void BaseMainWindow::BeforeClose() {
  SaveCurrentPage();

  SetActiveView(nullptr);
  SetActiveDataView(nullptr);

  view_manager_->ClosePage();
}

MainWindowDef& BaseMainWindow::GetPrefs() const {
  assert(window_id_ != 0);
  return profile_.GetMainWindow(window_id_);
}

void BaseMainWindow::Close() {
  main_window_manager_.CloseMainWindow(window_id_);
}

void BaseMainWindow::SetActiveView(OpenedView* view) {
  if (active_view_ == view) {
    return;
  }

  if (active_view_) {
    if (auto* selection_model =
            active_view_->controller().GetSelectionModel()) {
      selection_model->change_handler = nullptr;
    }
    selection_commands_->SetContext(nullptr, nullptr, nullptr, nullptr,
                                    nullptr);
  }

  active_view_ = view;

  if (active_view_) {
    auto* selection_model = active_view_->controller().GetSelectionModel();

    selection_commands_->SetContext(this, &GetDialogService(), active_view_,
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

void BaseMainWindow::OpenPane(const WindowInfo& window_info, bool activate) {
  OpenView(WindowDefinition(window_info), activate);
}

void BaseMainWindow::ClosePane(const WindowInfo& window_info) {
  auto* opened_view = view_manager_->FindViewByType(window_info.name);
  if (opened_view) {
    CloseView(*opened_view);
  }
}

void BaseMainWindow::OnActiveViewChanged(OpenedView* opened_view) {
  SetActiveView(opened_view);
}

void BaseMainWindow::SetActiveDataView(OpenedView* view) {
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

OpenedView* BaseMainWindow::FindViewToRecycle(unsigned type) {
  const auto& views = view_manager_->views();

  auto i = std::ranges::find_if(views, [type](OpenedView* opened_view) {
    return opened_view->window_info().command_id == type &&
           opened_view->window_info().can_insert_item() &&
           !opened_view->locked();
  });

  return i != views.end() ? *i : nullptr;
}

promise<OpenedViewInterface*> BaseMainWindow::OpenView(
    const WindowDefinition& window_def,
    bool make_active) {
  promise<void> download_promise =
      window_def.path.empty()
          ? make_resolved_promise()
          : IgnoreResult(file_manager_.DownloadFileFromServer(window_def.path));

  return download_promise
      // TODO: Fix captured `this` and run under the local executor.
      .then([this, window_def, make_active]() -> OpenedViewInterface* {
        const auto* window_info = FindWindowInfoByName(window_def.type);
        OpenedView* after_view = window_info && !window_info->is_pane()
                                     ? active_data_view()
                                     : nullptr;
        return view_manager_->OpenView(window_def, make_active, after_view);
      });
}

void BaseMainWindow::OnViewClosed(OpenedView& view) {
  if (&view == active_data_view_) {
    SetActiveDataView(nullptr);
  }

  BOOST_LOG_TRIVIAL(info) << "Window " << view.window_info().title << " closed.";

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

void BaseMainWindow::OpenPage(const Page& page) {
  BOOST_LOG_TRIVIAL(info) << "Open page " << page.id;

  view_manager_->OpenPage(page);

  GetPrefs().page_id = page.id;

  UpdateTitle();

  OnSelectionChanged();
}

void BaseMainWindow::DeleteCurrentPage() {
  auto& page = current_page();
  profile_.pages.erase(page.id);

  // Select first not opened page.
  const Page* select_page = main_window_manager_.FindFirstNotOpenedPage();
  if (!select_page) {
    select_page = &profile_.AddPage(CreateInitialPage());
  }

  OpenPage(*select_page);
}

const Page& BaseMainWindow::current_page() const {
  return view_manager_->current_page();
}

OpenedView* BaseMainWindow::GetActiveView() const {
  return active_view_;
}

OpenedView* BaseMainWindow::GetActiveDataView() const {
  return active_data_view_;
}

OpenedView* BaseMainWindow::FindOpenedViewByFilePath(
    const std::filesystem::path& path) {
  for (auto* opened_view : view_manager_->views()) {
    if (opened_view->window_def().path == path)
      return opened_view;
  }
  return nullptr;
}

OpenedViewInterface* BaseMainWindow::FindViewByType(
    std::string_view window_type) const {
  return view_manager_->FindViewByType(window_type);
}

void BaseMainWindow::SaveCurrentPage() {
  BOOST_LOG_TRIVIAL(info) << "Save page " << view_manager_->current_page().id;

  view_manager_->SavePage();

  auto& pages = profile_.pages;
  pages[view_manager_->current_page().id] = view_manager_->current_page();
}

void BaseMainWindow::OnViewTitleUpdated(OpenedView& view,
                                        const std::u16string& title) {
  view_manager_->SetViewTitle(view, title);
}

void BaseMainWindow::ActivateView(const OpenedViewInterface& view) {
  // TODO: Remove the static cast.
  view_manager_->ActivateView(static_cast<const OpenedView&>(view));
}

void BaseMainWindow::CloseView(OpenedView& view) {
  if (view_manager_->is_closing_page())
    return;

  view_manager_->CloseView(view);
}

void BaseMainWindow::SetCurrentPageTitle(std::u16string_view title) {
  const_cast<Page&>(current_page()).title = title;
  UpdateTitle();
}

void BaseMainWindow::AddContentsObserver(ContentsObserver& observer) {
  contents_observers_.AddObserver(&observer);
}

void BaseMainWindow::RemoveContentsObserver(ContentsObserver& observer) {
  contents_observers_.RemoveObserver(&observer);
}

void BaseMainWindow::OnContentsChanged(
    const std::set<scada::NodeId>& item_ids) {
  for (auto& o : contents_observers_)
    o.OnContentsChanged(item_ids);
}

void BaseMainWindow::OnContainedItemChanged(const scada::NodeId& item_id,
                                            bool added) {
  for (auto& o : contents_observers_)
    o.OnContainedItemChanged(item_id, added);
}

void BaseMainWindow::SplitView(OpenedViewInterface& view, bool vertically) {
  // TODO: Remove the static cast.
  view_manager_->SplitView(static_cast<OpenedView&>(view), vertically);
}

void BaseMainWindow::ExecuteDefaultNodeCommand(const NodeRef& node) {
  aui::KeyModifiers key_modifiers{};
  if (::GetAsyncKeyState(VK_SHIFT)) {
    key_modifiers |= aui::ShiftModifier;
  }
  if (::GetAsyncKeyState(VK_CONTROL)) {
    key_modifiers |= aui::ControlModifier;
  }

  node_command_handler_(
      NodeCommandContext{this, GetDialogService(), node, key_modifiers});
}

const std::list<OpenedView*>& BaseMainWindow::opened_views() const {
  return view_manager_->views();
}

std::vector<OpenedViewInterface*> BaseMainWindow::GetOpenedViews() const {
  const auto& views = opened_views();
  return {views.begin(), views.end()};
}