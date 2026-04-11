#include "main_window/opened_view.h"

#include "aui/translation.h"
#include "base/utf_convert.h"
#include "common_resources.h"
#include "controller/command_handler.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "export/export_model.h"
#include "main_window/main_window.h"
#include "print/service/print_util.h"
#include "window_definition_builder.h"

#if defined(UI_QT)
#include <QWidget>
#elif defined(UI_WT)
#include <Wt/WWidget.h>
#endif

using namespace std::chrono_literals;

OpenedView::OpenedView(OpenedViewContext&& context)
    : OpenedViewContext{std::move(context)},
      update_working_timer_{context.executor_} {}

OpenedView::~OpenedView() {}

void OpenedView::Init() {
  assert(controller_factory_);
  controller_ =
      controller_factory_(window_info_.command_id, *this, dialog_service_);
  if (!controller_) {
    throw std::runtime_error{"View type not found"};
  }

  title_ = user_title_ = window_def_.title;

  view_ = controller_->Init(window_def_);
  if (!view_) {
    throw std::runtime_error{"Can't create widget"};
  }

  update_working_timer_.StartRepeating(100ms, [this] { UpdateWorking(); });

  //  image_ =
  //  ui::ResourceBundle::GetSharedInstance().GetNamedImage(window_info_.type);
}

void OpenedView::Activate() {
  if (main_window_)
    main_window_->ActivateView(*this);
}

void OpenedView::SetWindowTitle(std::u16string_view title) {
  assert(!window_info().is_pane());

  if (user_title_ != title) {
    user_title_.assign(title.data(), title.size());
    UpdateTitle();
  }
}

std::u16string OpenedView::GetWindowTitle() const {
  // WindowInfo::title is stored as an English literal in the source; run it
  // through Translate() so localized builds show the translated title.
  auto translated_info_title = [this] {
    return Translate(UtfConvert<char>(std::u16string_view{window_info().title}));
  };

  // don't allow custom titles for predefined windows
  if (window_info().is_pane())
    return translated_info_title();

  if (!user_title_.empty())
    return user_title_;

  if (!title_.empty())
    return title_;

  return translated_info_title();
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

ContentsModel* OpenedView::GetContents() {
  return controller_->GetContentsModel();
}

void OpenedView::Select(const scada::NodeId& node_id) {
  controller_->ShowContainedItem(node_id);
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

WindowDefinition OpenedView::Save() {
  window_def_.Clear();
  window_def_.title = user_title_;
  controller_->Save(window_def_);

  SetModified(false);

  return window_def_;
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

void OpenedView::ShowPopupMenu(aui::MenuModel* merge_menu,
                               unsigned resource_id,
                               const aui::Point& point,
                               bool right_click) {
  if (resource_id == 0)
    resource_id = window_info().menu;

  if (resource_id == 0)
    resource_id = IDR_ITEM_POPUP;

  popup_menu_handler_(merge_menu, resource_id, point, right_click);
}

promise<WindowDefinition> OpenedView::GetOpenWindowDefinition(
    const WindowInfo* window_info) const {
  if (auto open_context = controller_->GetOpenContext();
      open_context.has_value()) {
    return MakeWindowDefinition(window_info, *open_context);
  }

  const SelectionModel* selection = controller_->GetSelectionModel();
  if (!selection) {
    return make_resolved_promise(WindowDefinition{*window_info});
  }

  if (selection->multiple()) {
    const std::u16string& title = selection->GetTitle();
    const NodeIdSet& node_ids = selection->GetMultipleNodeIds();
    return make_resolved_promise(MakeWindowDefinition(
        window_info, {node_ids.begin(), node_ids.end()}, title));
  }

  if (!selection->node() && selection->timed_data().connected()) {
    const std::string& formula = selection->timed_data().formula();
    return make_resolved_promise(MakeWindowDefinition(window_info, formula));
  }

  return MakeWindowDefinition(window_info, selection->node(), true);
}
