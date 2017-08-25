#include "client/components/hardware_tree/hardware_tree_model.h"

#include "client/components/configuration_tree/configuration_tree_view.h"
#include "client/common_resources.h"
#include "client/controller_factory.h"
#include "common/scada_node_ids.h"

class HardwareTreeView : public ConfigurationTreeView {
 public:
  explicit HardwareTreeView(const ControllerContext& context)
      : ConfigurationTreeView{context, std::make_unique<HardwareTreeModel>(
          context.node_service_,
          context.timed_data_service_)} {}
};

REGISTER_CONTROLLER(HardwareTreeView, ID_HARDWARE_VIEW);
