#include "opened_view.h"

#include "command_handler.h"
#include "components/main/main_window.h"
#include "components/main/main_window_util.h"
#include "controller.h"
#include "export_model.h"
#include "print_util.h"
#include "window_info.h"

OpenedView::OpenedView(OpenedViewContext&& context)
    : OpenedViewContext{std::move(context)} {
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

  update_working_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(100),
                              this, &OpenedView::UpdateWorking);

  //  image_ =
  //  ui::ResourceBundle::GetSharedInstance().GetNamedImage(window_info_.type);
}

OpenedView::~OpenedView() {}

void OpenedView::Activate() {
  if (main_window_)
    main_window_->ActivateView(*this);
}

void OpenedView::SetUserTitle(const std::wstring_view& title) {
  assert(!window_info().is_pane());

  if (user_title_ != title) {
    user_title_ = std::wstring{title};
    UpdateTitle();
  }
}

std::wstring OpenedView::GetWindowTitle() const {
  // don't allow custom titles for predefined windows
  if (window_info().is_pane())
    return window_info().title;

  if (!user_title_.empty())
    return user_title_;

  if (!title_.empty())
    return title_;

  return window_info().title;
}

void OpenedView::UpdateTitle() {
  std::wstring title = GetWindowTitle();
  if (working_)
    title += L" [Выполнение]";

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

void OpenedView::SetTitle(const std::wstring_view& title) {
  assert(!window_info().is_pane());
  if (title_ != title) {
    title_ = std::wstring{title};
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
  WORD shift = 0;
  if (::GetAsyncKeyState(VK_SHIFT))
    shift |= MK_SHIFT;
  if (::GetAsyncKeyState(VK_CONTROL))
    shift |= MK_CONTROL;

  ::ExecuteDefaultNodeCommand(main_window_, node, shift);
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
