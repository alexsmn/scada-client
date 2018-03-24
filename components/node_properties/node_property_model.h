#pragma once

#include <vector>

#include "common/node_observer.h"
#include "common/node_ref.h"
#include "core/configuration_types.h"
#include "services/property_defs.h"
#include "ui/base/models/property_list_model.h"

class NodeService;
class PropertyDefinition;

class NodePropertyModel : private PropertyContext,
                          public ui::PropertyListModel,
                          private NodeRefObserver {
 public:
  typedef std::vector<NodeRef> Nodes;

  NodePropertyModel(NodeService& node_service,
                    TaskManager& task_manager,
                    Nodes nodes);
  virtual ~NodePropertyModel();

  virtual int GetCount() override;
  virtual base::string16 GetName(int index) override;
  virtual base::string16 GetValue(int index) override;
  virtual bool IsInherited(int index) override;
  virtual void SetValue(int index, const base::string16& value) override;

 private:
  int FindProperty(const scada::NodeId& prop_type_id) const;

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  Nodes nodes_;

  struct Property {
    scada::LocalizedText display_name;
    base::string16 string_value;
    const PropertyDefinition* def;
    scada::NodeId prop_type_id;
  };

  std::vector<Property> properties_;
};
