#pragma once

#include "base/cancelation.h"
#include "components/node_properties/node_group_model.h"
#include "aui/models/property_model.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "properties/property_context.h"

#include <boost/signals2/signal.hpp>

class PropertyService;
struct PropertyContext;

class NodePropertyModel : protected PropertyContext,
                          private NodeRefObserver,
                          public aui::PropertyModel {
 public:
  NodePropertyModel(PropertyService& property_service,
                    PropertyContext&& context,
                    NodeRef node);
  virtual ~NodePropertyModel();

  const NodeRef& node() const { return node_; }

  boost::signals2::signal<void()> node_deleted;

 private:
  void OnNodeFetched();
  void Update();

  void InitProperty(NodeGroupModel::Property& prop);

  int FindProperty(const scada::NodeId& prop_decl_id) const;
  int FindProperty(scada::AttributeId attribute_id) const;

  void PropertiesChanged(int first, int count);

  // PropertyModel
  virtual aui::PropertyGroup& GetRootGroup() override { return root_; }

  // scada::NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  PropertyService& property_service_;

  NodeGroupModel root_{*this};

  NodeRef node_;

  Cancelation cancelation_;

  friend class NodeGroupModel;
};
