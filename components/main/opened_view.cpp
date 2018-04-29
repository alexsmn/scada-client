#include "opened_view.h"

#include "base/threading/thread_task_runner_handle.h"
#include "client_utils.h"
#include "commands/add_multiple_items_dialog.h"
#include "commands/add_service_items_dialog.h"
#include "commands/time_range_dialog.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "common/static_types.h"
#include "common_resources.h"
#include "components/main/actions.h"
#include "components/main/main_window_util.h"
#include "components/main/selection_commands.h"
#include "controller.h"
#include "controller_factory.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "main_window.h"
#include "net/transport_string.h"
#include "services/task_manager.h"
#include "time_model.h"
#include "window_definition.h"
#include "window_info.h"

#if defined(UI_QT)
#include "components/main/qt/main_window_qt.h"

#include <QMenu>
#endif

namespace {

NodeRef GetCreateParentNode(const NodeRef& suggested_parent,
                            const NodeRef& root,
                            const NodeRef& component_type) {
  if (!component_type)
    return nullptr;

  for (const auto& parent : {suggested_parent, root}) {
    if (parent && CanCreate(parent, component_type))
      return parent;
  }

  return nullptr;
}

}  // namespace

OpenedView::OpenedView(OpenedViewContext&& context)
    : OpenedViewContext{std::move(context)} {
  controller_ = CreateController(
      window_def_.window_info().command_id,
      ControllerContext{*this, alias_resolver_, task_manager_, session_service_,
                        event_manager_, history_service_,
                        monitored_item_service_, timed_data_service_,
                        node_service_, portfolio_manager_, local_events_,
                        favourites_, file_cache_, profile_, dialog_service_});
  if (!controller_)
    throw std::runtime_error{"View type not found"};

  title_ = user_title_ = window_def_.title;

  selection_commands_ =
      std::make_unique<SelectionCommands>(SelectionCommandsContext{
          *main_window_, task_manager_, method_service_, session_service_,
          node_management_service_, event_manager_, timed_data_service_,
          local_events_, file_cache_, profile_, dialog_service_,
          main_window_manager_});
  selection_commands_->set_selection(&controller_->selection());

  auto* view = controller_->Init(window_def_);
  if (!view)
    throw std::runtime_error{"Can't create widget"};
  view_ = view;

#if defined(UI_QT)
  view_->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(view_, &QWidget::customContextMenuRequested,
                   [this](const QPoint& pos) {
                     // TODO: Avoid the cast.
                     static_cast<MainWindowQt*>(main_window_)
                         ->context_menu()
                         .exec(view_->mapToGlobal(pos));
                   });

#elif defined(UI_VIEWS)
  view_->set_parent_owned(false);
  view_->set_drop_controller(this);
  view_->set_context_menu_controller(this);
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

void OpenedView::SetUserTitle(const base::StringPiece16& title) {
  assert(!window_info().is_pane());

  if (user_title_ != title) {
    user_title_ = title.as_string();
    UpdateTitle();
  }
}

base::string16 OpenedView::GetWindowTitle() const {
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
  base::string16 title = GetWindowTitle();
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

CommandHandler* OpenedView::GetCommandHandler(unsigned command_id) {
  if (auto* handler = controller_->GetCommandHandler(command_id))
    return handler;

  switch (command_id) {
    case ID_VIEW_CLOSE:
    case ID_PRINT:
      return this;

    case ID_NEW_SERVICE_ITEMS:
    case ID_ADD_MULTIPLE_ITEMS:
      return CanCreateRecord(id::DiscreteItemType) ? this : NULL;

    case ID_NEW_IEC60870_LINK101:
    case ID_NEW_IEC60870_LINK104:
      return CanCreateRecord(id::Iec60870LinkType) ? this : NULL;

    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
    case ID_TIME_RANGE_CUSTOM:
      return controller_->GetTimeModel() != nullptr ? this : NULL;
  }

  auto node_id = GetNewCommandTypeId(command_id);
  if (node_id != scada::NodeId())
    return CanCreateRecord(node_id) ? this : NULL;

  return selection_commands_->GetCommandHandler(command_id);
}

void OpenedView::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_VIEW_CLOSE:
      Close();
      return;
    case ID_PRINT:
      Print();
      return;
    case ID_NEW_IEC60870_LINK101:
      CreateRecord(id::Iec60870LinkType, 0);
      return;
    case ID_NEW_IEC60870_LINK104:
      CreateRecord(id::Iec60870LinkType, 1);
      return;

    case ID_NEW_SERVICE_ITEMS:
      ShowAddServiceItemsDialog(node_service_, task_manager_,
                                controller_->selection().node().id());
      return;
    case ID_ADD_MULTIPLE_ITEMS:
      ShowAddMultipleItemsDialog(node_service_, task_manager_,
                                 controller_->selection().node().id());
      return;

    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
      if (auto* model = controller_->GetTimeModel())
        model->SetTimeRange(TimeRange{command_id});
      return;

    case ID_TIME_RANGE_CUSTOM:
      if (auto* model = controller_->GetTimeModel()) {
        auto range = model->GetTimeRange();
        bool time_required = model->IsTimeRequired();
        if (ShowTimeRangeDialog(profile_, range, time_required))
          model->SetTimeRange(range);
      }
      return;
  }

  auto node_id = GetNewCommandTypeId(command_id);
  if (node_id != scada::NodeId()) {
    CreateRecord(node_id, 0);
    return;
  }

  // Command is supported but not handled.
  assert(false);
}

bool OpenedView::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
    case ID_TIME_RANGE_CUSTOM:
      if (auto* model = controller_->GetTimeModel())
        return model->GetTimeRange().command_id == command_id;
      return false;

    default:
      return false;
  }
}

ContentsModel* OpenedView::GetContentsModel() {
  return controller_->GetContentsModel();
}

bool OpenedView::CanCreateRecord(const scada::NodeId& type_node_id) const {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return false;

  return GetCreateParentNode(controller_->selection().node(),
                             controller_->GetRootNode(),
                             node_service_.GetNode(type_node_id)) != nullptr;
}

void OpenedView::CreateRecord(const scada::NodeId& type_node_id, int tag) {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  auto node_type = node_service_.GetNode(type_node_id);
  if (!node_type)
    return;

  auto parent_node = GetCreateParentNode(controller_->selection().node(),
                                         controller_->GetRootNode(), node_type);
  if (!parent_node)
    return;

  scada::NodeAttributes attributes;
  scada::NodeProperties properties;

  bool is104 = tag != 0;

  // name
  attributes.display_name = node_type.display_name();
  if (type_node_id == id::Iec60870LinkType) {
    attributes.display_name =
        is104 ? L"Направление МЭК-60870-104" : L"Направление МЭК-60870-101";
  }

  // IEC link specific.
  if (type_node_id == id::Iec60870LinkType) {
    // type
    auto protocol =
        is104 ? cfg::Iec60870Protocol::IEC104 : cfg::Iec60870Protocol::IEC101;
    properties.emplace_back(id::Iec60870LinkType_Protocol,
                            static_cast<int>(protocol));

    net::TransportString ts;
    if (is104) {
      ts.SetProtocol(net::TransportString::TCP);
      ts.SetParam(net::TransportString::kParamHost, "localhost");
      ts.SetParam(net::TransportString::kParamPort, 2404);
    } else {
      ts.SetProtocol(net::TransportString::SERIAL);
      ts.SetParam(net::TransportString::kParamName, "COM1");
    }
    ts.SetParam(net::TransportString::kParamActive);
    properties.emplace_back(id::LinkType_Transport, ts.ToString());
  }

  auto dispay_name = attributes.display_name;
  auto weak_ptr = weak_factory_.GetWeakPtr();
  task_manager_.PostInsertTask(
      scada::NodeId(), parent_node.id(), type_node_id, std::move(attributes),
      std::move(properties),
      [weak_ptr, dispay_name](const scada::Status& status,
                              const scada::NodeId& node_id) {
        if (auto ptr = weak_ptr.get())
          ptr->OnCreateRecordComplete(dispay_name, status, node_id);
      });
}

void OpenedView::OnCreateRecordComplete(
    const scada::LocalizedText& display_name,
    const scada::Status& status,
    const scada::NodeId& node_id) {
  base::string16 title =
      base::StringPrintf(L"Создание \"%ls\"", display_name.c_str());
  ReportRequestResult(title, status, local_events_, profile_);

  if (!status)
    return;

  auto weak_ptr = weak_factory_.GetWeakPtr();
  auto node = node_service_.GetNode(node_id);
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [weak_ptr, node](const NodeRef& node) {
               if (auto* ptr = weak_ptr.get()) {
                 ptr->controller_->OnViewNodeCreated(node);
                 auto def = MakeWindowDefinition(node, ID_PROPERTY_VIEW, false);
                 ::OpenView(&ptr->main_window(), def, true);
               }
             });
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

void OpenedView::SetTitle(const base::StringPiece16& title) {
  assert(!window_info().is_pane());
  if (title_ != title) {
    title_ = title.as_string();
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
  ::OpenView(main_window_, def, true);
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
