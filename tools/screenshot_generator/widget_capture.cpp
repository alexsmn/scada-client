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

  // On-page views are laid-out children (docked / tabbed), so resize() alone is
  // immediately overridden by the parent layout and grab() captures the
  // layout-controlled size rather than the requested spec size — a sparse page
  // (e.g. a --only run with few windows) tiles them narrow. Detach the widget to
  // a top-level for the grab so the requested dimensions stick, then restore its
  // parent so the widget tree is left as we found it.
  QWidget* const parent = widget->parentWidget();
  const bool was_visible = widget->isVisible();
  if (parent)
    widget->setParent(nullptr);

  widget->resize(spec.width, spec.height);
  QApplication::processEvents();
  widget->repaint();
  QApplication::processEvents();

  QPixmap pixmap = widget->grab();

  if (parent) {
    widget->setParent(parent);
    widget->setVisible(was_visible);
  }

  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));
}
