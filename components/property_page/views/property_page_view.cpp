#include "components/property_page/views/property_page_view.h"

#include "client_utils.h"
#include "model/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "model/scada_node_ids.h"
#include "components/property_page/views/record_editors.h"
#include "controller_factory.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "window_definition.h"

const WindowInfo kWindowInfo = {ID_PROPERTY_VIEW, "RecEditor", L"Параметры",
                                WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN};

REGISTER_CONTROLLER(PropertyPageView, kWindowInfo);

PropertyPageView::PropertyPageView(const ControllerContext& context)
    : Controller(context) {}

PropertyPageView::~PropertyPageView() {
  if (node_)
    node_.Unsubscribe(*this);
}

views::View* PropertyPageView::Init(const WindowDefinition& definition) {
  scada::NodeId node_id;
  for (auto& item : definition.items) {
    if (!item.name_is("Item"))
      continue;

    auto path = item.GetString("path");
    node_id = NodeIdFromScadaString(path);
    if (!node_id.is_null())
      break;
  }

  view_ = new views::View;
  view_->SetLayoutManager(new views::FillLayout);

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  node_service_.GetNode(node_id).Fetch(NodeFetchStatus::NodeOnly(),
                                       [weak_ptr](const NodeRef& node) {
                                         // |parent| can be null.
                                         if (auto* ptr = weak_ptr.get())
                                           ptr->SetNode(node);
                                       });

  return view_;
}

void PropertyPageView::Save(WindowDefinition& definition) {
  if (node_) {
    WindowItem& item = definition.AddItem("Item");
    item.SetString("path", NodeIdToScadaString(node_.node_id()));
  }
}

void PropertyPageViewContents::NativeControlDestroyed() {
  editor_ = nullptr;
  __super::NativeControlDestroyed();
}

bool PropertyPageViewContents::DispatchNativeEvent(
    const base::NativeEvent& event) {
  return editor_ && editor_->IsDialogMessage(const_cast<MSG*>(&event));
}

void PropertyPageView::OnModelChanged(const scada::ModelChangeEvent& event) {
  if ((event.verb & scada::ModelChangeEvent::NodeDeleted) &&
      (node_.node_id() == event.node_id))
    controller_delegate_.Close();
}

void PropertyPageViewContents::CreateNativeControl(HWND parent_handle) {
  assert(editor_);

  editor_->destroy_handler = [this] { NativeControlDestroyed(); };
  editor_->Create(GetWidget()->GetNativeView());
  editor_->UpdateData();
  editor_->ShowWindow(SW_SHOW);

  NativeControlCreated(*editor_.release());
}

PropertyPageViewContents::PropertyPageViewContents(
    std::unique_ptr<RecordEditor> editor)
    : editor_{std::move(editor)} {}

void PropertyPageView::SetNode(const NodeRef& node) {
  RecordEditorContext context{dialog_service_, task_manager_, node_service_};

  std::unique_ptr<RecordEditor> editor;
  if (IsInstanceOf(node, id::DataGroupType))
    editor = std::make_unique<record_editors::GroupEditor>(std::move(context));
  else if (IsInstanceOf(node, id::DiscreteItemType))
    editor = std::make_unique<record_editors::TsEditor>(std::move(context));
  else if (IsInstanceOf(node, id::AnalogItemType))
    editor = std::make_unique<record_editors::TitEditor>(std::move(context));
  else if (IsInstanceOf(node, id::TsFormatType))
    editor =
        std::make_unique<record_editors::TsFormatEditor>(std::move(context));
  else if (IsInstanceOf(node, id::Iec60870LinkType)) {
    const auto protocol = static_cast<cfg::Iec60870Protocol>(
        node[id::Iec60870LinkType_Protocol].value().get_or(
            static_cast<int>(cfg::Iec60870Protocol::IEC104)));
    switch (protocol) {
      case cfg::Iec60870Protocol::IEC101:
        editor = std::make_unique<record_editors::Iec60870LinkEditor>(
            IDD_IEC60870_LINK101, std::move(context));
        break;
      case cfg::Iec60870Protocol::IEC104:
        editor = std::make_unique<record_editors::Iec60870LinkEditor>(
            IDD_IEC60870_LINK104, std::move(context));
        break;
    }
  } else if (IsInstanceOf(node, id::Iec60870DeviceType))
    editor = std::make_unique<record_editors::Iec60870DeviceEditor>(
        std::move(context));
  else if (IsInstanceOf(node, id::Iec61850DeviceType))
    editor = std::make_unique<record_editors::Iec61850DeviceEditor>(
        std::move(context));
  else if (IsInstanceOf(node, id::Iec61850RcbType))
    editor =
        std::make_unique<record_editors::Iec61850RCBEditor>(std::move(context));
  else if (IsInstanceOf(node, id::SimulationSignalType))
    editor = std::make_unique<record_editors::SimulationItemEditor>(
        std::move(context));
  else if (IsInstanceOf(node, id::ModbusLinkType))
    editor =
        std::make_unique<record_editors::ModbusLinkEditor>(std::move(context));
  else if (IsInstanceOf(node, id::ModbusDeviceType))
    editor = std::make_unique<record_editors::ModbusDeviceEditor>(
        std::move(context));
  else if (IsInstanceOf(node, id::HistoricalDatabaseType))
    editor = std::make_unique<record_editors::HistoricalDBEditor>(
        std::move(context));
  else
    editor = std::make_unique<record_editors::NamedRecordEditor>(
        IDD_BASE_EDITOR, std::move(context));

  if (!editor)
    return;

  node_ = node;
  node_.Subscribe(*this);

  editor->Init(node);

  contents_ = new PropertyPageViewContents(std::move(editor));
  view_->AddChildView(contents_);
  view_->Layout();
}
