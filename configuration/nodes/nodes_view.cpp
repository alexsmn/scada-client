#include "configuration/nodes/nodes_view.h"

#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "node_service/node_service.h"

NodesView::NodesView(const ControllerContext& context,
                     const NodeServiceTreeFactory& node_service_tree_factory)
    : ConfigurationTreeView{
          context,
          CreateConfigurationTreeModel(context, node_service_tree_factory),
          CreateConfigurationTreeDropHandler(context)} {}

// static
std::shared_ptr<ConfigurationTreeModel> NodesView::CreateConfigurationTreeModel(
    const ControllerContext& context,
    const NodeServiceTreeFactory& node_service_tree_factory) {
  auto node_service_tree =
      node_service_tree_factory(NodeServiceTreeImplContext{
          .executor_ = context.executor_,
          .node_service_ = context.node_service_,
          .root_node_ = context.node_service_.GetNode(scada::id::RootFolder),
          .reference_filter_ = {{scada::id::HierarchicalReferences, true}}});
  auto model =
      std::make_shared<ConfigurationTreeModel>(ConfigurationTreeModelContext{
          .node_service_tree_ = std::move(node_service_tree)});
  model->Init();
  return model;
}

// static
std::unique_ptr<ConfigurationTreeDropHandler>
NodesView::CreateConfigurationTreeDropHandler(
    const ControllerContext& context) {
  return std::make_unique<ConfigurationTreeDropHandler>(
      ConfigurationTreeDropHandlerContext{
          context.executor_,
          context.node_service_,
          context.task_manager_,
          context.create_tree_,
      });
}
