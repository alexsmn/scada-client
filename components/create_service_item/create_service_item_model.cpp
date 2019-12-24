#include "components/create_service_item/create_service_item_model.h"

#include "common/formula_util.h"
#include "model/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "services/task_manager.h"

CreateServiceItemModel::CreateServiceItemModel(
    CreateServiceItemContext&& context)
    : CreateServiceItemContext{std::move(context)} {
  devices_ = GetNamedNodes(node_service_.GetNode(id::Devices), id::DeviceType);
  SortNamedNodes(devices_);

  if (!devices_.empty())
    SetDeviceIndex(0);
}

void CreateServiceItemModel::SetDeviceIndex(int index) {
  device_index_ = index;

  components_.clear();

  if (device_index_ == -1)
    return;

  auto device = devices_[device_index_].second;
  if (!device)
    return;

  for (const auto& component : GetDataVariables(device))
    components_.emplace_back(ToString16(component.display_name()), component);

  SortNamedNodes(components_);
}

void CreateServiceItemModel::Run(const RunParams& params) {
  if (device_index_ == -1)
    return;

  auto device = devices_[device_index_].second;
  if (!device)
    return;

  for (int component_index : params.component_indexes) {
    const auto& component = components_[component_index].second;
    if (!component)
      continue;

    auto display_name = component.display_name();
    auto formula = MakeNodeIdFormula(
        MakeNestedNodeId(device.node_id(), component.browse_name().name()));
    auto type_definition_id =
        IsSubtypeOf(component.data_type(), scada::id::Boolean)
            ? id::DiscreteItemType
            : id::AnalogItemType;

    scada::NodeProperties properties;
    properties.emplace_back(id::DataItemType_Input1, std::move(formula));
    if (type_definition_id == id::AnalogItemType)
      properties.emplace_back(id::AnalogItemType_DisplayFormat, "0.");

    task_manager_.PostInsertTask(
        scada::NodeId{}, parent_id_, type_definition_id,
        scada::NodeAttributes{}.set_display_name(std::move(display_name)),
        std::move(properties));
  }
}
