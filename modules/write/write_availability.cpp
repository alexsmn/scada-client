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

bool IsTwoStagedControl(const NodeRef& node, bool scada_server) {
  // The property defaults to Yes when absent, which is right for our data
  // items and wrong for every Variable of a foreign server: it has no Control
  // object to select, and the Select would fail before the write was tried.
  return scada_server &&
         node[scada::data_items::id::DataItemType_OutputTwoStaged]
             .value()
             .get_or(true);
}

std::string_view ControlFlowText(bool two_staged, bool confirmation) {
  // One literal per return, unsplit: tools/check_ui_translations.py reads the
  // strings a function returns into Translate(), and a concatenation is not
  // one it can match against client_ru.ts.
  // clang-format off
  if (two_staged && confirmation)
    return "Opens the Control dialog. The command is sent in two stages, select then execute, and you confirm it before it is executed.";
  if (two_staged)
    return "Opens the Control dialog. The command is sent in two stages, select then execute, without confirmation.";
  if (confirmation)
    return "Opens the Control dialog. You confirm the command before it is sent.";
  return "Opens the Control dialog. The command is sent without confirmation.";
  // clang-format on
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
