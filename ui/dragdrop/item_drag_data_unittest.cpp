#include "ui/dragdrop/item_drag_data.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

scada::NodeId RoundTrip(const scada::NodeId& node_id) {
  DragData drag_data;
  ItemDragData{node_id}.Save(drag_data);

  ItemDragData loaded;
  EXPECT_TRUE(loaded.Load(drag_data));
  EXPECT_EQ(loaded.item_ids().size(), 1u);
  return loaded.item_ids().empty() ? scada::NodeId{} : loaded.item_ids()[0];
}

TEST(ItemDragDataTest, RoundTripsNumericId) {
  const scada::NodeId node_id{1201, 7};
  EXPECT_EQ(RoundTrip(node_id), node_id);
}

TEST(ItemDragDataTest, RoundTripsStringId) {
  const scada::NodeId node_id{std::string{"Devices.Unit1"}, 3};
  EXPECT_EQ(RoundTrip(node_id), node_id);
}

// The Pickle encoding wrote no identifier for an opaque id and its loader
// rejected the payload, so an opaque node could not be dragged at all.
TEST(ItemDragDataTest, RoundTripsOpaqueId) {
  const scada::NodeId node_id{scada::ByteString{'\x01', '\x02', '\x00'}, 2};
  EXPECT_EQ(RoundTrip(node_id), node_id);
}

TEST(ItemDragDataTest, RoundTripsSeveralIdsInOrder) {
  const std::vector<scada::NodeId> node_ids{
      scada::NodeId{1201, 7}, scada::NodeId{std::string{"Devices.Unit1"}, 3},
      scada::NodeId{scada::ByteString{'\x01', '\x00'}, 2}};
  DragData drag_data;
  ItemDragData{node_ids}.Save(drag_data);

  ItemDragData loaded;
  ASSERT_TRUE(loaded.Load(drag_data));
  EXPECT_TRUE(std::ranges::equal(loaded.item_ids(), node_ids));
}

// The payload is a serialized `protocol::DragNodes` (core/remote/scada.proto):
// field 1 `node_id`, length-delimited, holding a `protocol::NodeId`
// (core/remote/scada_core.proto) whose field 1 `namespace_index` and field 2
// `numeric_id` are varints. Spelled as wire bytes so the test needs no
// generated header.
TEST(ItemDragDataTest, PayloadIsASerializedProtocolDragNodes) {
  DragData drag_data;
  ItemDragData{scada::NodeId{42, 5}}.Save(drag_data);

  auto i = drag_data.find(std::string{ItemDragData::kMimeType});
  ASSERT_NE(i, drag_data.end());
  EXPECT_EQ(i->second, (std::vector<char>{'\x0a', '\x04', '\x08', '\x05',
                                          '\x10', '\x2a'}));
}

// A payload listing a null id among real ones is rejected as a whole, rather
// than dropping the null and handing the rest to the drop target.
TEST(ItemDragDataTest, LoadRejectsANullIdInTheList) {
  DragData drag_data;
  // Two `node_id` entries: {ns 5, numeric 42}, then an empty (null) NodeId.
  drag_data.emplace(std::string{ItemDragData::kMimeType},
                    std::vector<char>{'\x0a', '\x04', '\x08', '\x05', '\x10',
                                      '\x2a', '\x0a', '\x00'});

  ItemDragData loaded;
  EXPECT_FALSE(loaded.Load(drag_data));
  EXPECT_TRUE(loaded.item_ids().empty());
}

TEST(ItemDragDataTest, LoadRejectsMissingMimeType) {
  DragData drag_data;
  drag_data.emplace("text/plain", std::vector<char>{'x'});

  ItemDragData loaded;
  EXPECT_FALSE(loaded.Load(drag_data));
}

TEST(ItemDragDataTest, LoadRejectsMalformedPayload) {
  DragData drag_data;
  // A length-delimited field whose length runs past the end of the buffer.
  drag_data.emplace(std::string{ItemDragData::kMimeType},
                    std::vector<char>{'\x1a', '\x7f', 'a'});

  ItemDragData loaded;
  EXPECT_FALSE(loaded.Load(drag_data));
}

TEST(ItemDragDataTest, LoadRejectsEmptyPayload) {
  DragData drag_data;
  drag_data.emplace(std::string{ItemDragData::kMimeType}, std::vector<char>{});

  ItemDragData loaded;
  EXPECT_FALSE(loaded.Load(drag_data));
}

}  // namespace
