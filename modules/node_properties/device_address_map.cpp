#include "node_properties/device_address_map.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/basic_types.h"
#include "scada/localized_text.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <string>
#include <utility>

std::u16string SignalTypeTag(const scada::NodeId& type_definition_id) {
  if (type_definition_id == scada::data_items::id::DiscreteItemType)
    return u"TS";
  if (type_definition_id == scada::data_items::id::AnalogItemType)
    return u"TI";
  return {};
}

Awaitable<std::vector<AddressMapRow>> BuildDeviceAddressMap(
    AnyExecutor executor,
    NodeRef device) {
  std::vector<AddressMapRow> rows;

  co_await device.Fetch(NodeFetchStatus::NodeAndChildren);
  for (NodeRef& child : device.targets(scada::id::Organizes)) {
    co_await child.Fetch(NodeFetchStatus::NodeAndChildren);

    // Fetch the child's type chain first: operator[](declaration_id) maps the
    // SourceNode/Address aggregate declarations to instance properties
    // through the type, so the declarations must be resident.
    for (NodeRef type = child.type_definition(); type;) {
      co_await type.Fetch(NodeFetchStatus::NodeAndChildren);
      type = type.supertype();
    }

    // A transmission item links a source data item to a protocol address via
    // the SourceNode NodeId property (transmission OPC UA alignment, phase 4);
    // a child without that property is some other node. The property node's
    // value needs its own fetch (children fetch loads the node, not the
    // value).
    NodeRef source_prop =
        child[scada::devices::id::TransmissionItemType_SourceNode];
    if (!source_prop)
      continue;
    co_await source_prop.Fetch(NodeFetchStatus::NodeOnly);
    const scada::NodeId source_id = source_prop.value().get_or(scada::NodeId{});
    if (source_id.is_null())
      continue;
    NodeRef source = device.service()->GetNode(source_id);
    co_await source.Fetch(NodeFetchStatus::NodeOnly);

    // Fetch the source-address property node itself so its Value attribute is
    // resident (children fetch loads the property node but not its value).
    scada::Int32 ioa = 0;
    if (NodeRef address =
            child[scada::devices::id::TransmissionItemType_Address]) {
      co_await address.Fetch(NodeFetchStatus::NodeOnly);
      ioa = address.value().get_or<scada::Int32>(0);
    }

    AddressMapRow row;
    row.signal = source.display_name();
    row.type = SignalTypeTag(source.type_definition().node_id());
    row.ioa = scada::ToLocalizedText(std::to_string(ioa));
    row.node_id = scada::ToLocalizedText(NodeIdToScadaString(source.node_id()));
    rows.push_back(std::move(row));
  }

  co_return rows;
}
