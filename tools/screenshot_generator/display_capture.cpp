#include "display_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "common/vds_runtime_api.h"
#include "display_frame/qt/display_frame.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QApplication>
#include <QPixmap>
#include <QString>

#include <filesystem>
#include <memory>
#include <string>

namespace {

// The display document to render, resolved from the manifest's `display.path`
// (relative to the fixture directory, next to screenshot_data.json).
std::filesystem::path FixturePath(const boost::json::value& json) {
  std::string rel = "fixtures/substation.vds";
  if (const auto* display = json.as_object().if_contains("display")) {
    if (const auto* path = display->as_object().if_contains("path"))
      rel = std::string(path->as_string());
  }
  std::filesystem::path result{rel};
  if (result.is_relative())
    result = GetDataFilePath().parent_path() / result;
  return result;
}

}  // namespace

void SaveDisplayScreenshot(const ScreenshotSpec& spec,
                           const boost::json::value& json) {
  // The VDS runtime dylib is loaded from the client install dir. The generator
  // never sets base::DIR_EXE, so point client::DIR_INSTALL at the binary dir
  // (where the dylib is co-located) so the renderer resolves it and paints the
  // real document instead of its "runtime unavailable" placeholder.
  base::PathService::Override(
      client::DIR_INSTALL,
      std::filesystem::path{QApplication::applicationDirPath().toStdString()});

  // The DisplayFrame reparents (owns) the renderer, so the frame is the single
  // owning widget we render and delete.
  auto* diagram = new VdsRuntimeWidget;
  diagram->Open(FixturePath(json), TC_VDS_RUNTIME_DOCUMENT_KIND_AUTO);

  // The standalone capture renders the chrome + diagram only: the bay strips
  // need live app services (a running session), which the offscreen generator
  // does not stand up here. The strips are covered by client_display_frame's
  // widget tests instead.
  QWidget* frame =
      WrapDisplayInFrame(diagram, diagram->title(), DisplayFrameContext{});
  std::unique_ptr<QWidget> owner{frame};

  frame->setFixedSize(spec.width, spec.height);
  frame->show();
  QApplication::processEvents();

  QPixmap pixmap = frame->grab();
  auto output_path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(output_path.string()));
}
