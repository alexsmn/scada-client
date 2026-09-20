#include "dialog_capture.h"

#include "dialog_kinds.h"
#include "null_task_manager.h"
#include "publish_guard.h"
#include "screenshot_config.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "base/boost_log.h"
#include "scada/logging.h"
#include "scada/node_id.h"

#include <QCoreApplication>
#include <gtest/gtest.h>

#include <memory>
#include <string_view>

namespace {

using scada::screenshot_generator::DialogCaptureContext;
using scada::screenshot_generator::NullTransportFactory;

// One dialog capture: the `kind` key a fixture spec selects it by, and the
// function that renders it.
struct DialogCapture {
  std::string_view kind;
  bool (*capture)(const DialogCaptureContext&);
};

// The `kind` keys, in one place — the dialog-side twin of `kStandaloneCaptures`
// in main.cpp, and a table for the same reason that one is. This was sixteen
// hand-written `else if` arms, each ending in its own copy of the grab-and-wait
// tail; a table cannot lose a line of that tail, because no entry carries it.
constexpr DialogCapture kDialogCaptures[] = {
    {"login", &scada::screenshot_generator::CaptureLoginDialog},
    {"limits", &scada::screenshot_generator::CaptureLimitsDialog},
    {"write-manual", &scada::screenshot_generator::CaptureManualWriteDialog},
    {"write-remote", &scada::screenshot_generator::CaptureRemoteWriteDialog},
    {"control-confirm",
     &scada::screenshot_generator::CaptureControlConfirmationDialog},
    {"command-palette",
     &scada::screenshot_generator::CaptureCommandPaletteDialog},
    {"about", &scada::screenshot_generator::CaptureAboutDialog},
    {"change-password",
     &scada::screenshot_generator::CaptureChangePasswordDialog},
    {"time-range", &scada::screenshot_generator::CaptureTimeRangeDialog},
    {"message-box", &scada::screenshot_generator::CaptureMessageBoxDialog},
    {"multi-create", &scada::screenshot_generator::CaptureMultiCreateDialog},
    {"csv-export", &scada::screenshot_generator::CaptureCsvExportDialog},
    {"add-favourites",
     &scada::screenshot_generator::CaptureAddFavouritesDialog},
    {"transport", &scada::screenshot_generator::CaptureTransportDialog},
    {"create-service-item",
     &scada::screenshot_generator::CaptureCreateServiceItemDialog},
};

// Resolves a spec's `kind`. Null means the key is not one this generator knows,
// which is a fixture error rather than a skip.
const DialogCapture* FindDialogCapture(std::string_view kind) {
  for (const DialogCapture& entry : kDialogCaptures) {
    if (entry.kind == kind)
      return &entry;
  }
  return nullptr;
}

}  // namespace

bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env) {
  // Built before the per-kind setup below, so it precedes every assertion those
  // captures make. One guard per dialog, not per sweep: CaptureDialogs runs
  // this for every spec in the fixture, and a test-wide check would let the
  // first bad dialog suppress the rest of the gallery.
  CapturePublishGuard publish_guard{spec.filename};

  // Everything below grabs a QDialog out of `QApplication::topLevelWidgets()`
  // and closes it with `reject()`, so a dialog that the platform theme takes
  // over is both uncapturable and undismissable. The fixture pins
  // AA_DontUseNativeDialogs for exactly that reason (see SetUpTestSuite in
  // screenshot_fixture.h); say so here rather than leaving the next person with
  // a hung or segfaulting run to diagnose.
  if (!QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)) {
    ADD_FAILURE() << "AA_DontUseNativeDialogs is not set: QMessageBox and "
                     "QFileDialog would become native modals the capture can "
                     "neither grab nor close (kind: "
                  << spec.kind << ")";
    return false;
  }

  const DialogCapture* entry = FindDialogCapture(spec.kind);
  if (!entry) {
    ADD_FAILURE() << "Unknown dialog kind: " << spec.kind;
    return false;
  }

  // Per-call stubs — cheap to construct, no shared state. They outlive the
  // capture only by the width of this scope, which is why every coroutine-driven
  // kind waits for its awaitable to unwind before returning.
  NullTransportFactory transport_factory;
  NullTaskManager task_manager;
  DialogServiceImplQt dialog_service;  // parent_widget = nullptr

  return entry->capture(DialogCaptureContext{
      .spec = spec,
      .env = env,
      .dialog_service = dialog_service,
      .task_manager = task_manager,
      .transport_factory = transport_factory,
      .logger = std::make_shared<BoostLogger>(LOG_NAME("Screenshot")),
      .publish_guard = publish_guard,
      // A spec may name its own target node; otherwise every node-driven kind
      // shares the fixture-wide one.
      .node_id = spec.node_id.is_null() ? env.dialog_analog_node_id
                                        : spec.node_id,
  });
}
