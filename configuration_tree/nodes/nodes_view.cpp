#include "configuration_tree/nodes/nodes_view.h"

#include "configuration_tree/lib/configuration_tree_drop_handler.h"
#include "configuration_tree/lib/configuration_tree_model.h"
#include "configuration_tree/lib/node_service_tree_impl.h"
#include "node_service/node_service.h"

NodesView::NodesView(const ControllerContext& context)
    : ConfigurationTreeView{context, CreateConfigurationTreeModel(context),
                            CreateConfigurationTreeDropHandler(context)} {}

// static
std::shared_ptr<ConfigurationTreeModel> NodesView::CreateConfigurationTreeModel(
    const ControllerContext& context) {
  auto node_service_tree =
      std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
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
          context.node_service_,
          context.task_manager_,
          context.create_tree_,
      });
}
