#pragma once

#include "base/memory/weak_ptr.h"
#include "components/node_properties/node_group_model.h"
#include "controls/property_model.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "services/property_defs.h"

struct PropertyContext;

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
  virtual void OnNodeFetched(const scada::NodeId& node_id,
                             bool children) override;

  NodeGroupModel root_{*this};

  NodeRef node_;

  base::WeakPtrFactory<NodePropertyModel> weak_ptr_factory_{this};

  friend class NodeGroupModel;
};
