#include "components/configuration_tree/configuration_tree_view.h"

#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/hardware_tree/hardware_tree_model.h"
#include "controller_factory.h"

class HardwareTreeView : public ConfigurationTreeView {
 public:
  explicit HardwareTreeView(const ControllerContext& context)
      : ConfigurationTreeView{
            context,
            *new HardwareTreeModel{context.node_service_, context.task_manager_,
                                   context.timed_data_service_}} {}
};

const WindowInfo kWindowInfo = {ID_HARDWARE_VIEW, "Subsystems", L"Оборудование",
                                WIN_SING,         200,          400};

REGISTER_CONTROLLER(HardwareTreeView, kWindowInfo);
