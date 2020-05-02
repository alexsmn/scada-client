#include "components/multi_create/multi_create_model.h"

#include "base/strings/stringprintf.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "services/task_manager.h"

namespace {

void FillDeviceItems(const NodeRef& parent,
                     std::map<base::string16, scada::NodeId>& items) {
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

base::string16 MultiCreateModel::GetAutoName(bool ts) const {
  return ts ? L"ТС" : L"ТИТ";
}

void MultiCreateModel::Run(const RunParams& params) {
  auto i = devices_.find(params.device);
  auto device_id = i == devices_.end() ? scada::NodeId() : i->second;

  scada::NodeId type_definition_id = params.ts
                                         ? data_items::id::DiscreteItemType
                                         : data_items::id::AnalogItemType;

  for (int i = 0; i < params.count; ++i) {
    int number = params.starting_number + i;
    int address = params.starting_address + i;

    auto display_name = scada::ToLocalizedText(
        base::StringPrintf(L"%ls%d", params.name_prefix.c_str(), number));
    auto item_path =
        base::StringPrintf("%s%d", params.path_prefix.c_str(), address);
    auto path = MakeNodeIdFormula(MakeNestedNodeId(device_id, item_path));

    task_manager_.PostInsertTask(
        scada::NodeId(), parent_id_, type_definition_id,
        scada::NodeAttributes().set_display_name(std::move(display_name)),
        {{data_items::id::DataItemType_Input1, std::move(path)}});
  }
}
