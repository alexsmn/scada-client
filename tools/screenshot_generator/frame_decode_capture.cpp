#include "frame_decode_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "aui/translation.h"
#include "modules/watch/qt/frame_decode_pane.h"

#include <memory>

namespace {

// A measured-value frame, the most common thing on a -104 link and the one the
// mockup shows: I-format with N(S)=2045 / N(R)=1602, carrying M_ME_NC_1 (short
// float) 130.5 for IOA 4002 with good quality, spontaneous.
//
//   68 12 | FA 0F 84 0C | 0D 01 03 00 01 00 | A2 0F 00 | 00 80 02 43 | 00
//
// Chosen so the capture exercises every group the pane can draw — APCI, ASDU
// header, information object with a value and a quality — rather than the
// degenerate S-format case.
scada::DeviceFrame FixtureFrame() {
  static constexpr int kOctets[] = {0x68, 0x12, 0xFA, 0x0F, 0x84, 0x0C, 0x0D,
                                    0x01, 0x03, 0x00, 0x01, 0x00, 0xA2, 0x0F,
                                    0x00, 0x00, 0x80, 0x02, 0x43, 0x00};
  scada::DeviceFrame frame;
  frame.direction = scada::DeviceFrame::kInbound;
  for (int octet : kOctets)
    frame.raw_data.push_back(static_cast<char>(octet));
  return frame;
}

}  // namespace

void SaveFrameDecodeScreenshot(const ScreenshotSpec& spec) {
  const scada::DeviceFrame frame = FixtureFrame();

  // The header WatchView builds for a selected row: direction, arrival time,
  // captured size. Composed here because the pane is being shown outside the
  // trace that would otherwise supply it.
  const std::u16string header = Translate("RX") + u" · 21:53:58.402 · " +
                                u"20 " + Translate("bytes");

  auto pane = std::make_unique<FrameDecodePane>();
  pane->ShowFrame(header, frame);
  SaveScreenshot(pane.get(), spec);
}
