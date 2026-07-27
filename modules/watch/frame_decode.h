#pragma once

#include "scada/event.h"

#include <string>
#include <vector>

// One node of the decoded-frame tree: a labelled field or group, the value as
// it should be shown, and the byte range of the captured frame it was read
// from.
struct FrameDecodeNode {
  std::u16string name;
  std::u16string value;
  // Byte range within the raw frame. `length == 0` marks a node with no octets
  // of its own — a grouping node, or a field derived from bits already shown.
  int offset = 0;
  int length = 0;
  std::vector<FrameDecodeNode> children;
};

// The decode of one captured frame: what the pane shows beside the trace.
struct FrameDecode {
  // One-line summary for the pane header, e.g. u"I-format · M_ME_NC_1".
  std::u16string summary;
  // Every captured octet, space-separated uppercase hex.
  std::u16string hex;
  // APCI, ASDU and the information objects. Empty when nothing was decodable.
  std::vector<FrameDecodeNode> nodes;
  // Why `nodes` is empty, when it is — no octets captured, or octets that are
  // not an IEC 60870-5-104 APDU. Empty when `nodes` is populated.
  std::u16string note;
};

// Decodes a captured frame's octets into the field tree the decode pane shows.
//
// This reads the frame's own bytes rather than trusting the summary fields the
// server already extracted, because byte offsets are the point of the pane: an
// engineer opens it to see which octet carried which field. The server-side
// fields are used only for the header summary.
//
// Deliberately tolerant. Anything it cannot make sense of — a driver that is
// not IEC 60870-5-104, a truncated capture, an unknown type identifier —
// degrades to showing the raw octets rather than failing or guessing. Nothing
// here drives behaviour; a mislabelled field is a cosmetic defect, never a
// control one.
FrameDecode DecodeFrame(const scada::DeviceFrame& frame);
