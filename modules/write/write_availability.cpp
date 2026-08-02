#include "modules/write/write_availability.h"

#include "model/data_items_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/node_class.h"

WriteBlock GetWriteBlock(const NodeRef& node) {
  // TODO: Use `scada::AttributeId::UserWriteMask` when available. Until then
  // every Variable is writable, except a data item that drives no output.
  if (node.node_class() != scada::NodeClass::Variable)
    return WriteBlock::kNotCommandable;

  if (IsInstanceOf(node, scada::data_items::id::DataItemType) &&
      node[scada::data_items::id::DataItemType_Output].value().is_null()) {
    return WriteBlock::kNoOutputChannel;
  }

  return WriteBlock::kNone;
}

std::string_view WriteBlockText(WriteBlock block) {
  switch (block) {
    case WriteBlock::kNotCommandable:
      return "This object cannot be controlled";
    case WriteBlock::kNoOutputChannel:
      return "The signal has no output channel";
    case WriteBlock::kNone:
      return {};
  }
  return {};
}
