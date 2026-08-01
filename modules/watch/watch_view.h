#pragma once

#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/time_model.h"
#include "export/export_model.h"
#include "modules/watch/watch_menu_model.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <memory>

namespace scada {
struct DeviceFrame;
}

namespace scada::aui {
class Table;
}

class FrameDecodePane;
class WatchModel;

class WatchView : protected ControllerContext,
                  public Controller,
                  public TimeModel,
                  public ExportModel {
 public:
  explicit WatchView(const ControllerContext& context);
  virtual ~WatchView();

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual TimeModel* GetTimeModel() override { return this; }
  virtual ExportModel* GetExportModel() override { return this; }

  // TimeModel
  virtual scada::RelativeTimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const scada::RelativeTimeRange& time_range) override;

  // ExportModel
  virtual ExportData GetExportData() override;

 private:
  std::u16string MakeTitle() const;

  // Arms or disarms the device's frame capture: writes its FrameCapture
  // variable and updates the status-strip registry. Both halves matter — the
  // write is what stops the device raising an event per frame, the registry is
  // what stops the operator forgetting they left it running.
  void SetCaptureArmed(bool armed);

#if defined(UI_QT)
  // Builds the trace + decode-pane layout and wires `refresh_decode_pane_`.
  // Qt-only: aui has no cross-platform splitter, so the Wt frontend keeps the
  // bare trace (the same split the other composed views make).
  std::unique_ptr<UiView> CreateFrameTraceLayout();

  std::u16string MakeDecodeHeader(int row,
                                  const scada::DeviceFrame& frame) const;

  // Reads the device's IOA → node mapping into `pane`, once. Deferred to the
  // first entry into the frame trace: it browses every transmission item of
  // the device, which is wasted work for the many sessions that only ever read
  // the log.
  void EnsureAddressMap(FrameDecodePane* pane);

#endif

  void ToggleFrameTrace();

  void SaveLog();

  void OnItemsAdded(int first, int count);

  const std::shared_ptr<WatchModel> model_;

  bool auto_scroll_ = false;

  scada::aui::Table* table_ = nullptr;

  // Selection, mode and clear all route through this, so no call site needs to
  // know whether a decode pane was built. It stays a no-op on the Wt frontend,
  // which keeps the bare trace.
  std::function<void()> refresh_decode_pane_ = [] {};

  bool address_map_requested_ = false;

  // What we last asked the server for, so teardown only disarms what it armed.
  bool capture_armed_ = false;

  CommandRegistry command_registry_;

  // Cross-platform context menu, backed by `command_registry_`. Declared after
  // it so the registry outlives the menu's delegate.
  WatchMenuModel watch_menu_model_{command_registry_};

  boost::signals2::scoped_connection items_added_connection_;
};
