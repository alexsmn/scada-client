#include "widget_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"

#include <QApplication>
#include <QPixmap>
#include <QString>
#include <QWidget>

void SaveScreenshot(QWidget* widget, const ScreenshotSpec& spec) {
  if (!widget)
    return;

  widget->resize(spec.width, spec.height);
  QApplication::processEvents();
  widget->repaint();
  QApplication::processEvents();

  QPixmap pixmap = widget->grab();
  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));
}
