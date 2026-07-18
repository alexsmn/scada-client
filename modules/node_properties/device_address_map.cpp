#include "node_properties/device_address_map.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
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

Awaitable<std::vector<AddressMapRow>> BuildDeviceAddressMap(AnyExecutor executor,
                                                            NodeRef device) {
  std::vector<AddressMapRow> rows;

  co_await device.Fetch(NodeFetchStatus::NodeAndChildren);
  for (NodeRef& child : device.targets(scada::id::Organizes)) {
    // A transmission item links a source data item to a protocol address; a
    // child without a HasTransmissionSource target is some other node.
    co_await child.Fetch(NodeFetchStatus::NodeAndChildren);
    NodeRef source = child.target(scada::devices::id::HasTransmissionSource);
    if (!source)
      continue;
    co_await source.Fetch(NodeFetchStatus::NodeOnly);

    const scada::Int32 ioa =
        child[scada::devices::id::TransmissionItemType_SourceAddress]
            .value()
            .get_or<scada::Int32>(0);

    AddressMapRow row;
    row.signal = source.display_name();
    row.type = SignalTypeTag(source.type_definition().node_id());
    row.ioa = scada::ToLocalizedText(std::to_string(ioa));
    row.node_id = scada::ToLocalizedText(NodeIdToScadaString(source.node_id()));
    rows.push_back(std::move(row));
  }

  co_return rows;
}
