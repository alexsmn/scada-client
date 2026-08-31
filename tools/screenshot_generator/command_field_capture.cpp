#include "command_field_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "main_window/command_field_qt.h"

#include <QKeySequence>

#include <memory>

void SaveCommandFieldScreenshot(const ScreenshotSpec& spec) {
  // The same prompt and shortcut MainWindow::CreateContextBar() builds it with,
  // so the capture cannot drift from the shipped field.
  auto field = std::make_unique<CommandField>(
      nullptr,
      QString::fromStdU16String(Translate("Search tags, objects, commands…")),
      QKeySequence{Qt::CTRL | Qt::Key_K}, CommandField::ActivateCallback{});

  SaveScreenshot(field.get(), spec);
}
