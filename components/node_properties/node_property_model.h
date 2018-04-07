#pragma once

#include <map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "common/node_observer.h"
#include "common/node_ref.h"
#include "services/property_defs.h"
#include "ui/base/models/tree_model.h"

class PropertyDefinition;
class TaskManager;
struct PropertyContext;

class NodePropertyModel : private PropertyContext,
                          public ui::TreeModel,
                          private NodeRefObserver {
 public:
  NodePropertyModel(PropertyContext&& context, NodeRef node);
  virtual ~NodePropertyModel();

  virtual int GetColumnCount() const { return 2; }

  virtual void* GetRoot() override { return this; }
  virtual void* GetParent(void* node) override;
  virtual int GetChildCount(void* parent) override;
  virtual void* GetChild(void* parent, int index) override;
  virtual base::string16 GetText(void* node, int column_id) override;
  virtual void SetText(void* node,
                       int column_id,
                       const base::string16& text) override;

 private:
  void Update();

  virtual int GetCount();
  virtual base::string16 GetName(int index);
  virtual base::string16 GetValue(int index);
  virtual bool IsInherited(int index);
  virtual void SetValue(int index, const base::string16& value);

  int NodeToIndex(void* node) const;
  int FindProperty(const scada::NodeId& prop_type_id) const;
  int FindProperty(scada::AttributeId attribute_id) const;

  // scada::NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  NodeRef node_;

  struct Property {
    base::string16 name;
    base::string16 string_value;
    scada::AttributeId attribute_id;
    const PropertyDefinition* def;
    scada::NodeId prop_type_id;
  };

  std::vector<Property> properties_;

  base::WeakPtrFactory<NodePropertyModel> weak_ptr_factory_{this};
};
