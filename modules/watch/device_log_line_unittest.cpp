#include "modules/watch/device_log_line.h"

#include <gtest/gtest.h>

namespace {

// The markers the drivers actually write, quoted from the IEC 60870 tier so a
// change on that side shows up here rather than as a silently empty trace.
// Raw frame:      iec60870_connection_state.cpp   "#RX: {} ({} bytes)"
// Decoded object: iec60870_device_state.cpp       "#RX: {} [dev: ...]"
// Outbound:       iec60870_device_state_client.cpp "$TX: Synchronizing clock"
TEST(DeviceLogLineTest, ClassifiesInboundTraffic) {
  const DeviceLogLine line =
      ClassifyDeviceLogLine(u"#RX: 68 0C FA 0F 04 0C (14 bytes)");
  EXPECT_EQ(line.direction, DeviceLogDirection::kInbound);
  // The marker is chrome and is stripped; the rest is untouched.
  EXPECT_EQ(line.text, u"RX: 68 0C FA 0F 04 0C (14 bytes)");
  EXPECT_TRUE(IsProtocolTraffic(line));
}

TEST(DeviceLogLineTest, ClassifiesOutboundTraffic) {
  const DeviceLogLine line =
      ClassifyDeviceLogLine(u"$TX: Synchronizing clock to 2026-07-27");
  EXPECT_EQ(line.direction, DeviceLogDirection::kOutbound);
  EXPECT_EQ(line.text, u"TX: Synchronizing clock to 2026-07-27");
  EXPECT_TRUE(IsProtocolTraffic(line));
}

// The reason the mode exists: on a busy link the traffic is the signal and the
// status lines are noise, so unmarked lines must not be mistaken for frames.
TEST(DeviceLogLineTest, PlainLogLinesAreNotTraffic) {
  for (std::u16string_view message :
       {u"Transmission completed", u"Message is too short",
        u"Subscription interrupted."}) {
    const DeviceLogLine line = ClassifyDeviceLogLine(message);
    EXPECT_EQ(line.direction, DeviceLogDirection::kNone) << "for " << message.size();
    // An unmarked line is returned whole — nothing is trimmed off the front.
    EXPECT_EQ(line.text, message);
    EXPECT_FALSE(IsProtocolTraffic(line));
  }
}

TEST(DeviceLogLineTest, HandlesEmptyAndMarkerOnlyMessages) {
  const DeviceLogLine empty = ClassifyDeviceLogLine(u"");
  EXPECT_EQ(empty.direction, DeviceLogDirection::kNone);
  EXPECT_TRUE(empty.text.empty());

  // A bare marker is still a direction, with nothing left to show.
  const DeviceLogLine marker_only = ClassifyDeviceLogLine(u"#");
  EXPECT_EQ(marker_only.direction, DeviceLogDirection::kInbound);
  EXPECT_TRUE(marker_only.text.empty());
}

// A marker only counts at the front. A '#' inside a message — a comment, a
// count, a hex prefix — must not turn that line into a frame.
TEST(DeviceLogLineTest, MarkerCountsOnlyAtTheStart) {
  const DeviceLogLine line =
      ClassifyDeviceLogLine(u"Interrogation group #2 completed");
  EXPECT_EQ(line.direction, DeviceLogDirection::kNone);
  EXPECT_EQ(line.text, u"Interrogation group #2 completed");
}

}  // namespace
