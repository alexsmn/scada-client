#include "components/main/opened_view.h"

#include "command_handler.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "controller.h"
#include "export_model.h"
#include "print_util.h"
#include "window_info.h"

#if defined(UI_QT)
#include <QWidget>
#elif defined(UI_WT)
#include <Wt/WWidget.h>
#endif

using namespace std::chrono_literals;

OpenedView::OpenedView(OpenedViewContext&& context)
    : OpenedViewContext{std::move(context)},
      update_working_timer_{context.executor_} {
  assert(controller_factory_);
  controller_ = controller_factory_(window_def_.window_info().command_id, *this,
                                    dialog_service_);
  if (!controller_)
    throw std::runtime_error{"View type not found"};

  title_ = user_title_ = window_def_.title;

  auto* view = controller_->Init(window_def_);
  if (!view)
    throw std::runtime_error{"Can't create widget"};
  view_ = view;

#if defined(UI_VIEWS)
  view_->set_parent_owned(false);
  view_->set_drop_controller(this);
#endif

  update_working_timer_.StartRepeating(100ms, [this] { UpdateWorking(); });

  //  image_ =
  //  ui::ResourceBundle::GetSharedInstance().GetNamedImage(window_info_.type);
}

OpenedView::~OpenedView() {
  delete view_;
}

void OpenedView::Activate() {
  if (main_window_)
    main_window_->ActivateView(*this);
}

void OpenedView::SetUserTitle(std::u16string_view title) {
  assert(!window_info().is_pane());

  if (user_title_ != title) {
    user_title_.assign(title.data(), title.size());
    UpdateTitle();
  }
}

std::u16string OpenedView::GetWindowTitle() const {
  // don't allow custom titles for predefined windows
  if (window_info().is_pane())
    return std::u16string{window_info().title};

  if (!user_title_.empty())
    return user_title_;

  if (!title_.empty())
    return title_;

  return std::u16string{window_info().title};
}

void OpenedView::UpdateTitle() {
  std::u16string title = GetWindowTitle();
  if (working_)
    title += u" [Выполнение]";

  assert(main_window_);
  main_window_->OnViewTitleUpdated(*this, title);
}

void OpenedView::Close() {
  assert(main_window_);
  main_window_->CloseView(*this);
}

void OpenedView::SetModified(bool modified) {
  modified_ = modified;
}

ContentsModel* OpenedView::GetContentsModel() {
  return controller_->GetContentsModel();
}

void OpenedView::SetSelection(const scada::NodeId& item_id) {
  controller_->ShowContainedItem(item_id);
}

void OpenedView::UpdateWorking() {
  bool working = controller_->IsWorking();
  if (working != working_) {
    working_ = working;
    UpdateTitle();
  }
}

void OpenedView::SetTitle(std::u16string_view title) {
  assert(!window_info().is_pane());
  if (title_ != title) {
    title_.assign(title.data(), title.size());
    UpdateTitle();
  }
}

void OpenedView::Save() {
  window_def_.Clear();
  window_def_.title = user_title_;
  controller_->Save(window_def_);

  SetModified(false);
}

void OpenedView::OpenView(const WindowDefinition& def) {
  assert(main_window_);
  main_window_->OpenView(def, true);
}

void OpenedView::ExecuteDefaultNodeCommand(const NodeRef& node) {
  default_node_command_handler_(node);
}

ContentsModel* OpenedView::GetActiveContentsModel() {
  OpenedView* view = main_window_ ? main_window_->active_data_view() : nullptr;
  return view ? view->controller().GetContentsModel() : nullptr;
}

void OpenedView::AddContentsObserver(ContentsObserver& observer) {
  main_window_->AddContentsObserver(observer);
}

void OpenedView::RemoveContentsObserver(ContentsObserver& observer) {
  main_window_->RemoveContentsObserver(observer);
}

void OpenedView::Print(PrintService& print_service) {
  auto* export_model = controller_->GetExportModel();
  if (!export_model)
    return;

  auto export_data = export_model->GetExportData();
  std::visit([&print_service](auto& data) { ::Print(print_service, data); },
             export_data);
}

void OpenedView::Focus() {
  main_window_->SetActiveView(this);
}

void OpenedView::ShowPopupMenu(unsigned resource_id,
                               const aui::Point& point,
                               bool right_click) {
  if (resource_id == 0)
    resource_id = window_info().menu;

  if (resource_id == 0)
    resource_id = IDR_ITEM_POPUP;

  popup_menu_handler_(resource_id, point, right_click);
}
