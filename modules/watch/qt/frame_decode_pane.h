#pragma once

#include "modules/watch/frame_decode.h"

#include <QWidget>

#include <memory>
#include <optional>
#include <vector>

class FrameDecodeTreeModel;
class QLabel;

namespace scada::aui {
class Tree;
}

// The decode pane beside the frame trace: a header line naming the selected
// frame, its raw octets, and the decoded field tree with byte offsets — the
// right-hand inspector of docs/product/ui-mockups/screens/device-protocol-trace.html.
//
// A widget rather than a few lines inside WatchView so that the headless
// screenshot generator can build and render the real pane; WatchView itself
// needs a full ControllerContext, which the generator has no way to assemble.
class FrameDecodePane : public QWidget {
 public:
  explicit FrameDecodePane(QWidget* parent = nullptr);
  ~FrameDecodePane();

  // Decodes `frame` and shows it under `header`.
  void ShowFrame(const std::u16string& header, const scada::DeviceFrame& frame);

  // The empty state: no row selected, or a selected row that is not a frame.
  void Clear();

  // Supplies the device's IOA → address-space mapping, read asynchronously by
  // the view. Call it even when the device has none: until it arrives the pane
  // says nothing about mapping, rather than reporting every object as
  // unmapped. Re-renders the frame currently on screen.
  void SetAddressMap(std::vector<FrameObjectMapping> mappings);

 private:
  // Installs `decode` in the tree and re-applies the view state a model reset
  // discards.
  void ShowDecode(const FrameDecode& decode);

  const std::shared_ptr<FrameDecodeTreeModel> model_;

  // The frame on screen, kept so a late-arriving address map can re-render it.
  std::optional<scada::DeviceFrame> frame_;
  std::vector<FrameObjectMapping> mappings_;
  bool address_map_loaded_ = false;

  QLabel* header_ = nullptr;
  QLabel* octets_ = nullptr;
  scada::aui::Tree* tree_ = nullptr;
};
