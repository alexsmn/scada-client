#pragma once

#include "node_service/node_ref.h"

#include <unordered_set>
#include <vector>

class PropertyDefinition;

using PropertyDefs =
    std::vector<std::pair<NodeRef /*prop_decl*/, const PropertyDefinition*>>;

class PropertyService {
 public:
  const PropertyDefinition* GetPropertyDef(const NodeRef& prop_decl);

  PropertyDefs GetTypePropertyDefs(const NodeRef& type_definition);

  scada::status_promise<PropertyDefs> GetChildPropertyDefs(
      const NodeRef& parent_node);

  // Returns property declarations and forward reference types.
  void GetTypeProperties(const NodeRef& type_definition,
                         std::unordered_set<NodeRef>& property_declarations);

 private:
  scada::status_promise<void> GetAllSubtypesProperties(
      const NodeRef& type_definition,
      const std::shared_ptr<std::unordered_set<NodeRef>>& property_decls);

  PropertyDefs GetPropertyDefs(
      const std::unordered_set<NodeRef>& property_decls);
};
