#include "client/components/main/main_window.h"

#include "client/components/main/main_commands.h"
#include "client/services/connection_state_reporter.h"
#include "client/client_application.h"
#include "client/client_utils.h"
#include "client/services/profile.h"
#include "client/components/main/events_helper.h"
#include "client/components/main/opened_view.h"
#include "client/components/main/view_manager.h"
#include "client/controller.h"
#include "client/window_info.h"
#include "client/contents_model.h"
#include "client/contents_observer.h"

MainWindow::MainWindow(MainWindowContext&& context)
    : MainWindowContext(std::move(context)),
      connection_state_reporter_(std::make_unique<ConnectionStateReporter>(session_service_, local_events_)),
      main_commands_(std::make_unique<MainCommands>(MainCommandsContext{
          *this,
          node_service_,
          task_manager_,
          event_manager_,
          local_events_,
          profile_,
          session_service_,
          new_main_window_,
          find_closed_page_,
          speech_,
          favourites_,
          GetDialogService(),
      })),
      weak_factory_(this) {
}

void MainWindow::Init(ViewManager& view_manager) {
  view_manager_ = &view_manager;

  events_helper_ = std::make_unique<EventsHelper>(*this, event_manager_, local_events_, profile_);

  Page* page = nullptr;
  Profile::PageMap& pages = profile_.pages();
  Profile::PageMap::iterator i = pages.find(GetPrefs().page_id);
  if (i != pages.end())
    page = &i->second;
  else if (!pages.empty())
    page = find_closed_page_();

  if (!page)
    page = &profile_.CreatePage();

  OpenPage(*page);
}

MainWindow::~MainWindow() {
}

void MainWindow::BeforeClose() {
  SavePage();
  SetActiveView(nullptr);
  SetActiveDataView(nullptr);
  view_manager_->ClosePage();
}

MainWindowDef& MainWindow::GetPrefs() const {
  DCHECK_NE(window_id_, 0);
  return profile_.GetMainWindow(window_id_);
}

void MainWindow::Close() {
  close_handler_(window_id_);
}

void MainWindow::SetActiveView(OpenedView* view) {
  if (active_view_ == view)
    return;

  if (active_view_)
    active_view_->controller().selection().set_change_handler(nullptr);

  active_view_ = view;

  if (active_view_)
    active_view_->controller().selection().set_change_handler([this]{ OnSelectionChanged(); });

  const WindowInfo* window_info = nullptr;
  if (view)
    window_info = &view->window_info();

  if (view && !view->window_info().is_pane())
    SetActiveDataView(view);

  OnSelectionChanged();
}

void MainWindow::OpenPane(unsigned pane_id, bool make_active) {
  OpenView(WindowDefinition(GetWindowInfo(pane_id)), make_active);
}

void MainWindow::ClosePane(unsigned pane_id) {
  OpenedView* view = view_manager_->FindViewByType(pane_id);
  if (view)
    CloseView(*view);
}

void MainWindow::OnActiveViewChanged(OpenedView* view) {
  SetActiveView(view);
}

void MainWindow::SetActiveDataView(OpenedView* view) {
  if (view == active_data_view_)
    return;

  if (active_data_view_) {
    auto* contents = active_data_view_->controller().GetContentsModel();
    if (contents)
      contents->set_contents_observer(nullptr);
  }

  active_data_view_ = view;

  {
    bool set = false;
    auto* contents = active_data_view_ ? active_data_view_->controller().GetContentsModel() : nullptr;
    if (contents) {
      contents->set_contents_observer(this);
      OnContainedItemsUpdate(contents->GetContainedItems());
      set = true;
    }
    if (!set)
      OnContainedItemsUpdate({});
  }
}

OpenedView* MainWindow::FindViewToRecycle(unsigned type) {
  for (auto& p : view_manager_->views()) {
    OpenedView* view = p.view;
    if (view->window_info().command_id == type &&
        view->window_info().can_insert_item() &&
        !view->locked())
      return view;
  }
  return nullptr;
}

OpenedView* MainWindow::OpenView(const WindowDefinition& def, bool make_active) {
  if (!def.window_info())
    return nullptr;

  OpenedView* after_view = !def.window_info()->is_pane() ? active_data_view() : nullptr;
  return view_manager_->OpenView(def, make_active, after_view);
}

void MainWindow::OnViewClosed(OpenedView& view, WindowDefinition& definition) {
  if (&view == active_data_view_)
    SetActiveDataView(nullptr);

  LOG(INFO) << "Window " << view.window_info().title << " closed.";

  view.Save(definition);

  // Don't remove window definition if window is single to allow parameter storing.
  if (view.window_info().is_pane()) {
    definition.visible = false;

  } else {
    // append trash
    Page& trash = profile_.trash();
    trash.AddWindow(definition);
    while (trash.GetWindowCount() > 10)
      trash.DeleteWindow(0);

    auto& page = view_manager_->current_page();
    page.DeleteWindow(page.FindWindow(definition));
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
  for (auto& p : view_manager_->views()) {
    if (p.definition->path == path)
      return p.view;
  }
  return nullptr;
}

OpenedView* MainWindow::FindOpenedViewByType(unsigned view_id) {
  return view_manager_->FindViewByType(view_id);
}

void MainWindow::SavePage() {
  LOG(INFO) << "Save page " << view_manager_->current_page().id;

  view_manager_->SavePage();

  Profile::PageMap& pages = profile_.pages();
  pages[view_manager_->current_page().id] = view_manager_->current_page();
}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(WindowDefinition& def) {
  if (!def.window_info())
    return nullptr;

  // Initialize defaults.
  const WindowInfo& window_info = *def.window_info();
  if (def.size == gfx::Size()) {
    if (window_info.cx && window_info.cy)
      def.size = gfx::Size(window_info.cx, window_info.cy);
  }

  return std::make_unique<OpenedView>(OpenedViewContext{
      this,
      def,
      timed_data_service_,
      node_service_,
      portfolio_manager_,
      task_manager_,
      profile_,
      action_manager_,
      local_events_,
      event_manager_,
      file_cache_,
      node_management_service_,
      history_service_,
      method_service_,
      favourites_,
      GetDialogService(),
      find_opened_view_,
      session_service_,
      monitored_item_service_,
  });
}

void MainWindow::OnViewTitleUpdated(OpenedView& view, const base::string16& title) {
  view_manager_->SetViewTitle(view, title);
}

void MainWindow::ActivateView(OpenedView& view) {
  view_manager_->ActivateView(view);
}

void MainWindow::CloseView(OpenedView& view) {
  if (view.controller().CanClose())
    view_manager_->CloseView(view);
}

void MainWindow::AddContentsObserver(ContentsObserver& observer) {
  contents_observers_.AddObserver(&observer);
}

void MainWindow::RemoveContentsObserver(ContentsObserver& observer) {
  contents_observers_.RemoveObserver(&observer);
}

void MainWindow::SetPageTitle(const base::string16& title) {
  const_cast<Page&>(current_page()).title = title;
  UpdateTitle();
}

void MainWindow::OnContainedItemsUpdate(const std::set<scada::NodeId>& item_ids) {
  FOR_EACH_OBSERVER(ContentsObserver, contents_observers_, OnContainedItemsUpdate(item_ids));
}

void MainWindow::OnContainedItemChanged(const scada::NodeId& item_id, bool added) {
  FOR_EACH_OBSERVER(ContentsObserver, contents_observers_, OnContainedItemChanged(item_id, added));
}
