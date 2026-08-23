#include "modules/transmission_rules/transmission_rule_fetch.h"

#include "model/devices_node_ids.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_service.h"
#include "scada/node_id.h"

Awaitable<void> FetchTransmissionRule(NodeRef transmission) {
  if (!transmission)
    co_return;

  co_await transmission.Fetch(NodeFetchStatus::NodeAndChildren);

  // The whole chain: `Address` and `SourceNode` are declared on the
  // TransmissionItemType supertype, so the immediate type alone leaves the
  // subscript unable to resolve either declaration.
  for (NodeRef type = transmission.type_definition(); type;) {
    co_await type.Fetch(NodeFetchStatus::NodeAndChildren);
    type = type.supertype();
  }

  static constexpr scada::NodeId kProperties[] = {
      scada::devices::id::TransmissionItemType_SourceNode,
      scada::devices::id::TransmissionItemType_Address,
  };
  for (const scada::NodeId& property_id : kProperties) {
    if (NodeRef property = transmission[property_id])
      co_await property.Fetch(NodeFetchStatus::NodeOnly);
  }

  // The destination endpoint the rule hangs under, for its name.
  if (NodeRef endpoint = transmission.parent())
    co_await endpoint.Fetch(NodeFetchStatus::NodeOnly);

  // The source signal, readable only now that the declaration resolved: the
  // link is a NodeId *value*, so nothing above this line has touched it. Its
  // own fetch answers the display name, and its type definition answers the
  // TS/TI tag.
  const scada::NodeId source_id =
      transmission[scada::devices::id::TransmissionItemType_SourceNode]
          .value()
          .get_or(scada::NodeId{});
  if (source_id.is_null())
    co_return;
  if (NodeRef source = transmission.service()->GetNode(source_id))
    co_await source.Fetch(NodeFetchStatus::NodeOnly);
}
