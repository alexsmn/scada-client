#include "components/configuration_tree/configuration_tree_view.h"

#include "common_resources.h"
#include "components/configuration_tree/configuration_tree_drop_handler.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "components/configuration_tree/node_service_tree_impl.h"
#include "model/filesystem_node_ids.h"
#include "node_service/node_service.h"

class FileSystemView : public ConfigurationTreeView {
 public:
  explicit FileSystemView(const ControllerContext& context)
      : ConfigurationTreeView{
            context,
            *new ConfigurationTreeModel(ConfigurationTreeModelContext{
                std::make_unique<NodeServiceTreeImpl>(
                    NodeServiceTreeImplContext{
                        context.node_service_,
                        context.node_service_.GetNode(
                            filesystem::id::FileSystem),
                        {{scada::id::Organizes, true}},
                        {},
                    })}),
            *new ConfigurationTreeDropHandler{
                ConfigurationTreeDropHandlerContext{
                    context.node_service_,
                    context.task_manager_,
                }}} {}
};
