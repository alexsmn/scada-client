#include "export/configuration/export_data_builder.h"

#include "address_space/test/test_scada_node_states.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

scada::NodeState MakeNodeState(const scada::NodeId& node_id,
                               const scada::NodeId& type_definition_id,
                               const scada::LocalizedText& display_name,
                               std::vector<scada::NodeProperty> props) {
  return {.node_id = node_id,
          .node_class = scada::NodeClass::Variable,
          .type_definition_id = type_definition_id,
          .parent_id = data_items::id::DataItems,
          .reference_type_id = scada::id::Organizes,
          .attributes = {.display_name = display_name},
          .properties = std::move(props)};
}

}  // namespace

TEST(ExportDataBuilder, Test) {
  // TODO: Test properties.

  const std::vector<scada::NodeState> nodes{
      MakeNodeState(
          /*node_id=*/{11, 1},
          /*type_definition_id=*/data_items::id::AnalogItemType,
          /*display_name=*/u"ТИТ 11",
          /*props=*/{{data_items::id::AnalogItemType_DisplayFormat, "#####"}}),
      MakeNodeState(
          /*node_id=*/{22, 2},
          /*type_definition_id=*/data_items::id::DiscreteItemType,
          /*display_name=*/u"ТС 22",
          /*props=*/{{data_items::id::DiscreteItemType_Inversion, true}}),
      MakeNodeState(
          /*node_id=*/{33, 3},
          /*type_definition_id=*/data_items::id::AnalogItemType,
          /*display_name=*/u"ТИТ 33",
          /*props=*/{{data_items::id::AnalogItemType_DisplayFormat, "000"}})};

  StaticNodeService node_service;
  node_service.AddAll(GetScadaNodeStates());
  node_service.AddAll(nodes);

  ExportDataBuilder builder{node_service};

  // ACT

  auto export_data = builder.Build().get();

  // CHECK

  EXPECT_THAT(
      export_data.nodes,
      UnorderedElementsAre(
          FieldsAre(
              /*node_id=*/scada::NodeId{11, 1},
              /*parent_id=*/scada::NodeId{},
              /*type_display_name=*/u"Объект ТИТ",
              /*type_id=*/data_items::id::AnalogItemType,
              /*display_name=*/u"ТИТ 11",
              /*property_values=*/
              ElementsAre(FieldsAre(
                  /*prop_decl_id=*/data_items::id::AnalogItemType_DisplayFormat,
                  /*value=*/"#####",
                  /*target_id=*/_, /*target_display_name=*/_,
                  /*reference=*/_))),
          FieldsAre(
              /*node_id=*/scada::NodeId{22, 2},
              /*parent_id=*/scada::NodeId{},
              /*type_display_name=*/u"Объект ТС",
              /*type_id=*/data_items::id::DiscreteItemType,
              /*display_name=*/u"ТС 22",
              /*property_values=*/
              ElementsAre(FieldsAre(
                  /*prop_decl_id=*/data_items::id::DiscreteItemType_Inversion,
                  /*value=*/true, /*target_id=*/_,
                  /*target_display_name=*/_,
                  /*reference=*/_))),
          FieldsAre(
              /*node_id=*/scada::NodeId{33, 3},
              /*parent_id=*/scada::NodeId{},
              /*type_display_name=*/u"Объект ТИТ",
              /*type_id=*/data_items::id::AnalogItemType,
              /*display_name=*/u"ТИТ 33",
              /*property_values=*/
              ElementsAre(FieldsAre(
                  /*prop_decl_id=*/data_items::id::AnalogItemType_DisplayFormat,
                  /*value=*/"000",
                  /*target_id=*/_, /*target_display_name=*/_,
                  /*reference=*/_)))));

  EXPECT_THAT(export_data.props,
              Contains(FieldsAre(
                  /*prop_decl_id=*/data_items::id::DiscreteItemType_Inversion,
                  /*display_name=*/u"Инверсия",
                  /*reference=*/false)));

  // TODO: Validate a reference.
}