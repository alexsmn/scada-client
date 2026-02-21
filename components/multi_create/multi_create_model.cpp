#include "components/multi_create/multi_create_model.h"

#include "base/u16format.h"
#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/task_manager.h"

#include <format>

namespace {

void FillDeviceItems(const NodeRef& parent,
                     std::map<std::u16string, scada::NodeId>& items) {
  for (auto& node : parent.targets(scada::id::Organizes)) {
    if (IsInstanceOf(node, devices::id::DeviceType)) {
      auto title = GetFullDisplayName(node);
      items.emplace(std::move(title), node.node_id());
      FillDeviceItems(node, items);
    }
  }
}

}  // namespace

MultiCreateModel::MultiCreateModel(MultiCreateContext&& context)
    : MultiCreateContext{std::move(context)} {
  FillDeviceItems(node_service_.GetNode(devices::id::Devices), devices_);
}

std::u16string MultiCreateModel::GetAutoName(bool ts) const {
  return ts ? u"ТС" : u"ТИТ";
}

void MultiCreateModel::Run(const RunParams& params) {
  auto p = devices_.find(params.device);
  auto device_id = p == devices_.end() ? scada::NodeId() : p->second;

  scada::NodeId type_definition_id = params.ts
                                         ? data_items::id::DiscreteItemType
                                         : data_items::id::AnalogItemType;

  for (int i = 0; i < params.count; ++i) {
    int number = params.starting_number + i;
    int address = params.starting_address + i;

    auto display_name = scada::ToLocalizedText(
        u16format(L"{}{}", params.name_prefix, number));
    auto item_path =
        std::format("{}{}", params.path_prefix, address);
    auto path = MakeNodeIdFormula(MakeNestedNodeId(device_id, item_path));

    task_manager_.PostInsertTask(
        {.type_definition_id = type_definition_id,
         .parent_id = parent_id_,
         .attributes = {.display_name = std::move(display_name)},
         .properties = {
             {data_items::id::DataItemType_Input1, std::move(path)}}});
  }
}
