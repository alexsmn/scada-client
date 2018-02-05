#include "components/transmission/transmission_model.h"

#include "base/format.h"
#include "base/strings/sys_string_conversions.h"
#include "common/browse_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "contents_observer.h"
#include "core/node_management_service.h"
#include "services/task_manager.h"
#include "translation.h"

TransmissionModel::TransmissionModel(
    scada::ViewService& view_service,
    NodeService& node_service,
    TaskManager& task_manager,
    scada::NodeManagementService& node_management_service)
    : FixedRowModel(*static_cast<FixedRowModel::Delegate*>(this)),
      view_service_{view_service},
      node_service_{node_service},
      task_manager_{task_manager},
      node_management_service_{node_management_service} {
  // Must subscribe to root, since references will be deleted.
  node_service_.Subscribe(*this);
}

TransmissionModel::~TransmissionModel() {
  node_service_.Unsubscribe(*this);
}

void TransmissionModel::SetDevice(NodeRef device) {
  device_ = std::move(device);
  Update();
}

void TransmissionModel::SetDeviceId(const scada::NodeId& device_id) {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  node_service_.GetNode(device_id).Fetch(NodeFetchStatus::NodeOnly(),
                                         [weak_ptr](const NodeRef& node) {
                                           if (auto* ptr = weak_ptr.get())
                                             ptr->SetDevice(std::move(node));
                                         });
}

int TransmissionModel::GetRowCount() {
  return static_cast<int>(rows_.size());
}

base::string16 TransmissionModel::GetRowTitle(int row) {
  return base::string16();
}

void TransmissionModel::GetCell(ui::GridCell& cell) {
  assert(cell.row >= 0 && cell.row <= (int)rows_.size());

  //	// Last cell.row is new cell.row.
  //	if (cell.row == rows_.size())
  //		return;

  auto& row = rows_[cell.row];

  switch (cell.column) {
    case 0: {
      auto source = row.transmission.target(id::HasTransmissionSource);
      cell.text = ToString16(source.display_name());
      break;
    }

    case 1:
      auto device_item_address =
          row.transmission[id::TransmissionItemType_SourceAddress]
              .value()
              .get_or(0);
      cell.text = WideFormat(device_item_address);
      break;
  }
}

void TransmissionModel::Update() {
  rows_.clear();

  if (device_) {
    auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
    BrowseNodes(view_service_, node_service_,
                {device_.id(), scada::BrowseDirection::Inverse,
                 id::HasTransmissionTarget, true},
                [weak_ptr](const scada::Status& status,
                           const std::vector<NodeRef>& transmission_items) {
                  if (!status)
                    return;
                  if (auto* ptr = weak_ptr.get()) {
                    for (auto& transmission_item : transmission_items)
                      ptr->UpdateItem(transmission_item);
                  }
                });
  }

  GridModel::NotifyModelChanged();
}

void TransmissionModel::OnModelChange(const ModelChangeEvent& event) {
  if (event.verb & ModelChangeEvent::NodeDeleted)
    DeleteRow(event.node_id);
  else if (auto node = node_service_.GetNode(event.node_id))
    UpdateItem(node);
}

void TransmissionModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  int index = FindRow(node_id);
  if (index != -1)
    UpdateItem(rows_[index].transmission);
}

void TransmissionModel::UpdateItem(const NodeRef& transmission) {
  assert(IsInstanceOf(transmission, id::TransmissionItemType));

  const auto& device = transmission.target(id::HasTransmissionTarget);
  if (device != device_) {
    DeleteRow(transmission.id());
    return;
  }

  const auto& source = transmission.target(id::HasTransmissionSource);
  const auto& source_id = source.id();

  int i = FindRow(transmission.id());
  if (i == -1) {
    rows_.push_back(Row{transmission, source_id});
    GridModel::NotifyRowsAdded(rows_.size() - 1, 1);
    if (contents_observer() && !source_id.is_null())
      contents_observer()->OnContainedItemChanged(source_id, true);
    return;
  }

  GridModel::NotifyRowsChanged(i, 1);

  auto old_source_id = rows_[i].source_id;
  if (old_source_id != source_id) {
    rows_[i].source_id = source_id;
    if (contents_observer() && !old_source_id.is_null() &&
        FindSource(old_source_id) == -1)
      contents_observer()->OnContainedItemChanged(old_source_id, false);
    if (contents_observer() && !source_id.is_null())
      contents_observer()->OnContainedItemChanged(source_id, true);
  }
}

void TransmissionModel::DeleteRow(const scada::NodeId& transmission_id) {
  int i = FindRow(transmission_id);
  if (i == -1)
    return;

  const auto& source = rows_[i].transmission.target(id::HasTransmissionSource);
  if (source && contents_observer())
    contents_observer()->OnContainedItemChanged(source.id(), false);

  rows_.erase(rows_.begin() + i);
  GridModel::NotifyModelChanged();
}

int TransmissionModel::FindRow(const scada::NodeId& transmission_id) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    if (rows_[i].transmission.id() == transmission_id)
      return i;
  }
  return -1;
}

int TransmissionModel::FindSource(const scada::NodeId& source_id) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    auto& row = rows_[i];
    auto source = row.transmission.target(id::HasTransmissionSource);
    if (source && source.id() == source_id)
      return i;
  }
  return -1;
}

NodeIdSet TransmissionModel::GetContainedItems() const {
  NodeIdSet items;
  for (auto& row : rows()) {
    if (auto source = row.transmission.target(id::HasTransmissionSource))
      items.emplace(source.id());
  }
  return items;
}

void TransmissionModel::AddContainedItem(const scada::NodeId& node_id,
                                         unsigned flags) {
  if (!device())
    return;

  auto& node_management_service = node_management_service_;
  const auto& device_id = device_.id();
  task_manager_.PostInsertTask(
      scada::NodeId(), id::TransmissionItems, id::TransmissionItemType, {}, {},
      [node_id, device_id, &node_management_service](
          const scada::Status& status, const scada::NodeId& transmission_id) {
        if (!status)
          return;
        node_management_service.AddReference(
            id::HasTransmissionSource, transmission_id, node_id,
            [](const scada::Status& status) {});
        node_management_service.AddReference(
            id::HasTransmissionTarget, transmission_id, device_id,
            [](const scada::Status& status) {});
      });
}

void TransmissionModel::RemoveContainedItem(const scada::NodeId& node_id) {
  int index = FindSource(node_id);
  if (index == -1)
    return;

  const auto& row = rows_[index];
  task_manager_.PostDeleteTask(row.transmission.id());
}
