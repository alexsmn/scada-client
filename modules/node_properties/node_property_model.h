#pragma once

#include "aui/models/property_model.h"
#include "base/cancelation.h"
#include "base/lifetime.h"
#include "modules/node_properties/node_group_model.h"
#include "node_service/node_ref.h"
#include "properties/property_context.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>

#include <memory>

class PropertyService;
struct PropertyContext;

class NodePropertyModel : protected PropertyContext,
                          public aui::PropertyModel,
                          public std::enable_shared_from_this<NodePropertyModel> {
 public:
  NodePropertyModel(PropertyService& property_service,
                    PropertyContext&& context,
                    NodeRef node);
  virtual ~NodePropertyModel();

  const NodeRef& node() const SCADA_LIFETIME_BOUND { return node_; }

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

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged(const scada::NodeId& node_id);

  PropertyService& property_service_;

  NodeGroupModel root_{*this};

  NodeRef node_;

  Cancelation cancelation_;

  boost::signals2::scoped_connection model_changed_connection_;
  boost::signals2::scoped_connection node_semantic_changed_connection_;

  friend class NodeGroupModel;
};
