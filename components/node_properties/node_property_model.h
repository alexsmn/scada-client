#pragma once

#include <vector>

#include "base/memory/weak_ptr.h"
#include "common/node_observer.h"
#include "common/node_ref.h"
#include "controls/property_model.h"
#include "services/property_defs.h"

class PropertyDefinition;
class TaskManager;
struct PropertyContext;

class NodePropertyModel : protected PropertyContext,
                          private NodeRefObserver,
                          public PropertyModel {
 public:
  NodePropertyModel(PropertyContext&& context, NodeRef node);
  virtual ~NodePropertyModel();

  virtual int GetCount() override;
  virtual base::string16 GetName(int index) override;
  virtual base::string16 GetValue(int index) override;
  virtual bool IsInherited(int index) override;
  virtual void SetValue(int index, const base::string16& value) override;
  virtual ui::EditData GetEditData(int index) override;

 private:
  void Update();

  int FindProperty(const scada::NodeId& prop_decl_id) const;
  int FindProperty(scada::AttributeId attribute_id) const;

  void PropertiesChanged(int first, int count);

  // scada::NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  NodeRef node_;

  struct Property {
    base::string16 name;
    scada::AttributeId attribute_id;
    const PropertyDefinition* def;
    scada::NodeId prop_decl_id;
  };

  std::vector<Property> properties_;

  base::WeakPtrFactory<NodePropertyModel> weak_ptr_factory_{this};
};
