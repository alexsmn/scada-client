#pragma once

#include "core/configuration_types.h"
#include "core/node_attributes.h"

// Can be used as a signature for TableReader.
const wchar_t kNodeIdTitle[] = L"Ид";

class NodeService;
class TableReader;
class TableWriter;

class ResourceError {
 public:
  explicit ResourceError(base::string16 message) : message_{std::move(message)} {}

  const base::string16& message() const { return message_; }

 private:
  const base::string16 message_;
};

void ExportConfiguration(NodeService& node_service, TableWriter& writer);

struct ImportData {
  struct Reference {
    scada::NodeId reference_type_id;
    scada::NodeId delete_target_id;
    scada::NodeId add_target_id;
  };

  struct CreateNode {
    scada::NodeId id;
    scada::NodeId type_id;
    scada::NodeId parent_id;
    scada::NodeAttributes attrs;
    scada::NodeProperties props;
    std::vector<Reference> refs;
  };

  bool IsEmpty() const {
    return create_nodes.empty() && modify_nodes.empty() && delete_nodes.empty();
  }

  std::vector<CreateNode> create_nodes;
  std::vector<CreateNode> modify_nodes;
  std::vector<scada::NodeId> delete_nodes;
};

ImportData ImportConfiguration(NodeService& node_service, TableReader& reader);