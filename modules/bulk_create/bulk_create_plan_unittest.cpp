#include "bulk_create/bulk_create_plan.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace {

// The planner is the seam between "what the pattern expanded" and "what gets
// created", so these assert the node the server would actually receive:
// its requested id, its type, its parent and the one property that differs
// between the two subjects.

BulkCreateParams DataItemParams(int count) {
  BulkCreateParams params;
  params.subject = BulkCreateSubject::kDataItem;
  params.name_template = u"AI{n}";
  params.node_id_template = u"ns=2;s=RTU.AI{n}";
  params.count = count;
  return params;
}

BulkCreateParams TransmissionParams(int count) {
  BulkCreateParams params;
  params.subject = BulkCreateSubject::kTransmissionItem;
  params.name_template = u"TX{n}";
  params.node_id_template = u"ns=2;s=DEST.TX{n}";
  params.count = count;
  params.ioa_start = 4001;
  params.ioa_step = 1;
  return params;
}

const scada::Variant* FindProperty(const scada::NodeState& node,
                                   const scada::NodeId& decl_id) {
  for (const auto& property : node.properties) {
    if (property.first == decl_id)
      return &property.second;
  }
  return nullptr;
}

TEST(PlanBulkCreateTest, DataItemCarriesItsSourceFormulaAndNoAddress) {
  BulkCreatePlan plan;
  plan.subject = BulkCreateSubject::kDataItem;
  plan.parent_id = scada::NodeId::FromString("ns=2;s=Folder");
  plan.type_definition_id = scada::data_items::id::AnalogItemType;
  plan.source_device_id = scada::NodeId::FromString("ns=2;s=RTU1");
  plan.source_path_template = u"Signal{n}";

  const std::vector<scada::NodeState> nodes =
      PlanBulkCreate(plan, ExpandBulkCreate(DataItemParams(2), {}));

  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].type_definition_id, scada::data_items::id::AnalogItemType);
  EXPECT_EQ(nodes[0].parent_id, plan.parent_id);
  // The requested id is the one the preview showed, which is the whole reason
  // the grid's NodeId column can be believed.
  EXPECT_EQ(nodes[0].node_id, scada::NodeId::FromString("ns=2;s=RTU.AI1"));
  EXPECT_NE(FindProperty(nodes[0], scada::data_items::id::DataItemType_Input1),
            nullptr);
  // A data item is not addressed on a link, so it must carry no Address at all
  // rather than a zero.
  EXPECT_EQ(
      FindProperty(nodes[0], scada::devices::id::TransmissionItemType_Address),
      nullptr);
}

// The source path steps with the row exactly as the name does; a fixed source
// across every row would bind every created item to one signal.
TEST(PlanBulkCreateTest, DataItemSourcePathStepsWithTheRow) {
  BulkCreatePlan plan;
  plan.subject = BulkCreateSubject::kDataItem;
  plan.type_definition_id = scada::data_items::id::AnalogItemType;
  plan.source_device_id = scada::NodeId::FromString("ns=2;s=RTU1");
  plan.source_path_template = u"Signal{n}";

  const std::vector<scada::NodeState> nodes =
      PlanBulkCreate(plan, ExpandBulkCreate(DataItemParams(3), {}));

  ASSERT_EQ(nodes.size(), 3u);
  const scada::Variant* first =
      FindProperty(nodes[0], scada::data_items::id::DataItemType_Input1);
  const scada::Variant* last =
      FindProperty(nodes[2], scada::data_items::id::DataItemType_Input1);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(last, nullptr);
  EXPECT_NE(*first, *last);
}

TEST(PlanBulkCreateTest, TransmissionItemCarriesItsSourceNodeAndAddress) {
  BulkCreatePlan plan;
  plan.subject = BulkCreateSubject::kTransmissionItem;
  plan.parent_id = scada::NodeId::FromString("ns=2;s=Dest");
  plan.type_definition_id = scada::devices::id::Iec60870TransmissionItemType;
  plan.source_node_ids = {scada::NodeId::FromString("ns=2;s=A"),
                          scada::NodeId::FromString("ns=2;s=B")};

  const std::vector<scada::NodeState> nodes =
      PlanBulkCreate(plan, ExpandBulkCreate(TransmissionParams(2), {}));

  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].type_definition_id,
            scada::devices::id::Iec60870TransmissionItemType);
  const scada::Variant* source = FindProperty(
      nodes[1], scada::devices::id::TransmissionItemType_SourceNode);
  ASSERT_NE(source, nullptr);
  // Sources are consumed in row order, so the second rule forwards the second
  // selected node.
  EXPECT_EQ(*source, scada::Variant{plan.source_node_ids[1]});
  const scada::Variant* address =
      FindProperty(nodes[1], scada::devices::id::TransmissionItemType_Address);
  ASSERT_NE(address, nullptr);
  EXPECT_EQ(*address, scada::Variant{4002});
}

// The footer says "will create 23 of 24", and it is the size of this result --
// so a conflicting row has to be dropped here rather than counted separately,
// or the two can disagree.
TEST(PlanBulkCreateTest, ConflictingRowsAreNotCreated) {
  BulkCreatePlan plan;
  plan.subject = BulkCreateSubject::kDataItem;
  plan.type_definition_id = scada::data_items::id::AnalogItemType;
  plan.source_device_id = scada::NodeId::FromString("ns=2;s=RTU1");
  plan.source_path_template = u"S{n}";

  const std::set<std::u16string> existing{u"ns=2;s=RTU.AI2"};
  const std::vector<scada::NodeState> nodes =
      PlanBulkCreate(plan, ExpandBulkCreate(DataItemParams(3), existing));

  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].node_id, scada::NodeId::FromString("ns=2;s=RTU.AI1"));
  EXPECT_EQ(nodes[1].node_id, scada::NodeId::FromString("ns=2;s=RTU.AI3"));
}

// A dropped row must not consume the source the next kept row needs, or every
// rule after the first conflict forwards the wrong node -- silently, since
// each one still looks well-formed.
TEST(PlanBulkCreateTest, ASkippedRowDoesNotConsumeASource) {
  BulkCreatePlan plan;
  plan.subject = BulkCreateSubject::kTransmissionItem;
  plan.type_definition_id = scada::devices::id::Iec60870TransmissionItemType;
  plan.source_node_ids = {scada::NodeId::FromString("ns=2;s=A"),
                          scada::NodeId::FromString("ns=2;s=B")};

  const std::set<std::u16string> existing{u"ns=2;s=DEST.TX1"};
  const std::vector<scada::NodeState> nodes =
      PlanBulkCreate(plan, ExpandBulkCreate(TransmissionParams(3), existing));

  ASSERT_EQ(nodes.size(), 2u);
  // TX1 was skipped, so TX2 -- the first node actually created -- takes the
  // FIRST selected source, not the second.
  const scada::Variant* source = FindProperty(
      nodes[0], scada::devices::id::TransmissionItemType_SourceNode);
  ASSERT_NE(source, nullptr);
  EXPECT_EQ(*source, scada::Variant{plan.source_node_ids[0]});
}

// Fewer sources than rows ends the run rather than creating rules that forward
// nothing, which would be a rule the operator has to find and delete.
TEST(PlanBulkCreateTest, RunningOutOfSourcesStopsRatherThanCreatingEmptyRules) {
  BulkCreatePlan plan;
  plan.subject = BulkCreateSubject::kTransmissionItem;
  plan.type_definition_id = scada::devices::id::Iec60870TransmissionItemType;
  plan.source_node_ids = {scada::NodeId::FromString("ns=2;s=A")};

  const std::vector<scada::NodeState> nodes =
      PlanBulkCreate(plan, ExpandBulkCreate(TransmissionParams(4), {}));

  EXPECT_EQ(nodes.size(), 1u);
}

TEST(PlanBulkCreateTest, NoRowsCreatesNothing) {
  BulkCreatePlan plan;
  plan.type_definition_id = scada::data_items::id::AnalogItemType;
  EXPECT_TRUE(PlanBulkCreate(plan, {}).empty());
}

}  // namespace
