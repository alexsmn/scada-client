#include "modules/transmission/transmission_model.h"

#include "base/awaitable.h"
#include "base/cancelation.h"
#include "base/check.h"
#include "base/format.h"
#include "base/range_util.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/event.h"
#include "scada/node_management_service.h"
#include "services/task_manager.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace {

scada::NodeId GetTransmissionItemTypeId(const NodeRef& device) {
  // The transmission item type is named by the device type's <TransmissionItem>
  // OptionalPlaceholder, attached via the HasTransmissionItem reference (the
  // standard-modelling replacement for the old Creates edge). Query that exact
  // reference type so it resolves without fetching the reference-type hierarchy
  // (matters for a remote node service).
  for (auto type = device.type_definition(); type; type = type.supertype()) {
    for (const auto& placeholder :
         type.targets(devices::id::HasTransmissionItem)) {
      NodeRef item_type = placeholder.type_definition();
      if (IsSubtypeOf(item_type, devices::id::TransmissionItemType))
        return item_type.node_id();
    }
  }
  return {};
}

}  // namespace

TransmissionModel::TransmissionModel(AnyExecutor executor,
                                     NodeService& node_service,
                                     TaskManager& task_manager)
    : FixedRowModel(*static_cast<FixedRowModel::Delegate*>(this)),
      executor_{std::move(executor)},
      node_service_{node_service},
      task_manager_{task_manager} {}

TransmissionModel::~TransmissionModel() = default;

void TransmissionModel::Init(NodeRef device) {
  device_ = std::move(device);

  connections_.push_back(node_service_.SubscribeModelChanged(
      [this](const scada::ModelChangeEvent& event) { OnModelChanged(event); }));
  connections_.push_back(node_service_.SubscribeNodeSemanticChanged(
      [this](const scada::NodeId& node_id) {
        OnNodeSemanticChanged(node_id);
      }));
  connections_.push_back(node_service_.SubscribeNodeFetched(
      [this](const NodeFetchedEvent& event) { OnNodeFetched(event); }));

  // Fetch everything the synchronous Refresh/GetCell paths read: the device's
  // children (the rows), each row's type chain (the transmission-item
  // IsInstanceOf filter and the source-address property declaration), each
  // row's children (the property instances holding the address values), and
  // each row's source node (the "Object" column display name). Node fetches
  // are per node, so none of this is implied by fetching the device alone.
  CoSpawn(executor_, cancelation_,
          [this,
           cancelation = cancelation_.ref()]() mutable -> Awaitable<void> {
            (void)co_await FetchChildrenStatus(device_);
            if (cancelation.canceled()) {
              co_return;
            }
            (void)co_await FetchTypeChainStatus(device_.type_definition());
            if (cancelation.canceled()) {
              co_return;
            }
            for (const auto& reference :
                 device_.references(scada::id::Organizes)) {
              NodeRef transmission = reference.target;
              (void)co_await FetchChildrenStatus(transmission);
              if (cancelation.canceled()) {
                co_return;
              }
              (void)co_await FetchTypeChainStatus(
                  transmission.type_definition());
              if (cancelation.canceled()) {
                co_return;
              }
              (void)co_await FetchNodeStatus(
                  transmission.target(devices::id::HasTransmissionSource));
              if (cancelation.canceled()) {
                co_return;
              }
            }
            Refresh();
          });

  if (device_.children_fetched())
    Refresh();
}

int TransmissionModel::GetRowCount() {
  return static_cast<int>(rows_.size());
}

std::u16string TransmissionModel::GetRowTitle(int row) {
  return std::u16string();
}

void TransmissionModel::GetCell(aui::GridCell& cell) {
  base::Check(cell.row >= 0 && cell.row <= (int)rows_.size());

  //	// Last cell.row is new cell.row.
  //	if (cell.row == rows_.size())
  //		return;

  auto& row = rows_[cell.row];

  switch (cell.column) {
    case 0: {
      auto source = row.transmission.target(devices::id::HasTransmissionSource);
      cell.text = source ? source.display_name() : std::u16string();
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
                                    const std::u16string& text) {
  base::Check(row >= 0 && row < GetRowCount());

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

void TransmissionModel::Refresh() {
  rows_ =
      device_.references(scada::id::Organizes) |
      boost::adaptors::filtered([](const NodeRef::Reference& reference) {
        return IsInstanceOf(reference.target,
                            devices::id::TransmissionItemType);
      }) |
      boost::adaptors::transformed([](const NodeRef::Reference& reference) {
        return reference.target;
      }) |
      boost::adaptors::transformed([](const NodeRef& transmission) {
        auto source_id =
            transmission.target(devices::id::HasTransmissionSource).node_id();
        return Row{transmission, source_id};
      }) |
      to_vector;

  GridModel::NotifyModelChanged();

  // Children included: the address property values live on the
  // transmission's property instance nodes.
  for (auto& row : rows_)
    row.transmission.StartFetch(NodeFetchStatus::NodeAndChildren);

  auto source_ids = rows_ | boost::adaptors::filtered([](const Row& row) {
                      return !row.source_id.is_null();
                    }) |
                    boost::adaptors::transformed(
                        [](const Row& row) { return row.source_id; }) |
                    to_set;
  NotifyContentsChanged(source_ids);
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

void TransmissionModel::OnNodeFetched(const NodeFetchedEvent& event) {
  if (event.node_id == device_.node_id() && device_.children_fetched())
    Refresh();
}

void TransmissionModel::Update(NodeRef transmission) {
  // Node type information comes from the server address space; skip nodes
  // that are not transmission items.
  if (!IsInstanceOf(transmission, devices::id::TransmissionItemType))
    return;

  transmission.StartFetch(NodeFetchStatus::NodeAndChildren);

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

  auto transmission_item_type_id = GetTransmissionItemTypeId(device_);

  task_manager_.PostInsertTask(
      {.type_definition_id = transmission_item_type_id,
       .parent_id = device_.node_id(),
       .references = {{devices::id::HasTransmissionSource, true, node_id}}});
}

void TransmissionModel::RemoveContainedItem(const scada::NodeId& node_id) {
  std::vector<scada::NodeId> transmission_ids;
  for (auto& row : rows()) {
    if (row.source_id == node_id) {
      transmission_ids.emplace_back(row.transmission.node_id());
    }
  }

  for (const auto& transmission_id : transmission_ids) {
    task_manager_.PostDeleteTask(transmission_id);
  }
}

// static
TransmissionModel::Row TransmissionModel::MakeRow(NodeRef transmission) {
  return Row{transmission};
}
