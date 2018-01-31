#include "components/main/opened_view.h"

#include "client_utils.h"
#include "commands/add_multiple_items_dialog.h"
#include "commands/add_service_items_dialog.h"
#include "common/node_ref_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common/static_types.h"
#include "common_resources.h"
#include "components/main/action.h"
#include "components/main/client_commands.h"
#include "components/main/main_window.h"
#include "components/main/main_window_util.h"
#include "components/main/selection_commands.h"
#include "controller.h"
#include "controller_factory.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "net/transport_string.h"
#include "services/task_manager.h"
#include "window_definition.h"
#include "window_info.h"

#if defined(UI_QT)
#include "components/main/qt/main_window_qt.h"

#include <QMenu>
#endif

namespace {

NodeRef GetCreateParentNode(const NodeRef& suggested_parent,
                            const NodeRef& root,
                            const NodeRef& new_component_type) {
  for (auto& parent : {suggested_parent, root}) {
    if (!parent)
      continue;

    for (auto& target : parent.type_definition().targets(id::Creates)) {
      if (IsSubtypeOf(new_component_type, target.id()))
        return parent;
    }
  }

  return nullptr;
}

}  // namespace

OpenedView::OpenedView(const OpenedViewContext& context)
    : OpenedViewContext(context),
      modified_(false),
      working_(false),
      view_(nullptr),
      window_info_(*context.definition_.window_info()),
      window_id_(context.definition_.id),
      weak_factory_(this) {
  controller_ =
      CreateController(window_info_.command_id, ControllerContext{
                                                    *this,
                                                    timed_data_service_,
                                                    node_service_,
                                                    portfolio_manager_,
                                                    task_manager_,
                                                    profile_,
                                                    local_events_,
                                                    event_manager_,
                                                    file_cache_,
                                                    node_management_service_,
                                                    history_service_,
                                                    favourites_,
                                                    dialog_service_,
                                                    session_service_,
                                                    monitored_item_service_,
                                                    view_service_,
                                                });
  if (!controller_)
    throw std::exception("View type not found");

  title_ = user_title_ = context.definition_.title;

  selection_commands_ =
      std::make_unique<SelectionCommands>(SelectionCommandsContext{
          main_window_,
          timed_data_service_,
          task_manager_,
          profile_,
          local_events_,
          event_manager_,
          session_service_,
          node_management_service_,
          method_service_,
          file_cache_,
          dialog_service_,
          find_opened_view_,
          node_service_,
      });
  selection_commands_->set_selection(&controller_->selection());

  auto* view = controller_->Init(context.definition_);
  if (!view)
    throw std::runtime_error("Can't create widget");
  view_ = view;

#if defined(UI_QT)
  view_->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(view_, &QWidget::customContextMenuRequested,
                   [this](const QPoint& pos) {
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
      ShowAddServiceItemsDialog(view_service_, node_service_,
                                controller_->selection().node(), task_manager_);
      return;
    case ID_ADD_MULTIPLE_ITEMS:
      ShowAddMultipleItemsDialog(view_service_, node_service_,
                                 controller_->selection().node(),
                                 task_manager_);
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

ContentsModel* OpenedView::GetContentsModel() {
  return controller_->GetContentsModel();
}

bool OpenedView::CanCreateRecord(const scada::NodeId& type_node_id) const {
  if (!session_service_.IsAdministrator())
    return false;

  auto type_definition = node_service_.GetNode(type_node_id);
  if (!type_definition.fetched())
    return false;

  return GetCreateParentNode(controller_->selection().node(),
                             controller_->GetRootNode(),
                             type_definition) != nullptr;
}

void OpenedView::CreateRecord(const scada::NodeId& type_node_id, int tag) {
  if (!session_service_.IsAdministrator())
    return;

  auto type_definition = node_service_.GetNode(type_node_id);
  if (!type_definition.fetched())
    return;

  auto parent_node =
      GetCreateParentNode(controller_->selection().node(),
                          controller_->GetRootNode(), type_definition);
  if (!parent_node)
    return;

  scada::NodeAttributes attributes;
  scada::NodeProperties properties;

  bool is104 = tag != 0;

  // name
  auto browse_name = type_definition.browse_name();
  if (type_node_id == id::Iec60870LinkType) {
    browse_name = is104 ? scada::QualifiedName{"Направление МЭК-60870-104", 0}
                        : scada::QualifiedName{"Направление МЭК-60870-101", 0};
  }
  attributes.set_browse_name(browse_name);

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

  auto weak_ptr = weak_factory_.GetWeakPtr();
  task_manager_.PostInsertTask(
      scada::NodeId(), parent_node.id(), type_node_id, std::move(attributes),
      std::move(properties),
      [weak_ptr, browse_name](const scada::Status& status,
                              const scada::NodeId& node_id) {
        if (auto ptr = weak_ptr.get())
          ptr->OnCreateRecordComplete(browse_name, status, node_id);
      });
}

void OpenedView::OnCreateRecordComplete(const scada::QualifiedName& browse_name,
                                        const scada::Status& status,
                                        const scada::NodeId& node_id) {
  base::string16 title = base::StringPrintf(
      L"Создание \"%ls\"", base::SysNativeMBToWide(browse_name.name()).c_str());
  ReportRequestResult(title, status, local_events_, profile_);

  if (!status)
    return;

  auto weak_ptr = weak_factory_.GetWeakPtr();
  node_service_.GetNode(node_id).Fetch([weak_ptr](const NodeRef& node) {
    if (!node.status())
      return;
    auto* ptr = weak_ptr.get();
    if (!ptr)
      return;
    ptr->controller_->OnViewNodeCreated(node);
    client::OpenRecordEditor(ptr->main_window_, node);
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

void OpenedView::Save(WindowDefinition& definition) {
  assert(&window_info() == definition.window_info());

  definition.Clear();
  definition.title = user_title_;
  controller_->Save(definition);

  SetModified(false);
}

void OpenedView::OpenView(const WindowDefinition& def) {
  assert(main_window_);
  main_window_->OpenView(def, true);
}

void OpenedView::ExecuteDefaultItemCommand(const NodeRef& node) {
  ::ExecuteDefaultItemCommand(node_service_, node, main_window_);
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
