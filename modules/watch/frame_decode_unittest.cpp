#include "modules/watch/frame_decode.h"

#include "base/lifetime.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <vector>

namespace {

scada::DeviceFrame Frame(std::initializer_list<int> octets) {
  scada::DeviceFrame frame;
  frame.direction = scada::DeviceFrame::kInbound;
  for (int octet : octets)
    frame.raw_data.push_back(static_cast<char>(octet));
  return frame;
}

// Finds a field by name anywhere under `node`, so a test can assert on one
// field without hard-coding the shape of everything around it.
const FrameDecodeNode* Find(const FrameDecodeNode& node SCADA_LIFETIME_BOUND,
                            std::u16string_view name) {
  for (const FrameDecodeNode& child : node.children) {
    if (child.name == name)
      return &child;
    if (const FrameDecodeNode* found = Find(child, name))
      return found;
  }
  return nullptr;
}

const FrameDecodeNode* Find(const FrameDecode& decode SCADA_LIFETIME_BOUND,
                            std::u16string_view name) {
  for (const FrameDecodeNode& node : decode.nodes) {
    if (node.name == name)
      return &node;
    if (const FrameDecodeNode* found = Find(node, name))
      return found;
  }
  return nullptr;
}

// A complete I-format APDU carrying M_ME_NC_1 (short float) for IOA 4002:
// 68 12 | FA 0F 84 0C | 0D 01 03 00 01 00 | A2 0F 00 | 00 80 02 43 | 00.
// 0x43028000 is IEEE-754 for 130.5, little-endian on the wire.
scada::DeviceFrame MeasurementFrame() {
  return Frame({0x68, 0x12, 0xFA, 0x0F, 0x84, 0x0C, 0x0D, 0x01, 0x03, 0x00,
                0x01, 0x00, 0xA2, 0x0F, 0x00, 0x00, 0x80, 0x02, 0x43, 0x00});
}

TEST(FrameDecodeTest, ShowsEveryOctetAsHex) {
  const FrameDecode decode = DecodeFrame(Frame({0x68, 0x04, 0x0B, 0x00, 0x00,
                                                0x00}));

  EXPECT_EQ(decode.hex, u"68 04 0B 00 00 00");
}

// The APCI fields, with the byte ranges that are the reason the pane exists.
TEST(FrameDecodeTest, DecodesTheApciWithByteRanges) {
  const FrameDecode decode = DecodeFrame(MeasurementFrame());

  ASSERT_FALSE(decode.nodes.empty());
  const FrameDecodeNode& apci = decode.nodes[0];
  EXPECT_EQ(apci.value, u"I-format");

  const FrameDecodeNode* start = Find(decode, u"Start");
  ASSERT_TRUE(start);
  EXPECT_EQ(start->value, u"0x68");
  EXPECT_EQ(start->offset, 0);
  EXPECT_EQ(start->length, 1);

  const FrameDecodeNode* send = Find(decode, u"N(S) send");
  ASSERT_TRUE(send);
  EXPECT_EQ(send->value, u"2045");
  EXPECT_EQ(send->offset, 2);
  EXPECT_EQ(send->length, 2);

  const FrameDecodeNode* recv = Find(decode, u"N(R) recv");
  ASSERT_TRUE(recv);
  EXPECT_EQ(recv->value, u"1602");
  EXPECT_EQ(recv->offset, 4);
  EXPECT_EQ(recv->length, 2);
}

TEST(FrameDecodeTest, DecodesTheAsduHeader) {
  const FrameDecode decode = DecodeFrame(MeasurementFrame());

  ASSERT_EQ(decode.nodes.size(), 2u);
  EXPECT_EQ(decode.nodes[1].value, u"M_ME_NC_1");
  EXPECT_EQ(decode.summary, u"I-format · M_ME_NC_1");

  const FrameDecodeNode* type = Find(decode, u"Type ID");
  ASSERT_TRUE(type);
  EXPECT_EQ(type->value, u"13 · M_ME_NC_1");
  EXPECT_EQ(type->offset, 6);

  const FrameDecodeNode* cause = Find(decode, u"Cause (COT)");
  ASSERT_TRUE(cause);
  EXPECT_EQ(cause->value, u"3 · spont");

  const FrameDecodeNode* address = Find(decode, u"Common address");
  ASSERT_TRUE(address);
  EXPECT_EQ(address->value, u"1");
  EXPECT_EQ(address->offset, 10);
  EXPECT_EQ(address->length, 2);
}

// The information object: the address, the value in engineering form, and the
// quality. This is what an engineer opened the pane to read.
TEST(FrameDecodeTest, DecodesTheInformationObject) {
  const FrameDecode decode = DecodeFrame(MeasurementFrame());

  const FrameDecodeNode* object = Find(decode, u"Object");
  ASSERT_TRUE(object);
  EXPECT_EQ(object->value, u"IOA 4002");
  EXPECT_EQ(object->offset, 12);

  const FrameDecodeNode* value = Find(decode, u"Value");
  ASSERT_TRUE(value);
  EXPECT_EQ(value->value, u"130.5");
  EXPECT_EQ(value->offset, 15);
  EXPECT_EQ(value->length, 4);

  const FrameDecodeNode* quality = Find(decode, u"Quality");
  ASSERT_TRUE(quality);
  EXPECT_EQ(quality->value, u"0x00 · good");
}

// A bad-quality reading is the case the pane is opened for, so the flags are
// spelled out rather than left as a hex byte.
TEST(FrameDecodeTest, NamesTheQualityFlags) {
  // M_SP_NA_1, IOA 1001, SIQ = 0x81: the point is ON and the value is invalid.
  const FrameDecode decode =
      DecodeFrame(Frame({0x68, 0x0E, 0xFA, 0x0F, 0x84, 0x0C, 0x01, 0x01, 0x03,
                         0x00, 0x01, 0x00, 0xE9, 0x03, 0x00, 0x81}));

  const FrameDecodeNode* value = Find(decode, u"Value");
  ASSERT_TRUE(value);
  EXPECT_EQ(value->value, u"ON");

  const FrameDecodeNode* quality = Find(decode, u"Quality");
  ASSERT_TRUE(quality);
  EXPECT_EQ(quality->value, u"0x80 · IV");
}

// IEC 60870-5-101 §7.2.5: with SQ set the frame carries one address followed by
// consecutive elements. Getting this wrong misattributes every value after the
// first, which is exactly the kind of defect the pane is used to find.
TEST(FrameDecodeTest, WalksASequenceOfInformationElements) {
  // M_ME_NB_1 (scaled + QDS, 3 octets each), SQ=1, count=2, from IOA 100.
  const FrameDecode decode = DecodeFrame(Frame({0x68, 0x13, 0xFA, 0x0F, 0x84,
                                                0x0C, 0x0B, 0x82, 0x01, 0x00,
                                                0x01, 0x00, 0x64, 0x00, 0x00,
                                                0x0A, 0x00, 0x00, 0x14, 0x00,
                                                0x00}));

  ASSERT_EQ(decode.nodes.size(), 2u);
  const FrameDecodeNode& asdu = decode.nodes[1];
  ASSERT_EQ(asdu.children.size(), 7u);  // 5 header fields + 2 objects

  EXPECT_EQ(asdu.children[5].value, u"IOA 100");
  EXPECT_EQ(asdu.children[5].children[0].value, u"10");
  // The second element carries no address of its own; it is IOA 100 + 1.
  EXPECT_EQ(asdu.children[6].value, u"IOA 101");
  EXPECT_EQ(asdu.children[6].children[0].value, u"20");
  EXPECT_EQ(asdu.children[6].offset, 18);
}

TEST(FrameDecodeTest, SupervisoryFrameCarriesNoAsdu) {
  const FrameDecode decode =
      DecodeFrame(Frame({0x68, 0x04, 0x01, 0x00, 0x84, 0x0C}));

  ASSERT_EQ(decode.nodes.size(), 1u);
  EXPECT_EQ(decode.summary, u"S-format");
  EXPECT_FALSE(Find(decode, u"N(S) send"));
  const FrameDecodeNode* recv = Find(decode, u"N(R) recv");
  ASSERT_TRUE(recv);
  EXPECT_EQ(recv->value, u"1602");
}

TEST(FrameDecodeTest, NamesTheUnnumberedFunction) {
  const FrameDecode decode =
      DecodeFrame(Frame({0x68, 0x04, 0x43, 0x00, 0x00, 0x00}));

  EXPECT_EQ(decode.summary, u"U-format");
  const FrameDecodeNode* function = Find(decode, u"Function");
  ASSERT_TRUE(function);
  EXPECT_EQ(function->value, u"TESTFR act");
  EXPECT_EQ(function->offset, 2);
}

// A command frame, where the select/execute bit decides whether the device is
// about to move.
TEST(FrameDecodeTest, DecodesASingleCommand) {
  // C_SC_NA_1, IOA 6001, SCO = 0x81: select, state ON.
  const FrameDecode decode =
      DecodeFrame(Frame({0x68, 0x0E, 0xFA, 0x0F, 0x84, 0x0C, 0x2D, 0x01, 0x06,
                         0x00, 0x01, 0x00, 0x71, 0x17, 0x00, 0x81}));

  EXPECT_EQ(decode.summary, u"I-format · C_SC_NA_1");
  const FrameDecodeNode* cause = Find(decode, u"Cause (COT)");
  ASSERT_TRUE(cause);
  EXPECT_EQ(cause->value, u"6 · act");

  const FrameDecodeNode* command = Find(decode, u"Command");
  ASSERT_TRUE(command);
  EXPECT_EQ(command->value, u"1 · select");
}

// The negative-confirmation and test bits change what a frame means, so they
// are never folded into the cause number.
TEST(FrameDecodeTest, MarksNegativeAndTestCauses) {
  scada::DeviceFrame frame = MeasurementFrame();
  frame.raw_data[8] = static_cast<char>(0x47);  // actcon (7) with P/N set

  const FrameDecode decode = DecodeFrame(frame);
  const FrameDecodeNode* cause = Find(decode, u"Cause (COT)");
  ASSERT_TRUE(cause);
  EXPECT_EQ(cause->value, u"7 · actcon · negative");
}

// A type this decoder does not know must not be split into objects by guessed
// element sizes — the octets are shown whole instead.
TEST(FrameDecodeTest, UnknownTypeShowsItsObjectOctetsWhole) {
  scada::DeviceFrame frame = MeasurementFrame();
  frame.raw_data[6] = static_cast<char>(0x7F);  // type 127, not assigned here

  const FrameDecode decode = DecodeFrame(frame);
  ASSERT_EQ(decode.nodes.size(), 2u);
  EXPECT_EQ(decode.nodes[1].value, u"type 127");
  EXPECT_FALSE(Find(decode, u"Object"));

  const FrameDecodeNode* objects = Find(decode, u"Information objects");
  ASSERT_TRUE(objects);
  EXPECT_EQ(objects->value, u"8 bytes");
  EXPECT_EQ(objects->offset, 12);
}

// A capture cut short must decode what it can and stop, not read past the end.
TEST(FrameDecodeTest, TruncatedObjectsStopAtTheLastCompleteOne) {
  // Says two objects; carries the octets for one.
  const FrameDecode decode =
      DecodeFrame(Frame({0x68, 0x0E, 0xFA, 0x0F, 0x84, 0x0C, 0x01, 0x02, 0x03,
                         0x00, 0x01, 0x00, 0xE9, 0x03, 0x00, 0x01}));

  ASSERT_EQ(decode.nodes.size(), 2u);
  const FrameDecodeNode& asdu = decode.nodes[1];
  EXPECT_EQ(asdu.children.size(), 6u);  // 5 header fields + 1 object
  EXPECT_EQ(asdu.children[5].value, u"IOA 1001");
}

// The connection layer also carries IEC 60870-5-101 FT1.2 frames, which start
// with the same 0x68 but repeat the length octet. They are not APDUs and must
// not be decoded as if they were.
TEST(FrameDecodeTest, RejectsANon104Frame) {
  const FrameDecode decode = DecodeFrame(Frame({0x68, 0x09, 0x09, 0x68, 0x53,
                                                0x01, 0x0D, 0x01, 0x03, 0x00,
                                                0x01, 0x00, 0x16}));

  EXPECT_TRUE(decode.nodes.empty());
  EXPECT_FALSE(decode.note.empty());
  // The octets are still shown: a frame the decoder cannot read is precisely
  // the one an engineer wants to see raw.
  EXPECT_EQ(decode.hex, u"68 09 09 68 53 01 0D 01 03 00 01 00 16");
}

TEST(FrameDecodeTest, ReportsWhenNothingWasCaptured) {
  const FrameDecode decode = DecodeFrame(scada::DeviceFrame{});

  EXPECT_TRUE(decode.nodes.empty());
  EXPECT_TRUE(decode.hex.empty());
  EXPECT_FALSE(decode.note.empty());
}

// An I-format APDU with nothing after the control field is legal on the wire
// only as a malformed frame; it must degrade rather than read the ASDU header
// out of bounds.
TEST(FrameDecodeTest, IFormatWithoutAnAsduDecodesTheApciAlone) {
  const FrameDecode decode =
      DecodeFrame(Frame({0x68, 0x04, 0xFA, 0x0F, 0x84, 0x0C}));

  ASSERT_EQ(decode.nodes.size(), 1u);
  EXPECT_EQ(decode.summary, u"I-format");
}

// A CP56Time2a tag is compared against the row's own arrival time to find a
// device with a wrong clock, so it is rendered as a time, not as seven octets.
TEST(FrameDecodeTest, RendersACp56TimeTag) {
  // M_SP_TB_1, IOA 1001, SIQ=0x00, then 2026-07-26 21:53:58.402.
  const FrameDecode decode = DecodeFrame(Frame(
      {0x68, 0x15, 0xFA, 0x0F, 0x84, 0x0C, 0x1E, 0x01, 0x03, 0x00, 0x01, 0x00,
       0xE9, 0x03, 0x00, 0x00, 0x22, 0xE4, 0x35, 0x15, 0x1A, 0x07, 0x1A}));

  const FrameDecodeNode* time = Find(decode, u"Time");
  ASSERT_TRUE(time);
  EXPECT_EQ(time->value, u"26-07-26 21:53:58.402");
  EXPECT_EQ(time->offset, 16);
  EXPECT_EQ(time->length, 7);
}


// The decoder reports the addresses it walked so the view can resolve them; it
// has no address space of its own.
TEST(FrameDecodeTest, ReportsTheObjectAddressesItWalked) {
  EXPECT_EQ(DecodeFrame(MeasurementFrame()).object_addresses,
            (std::vector<scada::Int32>{4002}));

  // A sequence: one address on the wire, two objects.
  const FrameDecode sequence = DecodeFrame(Frame({0x68, 0x13, 0xFA, 0x0F, 0x84,
                                                  0x0C, 0x0B, 0x82, 0x01, 0x00,
                                                  0x01, 0x00, 0x64, 0x00, 0x00,
                                                  0x0A, 0x00, 0x00, 0x14, 0x00,
                                                  0x00}));
  EXPECT_EQ(sequence.object_addresses, (std::vector<scada::Int32>{100, 101}));

  // S-format carries no objects at all.
  EXPECT_TRUE(
      DecodeFrame(Frame({0x68, 0x04, 0x01, 0x00, 0x84, 0x0C})).object_addresses
          .empty());
}

TEST(FrameDecodeTest, NamesTheNodeAnObjectMapsTo) {
  FrameDecode decode = DecodeFrame(MeasurementFrame());
  const FrameObjectMapping mappings[] = {
      {.object_address = 4001, .signal = u"ESTRA.I", .node_id = u"ns=2;s=I"},
      {.object_address = 4002, .signal = u"ESTRA.P", .node_id = u"ns=2;s=P"}};

  AppendMappedNodes(decode, mappings);

  const FrameDecodeNode& group = decode.nodes.back();
  EXPECT_EQ(group.name, u"Mapped node");
  ASSERT_EQ(group.children.size(), 1u);
  EXPECT_EQ(group.children[0].name, u"ESTRA.P");
  EXPECT_EQ(group.children[0].value, u"ns=2;s=P");
}

// An address the device reports that the configuration does not know is worth
// seeing, so it is listed rather than dropped.
TEST(FrameDecodeTest, ListsAnObjectThatMapsToNothing) {
  FrameDecode decode = DecodeFrame(MeasurementFrame());
  const FrameObjectMapping mappings[] = {
      {.object_address = 4001, .signal = u"ESTRA.I", .node_id = u"ns=2;s=I"}};

  AppendMappedNodes(decode, mappings);

  const FrameDecodeNode& group = decode.nodes.back();
  ASSERT_EQ(group.children.size(), 1u);
  EXPECT_EQ(group.children[0].name, u"IOA 4002");
  EXPECT_FALSE(group.children[0].value.empty());
  EXPECT_EQ(group.children[0].value.find(u"ns="), std::u16string::npos);
}

// A frame with no information objects gets no group at all — an empty "Mapped
// node" heading over an S-format frame would suggest something was missing.
TEST(FrameDecodeTest, NoMappedNodeGroupWithoutObjects) {
  FrameDecode decode = DecodeFrame(Frame({0x68, 0x04, 0x01, 0x00, 0x84, 0x0C}));
  const std::size_t before = decode.nodes.size();

  AppendMappedNodes(decode, {});

  EXPECT_EQ(decode.nodes.size(), before);
}

}  // namespace
