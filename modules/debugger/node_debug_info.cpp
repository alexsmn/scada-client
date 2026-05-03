#include "node_debug_info.h"

#include "resources/common_resources.h"
#include "controller/selection_model.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "scada/localized_text.h"

#include <memory>

namespace {

void PrintReferences(std::ostream& stream,
                     std::span<const NodeRef::Reference> references) {
  for (const auto& r : references) {
    stream << "{";
    stream << "forward: " << r.forward;
    stream << ", ";
    stream << "type: " << ToString(r.reference_type.browse_name());
    stream << ", ";
    stream << "target: " << NodeIdToScadaString(r.target.node_id());
    stream << "}";
    stream << std::endl;
  }
}

}

std::string GetNodeDebugInfo(const NodeRef& node) {
  std::ostringstream stream;
  stream << NodeIdToScadaString(node.node_id()) << std::endl;

  stream << "Fetched: " << node.fetched() << std::endl;
  stream << "Children fetched: " << node.children_fetched() << std::endl;
  stream << std::endl;

  stream << "Attributes:" << std::endl;
  stream << "BrowseName: " << ToString(node.browse_name()) << std::endl;
  stream << "DisplayName: " << ToString(node.display_name()) << std::endl;
  stream << "TypeDefinition: " << ToString(node.type_definition().browse_name())
         << std::endl;
  stream << "Supertype: " << ToString(node.supertype().browse_name()) << std::endl;
  stream << std::endl;

  stream << "References:" << std::endl;
  PrintReferences(stream, node.references());
  stream << std::endl;

  stream << "Inverse references:" << std::endl;
  PrintReferences(stream, node.inverse_references());
  stream << std::endl;

  return stream.str();
}
