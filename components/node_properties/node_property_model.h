#pragma once

#include <vector>

#include "base/memory/weak_ptr.h"
#include "common/node_observer.h"
#include "common/node_ref.h"
#include "controls/property_model.h"
#include "services/property_defs.h"

class NodePropertyModel;
class PropertyDefinition;
class TaskManager;
struct PropertyContext;

class NodeGroupModel : public PropertyGroup {
 public:
  explicit NodeGroupModel(NodePropertyModel& property_model);
  ~NodeGroupModel();

  virtual int GetCount() const override;
  virtual PropertyGroup* GetSubgroup(int index) const override;
  virtual base::string16 GetName(int index) const override;
  virtual base::string16 GetValue(int index) const override;
  virtual bool IsInherited(int index) const override;
  virtual void SetValue(int index, const base::string16& value) override;
  virtual ui::EditData GetEditData(int index) const override;

  struct Property {
    base::string16 name;
    scada::AttributeId attribute_id;
    const PropertyDefinition* def;
    scada::NodeId prop_decl_id;
    std::unique_ptr<NodeGroupModel> submodel;
  };

  base::string16 group_title;
  std::vector<Property> properties;

 private:
  NodePropertyModel& property_model_;
};

class NodePropertyModel : protected PropertyContext,
                          private NodeRefObserver,
                          public PropertyModel {
 public:
  NodePropertyModel(PropertyContext&& context, NodeRef node);
  virtual ~NodePropertyModel();

  const NodeRef& node() const { return node_; }

 private:
  void Update();

  int FindProperty(const scada::NodeId& prop_decl_id) const;
  int FindProperty(scada::AttributeId attribute_id) const;

  void PropertiesChanged(int first, int count);

  // PropertyModel
  virtual PropertyGroup& GetRootGroup() { return root_; }

  // scada::NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnNodeFetched(const scada::NodeId& node_id, bool children) override;

  NodeGroupModel root_{*this};

  NodeRef node_;

  base::WeakPtrFactory<NodePropertyModel> weak_ptr_factory_{this};

  friend class NodeGroupModel;
};
