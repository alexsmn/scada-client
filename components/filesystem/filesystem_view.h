#include "configuration/tree/configuration_tree_view.h"

#include "resources/common_resources.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/filesystem_node_ids.h"
#include "node_service/node_service.h"

class FileSystemView : public ConfigurationTreeView {
 public:
  explicit FileSystemView(const ControllerContext& context)
      : ConfigurationTreeView{
            context,
            CreateConfigurationTreeModel(context),
            CreateConfigurationTreeDropHandler(context),
        } {}

 private:
  static std::shared_ptr<ConfigurationTreeModel> CreateConfigurationTreeModel(
      const ControllerContext& context) {
    auto model =
        std::make_shared<ConfigurationTreeModel>(ConfigurationTreeModelContext{
            .executor_ = context.executor_,
            .node_service_tree_ =
                std::make_unique<NodeServiceTreeImpl>(NodeServiceTreeImplContext{
                .executor_ = context.executor_,
                .node_service_ = context.node_service_,
                .root_node_ =
                    context.node_service_.GetNode(filesystem::id::FileSystem),
                .reference_filter_ = {{scada::id::Organizes, true}},
                .leaf_type_definition_ids_ = {filesystem::id::FileType}})});
    model->Init();
    return model;
  }

  static std::unique_ptr<ConfigurationTreeDropHandler>
  CreateConfigurationTreeDropHandler(const ControllerContext& context) {
    return std::make_unique<ConfigurationTreeDropHandler>(
        ConfigurationTreeDropHandlerContext{
            context.executor_,
            context.node_service_,
            context.task_manager_,
            context.create_tree_,
        });
  }
};
