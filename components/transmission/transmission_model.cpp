#include "components/transmission/transmission_model.h"

#include "base/format.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "contents_observer.h"
#include "core/node_management_service.h"
#include "services/task_manager.h"

TransmissionModel::TransmissionModel(NodeService& node_service,
                                     TaskManager& task_manager)
    : FixedRowModel(*static_cast<FixedRowModel::Delegate*>(this)),
      node_service_{node_service},
      task_manager_{task_manager} {
  // Must subscribe to root, since references will be deleted.
  node_service_.Subscribe(*this);
}

TransmissionModel::~TransmissionModel() {
  node_service_.Unsubscribe(*this);
}

void TransmissionModel::SetDevice(NodeRef device) {
  device_ = device;
  Update();
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
      auto source = row.transmission.target(kIecTransmitSourceRefTypeId);
      cell.text = source ? source.display_name() : base::string16();
      break;
    }

    case 1:
      auto device_item_address =
          row.transmission[kIecTransmitTargetInfoAddressPropTypeId]
              .value()
              .get_or(0);
      cell.text = WideFormat(device_item_address);
      break;
  }
}

void TransmissionModel::Update() {
  rows_.clear();

  if (device_) {
    for (auto reference :
         device_.inverse_references(kIecTransmitTargetDeviceRefTypeId)) {
      assert(IsInstanceOf(reference.target, id::TransmissionItemType));
      Update(reference.target);
    }
  }

  GridModel::NotifyModelChanged();
}

void TransmissionModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted)
    Delete(event.node_id);
  else if (auto node = node_service_.GetNode(event.node_id))
    Update(node);
}

void TransmissionModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  if (auto node = node_service_.GetNode(node_id))
    Update(node);
}

void TransmissionModel::Update(NodeRef transmission) {
  assert(IsInstanceOf(transmission, id::TransmissionItemType));

  auto device = transmission.target(kIecTransmitTargetDeviceRefTypeId);
  if (device != device_) {
    Delete(transmission.id());
    return;
  }

  auto source_id = transmission.target(kIecTransmitSourceRefTypeId).id();

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

void TransmissionModel::Delete(const scada::NodeId& transmission_id) {
  int i = FindRow(transmission_id);
  if (i == -1)
    return;

  auto transmission = rows_[i].transmission;
  auto source = transmission.target(kIecTransmitSourceRefTypeId);
  if (source && contents_observer())
    contents_observer()->OnContainedItemChanged(source.id(), false);

  rows_.erase(rows_.begin() + i);
  GridModel::NotifyModelChanged();
}

int TransmissionModel::FindRow(const scada::NodeId& transmission_id) const {
  for (int i = 0; i < (int)rows_.size(); i++)
    if (rows_[i].transmission.id() == transmission_id)
      return i;
  return -1;
}

int TransmissionModel::FindSource(const scada::NodeId& source_id) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    auto& row = rows_[i];
    auto source = row.transmission.target(kIecTransmitSourceRefTypeId);
    if (source && source.id() == source_id)
      return i;
  }
  return -1;
}

NodeIdSet TransmissionModel::GetContainedItems() const {
  NodeIdSet items;
  for (auto& row : rows()) {
    if (auto source = row.transmission.target(kIecTransmitSourceRefTypeId))
      items.emplace(source.id());
  }
  return items;
}

void TransmissionModel::AddContainedItem(const scada::NodeId& node_id,
                                         unsigned flags) {
  if (!device())
    return;

  auto device_id = device().id();
  task_manager_.PostInsertTask(
      scada::NodeId(), id::TransmissionItems, id::TransmissionItemType, {}, {},
      [node_id, device_id, &task_manager = task_manager_](
          const scada::Status& status, const scada::NodeId& transmission_id) {
        if (!status)
          return;

        task_manager.PostAddReference(kIecTransmitSourceRefTypeId,
                                      transmission_id, node_id);
        task_manager.PostAddReference(kIecTransmitTargetDeviceRefTypeId,
                                      transmission_id, device_id);
      });
}

void TransmissionModel::RemoveContainedItem(const scada::NodeId& node_id) {
  auto source = node_service_.GetNode(node_id);
  if (!source)
    return;

  for (auto& row : rows()) {
    if (row.transmission.target(kIecTransmitSourceRefTypeId) == source)
      task_manager_.PostDeleteTask(row.transmission.id());
  }
}

// static
TransmissionModel::Row TransmissionModel::MakeRow(NodeRef transmission) {
  return Row{transmission};
}
