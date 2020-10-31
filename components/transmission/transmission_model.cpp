#include "components/transmission/transmission_model.h"

#include "base/format.h"
#include "base/strings/sys_string_conversions.h"
#include "contents_observer.h"
#include "core/event.h"
#include "core/node_management_service.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
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
  device_ = std::move(device);
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
      auto source = row.transmission.target(devices::id::HasTransmissionSource);
      cell.text = source ? source.display_name() : base::string16();
      break;
    }

    case 1:
      auto device_item_address =
          row.transmission[devices::id::TransmissionItemType_SourceAddress]
              .value()
              .get_or(0);
      cell.text = WideFormat(device_item_address);
      break;
  }
}

bool TransmissionModel::IsEditable(int row, int column) {
  return column != 0;
}

bool TransmissionModel::SetCellText(int row,
                                    int column,
                                    const base::string16& text) {
  assert(row >= 0 && row < GetRowCount());

  /*	GridRange range = selection();
    for (int row = range.top; row <= range.bottom; row++)
      for (int col = range.left; col <= range.right; col++)
        WriteCell(row, col, text);*/

  int value;
  if (!Parse(text, value))
    return false;

  auto& row_item = this->row(row);
  scada::NodeProperties properties;
  properties.emplace_back(devices::id::TransmissionItemType_SourceAddress,
                          static_cast<int>(value));
  task_manager_.PostUpdateTask(row_item.transmission.node_id(), {}, properties);

  return true;
}

void TransmissionModel::Update() {
  rows_.clear();

  if (device_) {
    for (auto reference :
         device_.inverse_references(devices::id::HasTransmissionTarget)) {
      assert(IsInstanceOf(reference.target, devices::id::TransmissionItemType));
      Update(reference.target);
    }
  }

  GridModel::NotifyModelChanged();
}

void TransmissionModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    Delete(event.node_id);
  } else {
    auto node = node_service_.GetNode(event.node_id);
    if (IsInstanceOf(node, devices::id::TransmissionItemType))
      Update(node);
  }
}

void TransmissionModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto node = node_service_.GetNode(node_id);
  if (IsInstanceOf(node, devices::id::TransmissionItemType))
    Update(node);
}

void TransmissionModel::Update(NodeRef transmission) {
  assert(IsInstanceOf(transmission, devices::id::TransmissionItemType));

  auto device = transmission.target(devices::id::HasTransmissionTarget);
  if (device != device_) {
    Delete(transmission.node_id());
    return;
  }

  auto source_id =
      transmission.target(devices::id::HasTransmissionSource).node_id();

  int i = FindRow(transmission.node_id());
  if (i == -1) {
    rows_.push_back(Row{transmission, source_id});
    GridModel::NotifyRowsAdded(rows_.size() - 1, 1);
    if (!source_id.is_null())
      NotifyContainedItemChanged(source_id, true);
    return;
  }

  GridModel::NotifyRowsChanged(i, 1);

  auto old_source_id = rows_[i].source_id;
  if (old_source_id != source_id) {
    rows_[i].source_id = source_id;
    if (!old_source_id.is_null() && FindSource(old_source_id) == -1)
      NotifyContainedItemChanged(old_source_id, false);
    if (!source_id.is_null())
      NotifyContainedItemChanged(source_id, true);
  }
}

void TransmissionModel::Delete(const scada::NodeId& transmission_id) {
  int i = FindRow(transmission_id);
  if (i == -1)
    return;

  auto& row = rows_[i];
  if (!row.source_id.is_null())
    NotifyContainedItemChanged(row.source_id, false);

  rows_.erase(rows_.begin() + i);
  GridModel::NotifyModelChanged();
}

int TransmissionModel::FindRow(const scada::NodeId& transmission_id) const {
  for (int i = 0; i < (int)rows_.size(); i++)
    if (rows_[i].transmission.node_id() == transmission_id)
      return i;
  return -1;
}

int TransmissionModel::FindSource(const scada::NodeId& source_id) const {
  for (int i = 0; i < (int)rows_.size(); i++) {
    auto& row = rows_[i];
    if (row.source_id == source_id)
      return i;
  }
  return -1;
}

NodeIdSet TransmissionModel::GetContainedItems() const {
  NodeIdSet items;
  for (auto& row : rows())
    items.emplace(row.source_id);
  return items;
}

void TransmissionModel::AddContainedItem(const scada::NodeId& node_id,
                                         unsigned flags) {
  if (!device())
    return;

  auto device_id = device().node_id();
  task_manager_.PostInsertTask(
      scada::NodeId(), devices::id::TransmissionItems,
      devices::id::TransmissionItemType, {}, {},
      [node_id, device_id, &task_manager = task_manager_](
          const scada::Status& status, const scada::NodeId& transmission_id) {
        if (!status)
          return;

        task_manager.PostAddReference(devices::id::HasTransmissionSource,
                                      transmission_id, node_id);
        task_manager.PostAddReference(devices::id::HasTransmissionTarget,
                                      transmission_id, device_id);
      });
}

void TransmissionModel::RemoveContainedItem(const scada::NodeId& node_id) {
  std::vector<scada::NodeId> transmission_ids;
  for (auto& row : rows()) {
    if (row.source_id == node_id)
      transmission_ids.emplace_back(row.transmission.node_id());
  }

  for (const auto& transmission_id : transmission_ids)
    task_manager_.PostDeleteTask(transmission_id);
}

// static
TransmissionModel::Row TransmissionModel::MakeRow(NodeRef transmission) {
  return Row{transmission};
}
