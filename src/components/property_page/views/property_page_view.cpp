#include "client/components/property_page/views/property_page_view.h"

#include "client/controller_factory.h"
#include "client/client_utils.h"
#include "client/components/property_page/views/record_editors.h"
#include "client/window_definition.h"
#include "common/scada_node_ids.h"
#include "ui/views/widget/widget.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"
#include "ui/views/layout/fill_layout.h"

REGISTER_CONTROLLER(PropertyPageView, ID_PROPERTY_VIEW);

PropertyPageView::PropertyPageView(const ControllerContext& context)
    : Controller(context) {
  node_service_.AddObserver(*this);
}

PropertyPageView::~PropertyPageView() {
  node_service_.RemoveObserver(*this);
}

views::View* PropertyPageView::Init(const WindowDefinition& definition) {
  for (auto& item : definition.items) {
    if (!item.name_is("Item"))
      continue;
      
    auto path = item.GetString("path");
    node_id_ = scada::NodeId::FromString(path);
    if (!node_id_.is_null())
      break;
  }

  view_ = new views::View;
  view_->SetLayoutManager(new views::FillLayout);

  auto& node_service = node_service_;
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  node_service.RequestNode(node_id_, [&node_service, weak_ptr](const scada::Status& status, const NodeRef& node) {
    if (!status)
      return;
    BrowseParent(node_service, node.id(), OpcUaId_HierarchicalReferences, [weak_ptr, node](const scada::Status& status, const NodeRef& parent) {
      // |parent| can be null.
      if (auto* ptr = weak_ptr.get())
        ptr->SetNode(node, parent);
    });
  });

  return view_;
}

void PropertyPageView::Save(WindowDefinition& definition) {
  if (!node_id_.is_null()) {
    WindowItem& item = definition.AddItem("Item");
    item.SetString("path", node_id_.ToString());
  }
}

void PropertyPageViewContents::NativeControlDestroyed() {
  editor_ = nullptr;
  __super::NativeControlDestroyed();
}

bool PropertyPageViewContents::DispatchNativeEvent(const base::NativeEvent& event) {
  return editor_ && editor_->IsDialogMessage(const_cast<MSG*>(&event));
}

void PropertyPageView::OnNodeDeleted(const scada::NodeId& node_id) {
  if (node_id_ == node_id)
    controller_delegate_.Close();
}

void PropertyPageViewContents::CreateNativeControl(HWND parent_handle) {
  assert(editor_);

  editor_->contents_ = this;
  editor_->Create(GetWidget()->GetNativeView());
  editor_->UpdateData();
  editor_->ShowWindow(SW_SHOW);

  NativeControlCreated(*editor_.release());
}

PropertyPageViewContents::PropertyPageViewContents(std::unique_ptr<RecordEditor> editor)
    : editor_{std::move(editor)} {
}

void PropertyPageView::SetNode(const NodeRef& node, const NodeRef& parent) {
  RecordEditorContext context{
      node_service_,
      task_manager_,
      node_management_service_,
  };

  std::unique_ptr<RecordEditor> editor;
  if (IsInstanceOf(node, id::DataGroupType))
    editor = std::make_unique<record_editors::GroupEditor>(std::move(context));
  else if (IsInstanceOf(node, id::DiscreteItemType))
    editor = std::make_unique<record_editors::TsEditor>(std::move(context));
  else if (IsInstanceOf(node, id::AnalogItemType))
    editor = std::make_unique<record_editors::TitEditor>(std::move(context));
  else if (IsInstanceOf(node, id::TsFormatType))
    editor = std::make_unique<record_editors::TsFormatEditor>(std::move(context));
  else if (IsInstanceOf(node, id::Iec60870LinkType)) {
    const auto protocol = static_cast<cfg::IecProtocol>(
        node[id::Iec60870LinkType_Protocol].value().get_or(static_cast<int>(cfg::IecProtocol::IEC104)));
    switch (protocol) {
      case cfg::IecProtocol::IEC101:
        editor = std::make_unique<record_editors::IecLinkEditor>(IDD_IEC60870_LINK101, std::move(context));
        break;
      case cfg::IecProtocol::IEC104:
        editor = std::make_unique<record_editors::IecLinkEditor>(IDD_IEC60870_LINK104, std::move(context));
        break;
    }
  } else if (IsInstanceOf(node, id::Iec60870DeviceType)) {
    const auto protocol = static_cast<cfg::IecProtocol>(
        parent[id::Iec60870LinkType_Protocol].value().get_or(static_cast<int>(cfg::IecProtocol::IEC104)));
    const bool iec104 = protocol == cfg::IecProtocol::IEC104;
    editor = std::make_unique<record_editors::IecDeviceEditor>(std::move(context), iec104);
  } else if (IsInstanceOf(node, id::SimulationSignalType))
    editor = std::make_unique<record_editors::SimulationItemEditor>(std::move(context));
  else if (IsInstanceOf(node, id::ModbusLinkType))
    editor = std::make_unique<record_editors::ModbusLinkEditor>(std::move(context));
  else if (IsInstanceOf(node, id::ModbusDeviceType))
    editor = std::make_unique<record_editors::ModbusDeviceEditor>(std::move(context));
  else if (IsInstanceOf(node, id::HistoricalDatabaseType))
    editor = std::make_unique<record_editors::HistoricalDBEditor>(std::move(context));
  else
    editor = std::make_unique<record_editors::NamedRecordEditor>(IDD_BASE_EDITOR, std::move(context));

  if (!editor)
    return;

  editor->Init(node);

  contents_ = new PropertyPageViewContents(std::move(editor));
  view_->AddChildView(contents_);
  view_->Layout();
}
