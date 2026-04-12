#include "dialog_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"

#include "base/console_logger.h"
#include "components/login/login_dialog.h"
#include "scada/data_services_factory.h"
#include "scada/logging.h"

#include <QApplication>
#include <QDialog>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <transport/transport_factory.h>

#include <gtest/gtest.h>

namespace {

// Dummy TransportFactory that never produces a transport. LoginDialog
// & friends hold a `TransportFactory&` inside their DataServicesContext
// but only touch it on a real login attempt — we reject before that.
class NullTransportFactory : public transport::TransportFactory {
 public:
  transport::expected<transport::any_transport> CreateTransport(
      const transport::TransportString&,
      const transport::executor&,
      const transport::log_source&) override {
    return transport::ERR_NOT_IMPLEMENTED;
  }
};

// Scans top-level widgets for a visible QDialog, resizes it to the
// spec dims, grabs a pixmap, and rejects the dialog so whoever called
// `show()` can finish the cleanup path (deleteLater in most factories).
bool GrabAndCloseVisibleDialog(const DialogSpec& spec) {
  QDialog* dialog = nullptr;
  for (QWidget* w : QApplication::topLevelWidgets()) {
    if (w->isVisible()) {
      if (auto* d = qobject_cast<QDialog*>(w)) {
        dialog = d;
        break;
      }
    }
  }
  if (!dialog)
    return false;

  if (spec.width > 0 && spec.height > 0) {
    dialog->resize(spec.width, spec.height);
    QApplication::processEvents();
  }
  dialog->repaint();
  QApplication::processEvents();

  QPixmap pixmap = dialog->grab();
  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));

  dialog->reject();
  QApplication::processEvents();
  return true;
}

// --- Per-kind builders. Each invokes the component's public factory
// (which calls show() internally) and returns; the generic
// GrabAndCloseVisibleDialog then picks the dialog up from the
// top-level-widgets list.
//
// Adding a new dialog = add a new `Build*Dialog` here and a new arm in
// the `Dispatch` switch below.

void BuildLoginDialog(DialogEnvironment& env,
                      NullTransportFactory& transport_factory,
                      const std::shared_ptr<Logger>& logger) {
  DataServicesContext services_context{logger, env.executor,
                                       transport_factory,
                                       scada::ServiceLogParams{}};
  // Returned promise is intentionally discarded — it resolves when the
  // user dismisses, which GrabAndCloseVisibleDialog does via reject().
  [[maybe_unused]] auto p =
      ExecuteLoginDialog(env.executor, std::move(services_context));
  QApplication::processEvents();
}

}  // namespace

bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env) {
  // Per-call stubs — cheap to construct, no shared state.
  NullTransportFactory transport_factory;
  auto logger = std::make_shared<ConsoleLogger>();

  if (spec.kind == "login") {
    BuildLoginDialog(env, transport_factory, logger);
  } else {
    ADD_FAILURE() << "Unknown dialog kind: " << spec.kind;
    return false;
  }

  if (!GrabAndCloseVisibleDialog(spec)) {
    ADD_FAILURE() << "No visible dialog for kind: " << spec.kind;
    return false;
  }
  return true;
}
