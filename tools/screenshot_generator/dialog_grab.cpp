#include "dialog_grab.h"

#include "dialog_pump.h"
#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_output.h"
#include "widget_capture.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <gtest/gtest.h>

#include <chrono>
#include <sstream>

namespace scada::screenshot_generator {

QDialog* FindVisibleDialog() {
  for (QWidget* widget : QApplication::topLevelWidgets()) {
    if (!widget->isVisible())
      continue;
    if (auto* dialog = qobject_cast<QDialog*>(widget))
      return dialog;
  }
  return nullptr;
}

QPixmap GrabDialogWithComboPopupOpen(QDialog* dialog,
                                     const std::string& combo_object) {
  auto* combo = dialog->findChild<QComboBox*>(
      QString::fromStdString(combo_object), Qt::FindChildrenRecursively);
  if (!combo) {
    ADD_FAILURE() << "expand_combo: no QComboBox named " << combo_object
                  << " in " << dialog->metaObject()->className();
    return GrabWhenSettled(dialog);
  }

  combo->showPopup();
  QWidget* popup = combo->view() ? combo->view()->window() : nullptr;
  PumpEventsUntil([popup] { return popup && popup->isVisible(); },
                  std::chrono::seconds{2});

  QPixmap dialog_pixmap = GrabWhenSettled(dialog);
  if (!popup || !popup->isVisible()) {
    ADD_FAILURE() << "expand_combo: " << combo_object
                  << " popup never became visible";
    return dialog_pixmap;
  }

  const QPixmap popup_pixmap = GrabWhenSettled(popup);
  const QRect dialog_rect{dialog->mapToGlobal(QPoint{0, 0}), dialog->size()};
  const QRect popup_rect{popup->mapToGlobal(QPoint{0, 0}), popup->size()};
  const QRect bounds = dialog_rect.united(popup_rect);

  const qreal ratio = dialog_pixmap.devicePixelRatio();
  QPixmap composed{QSize{static_cast<int>(bounds.width() * ratio),
                         static_cast<int>(bounds.height() * ratio)}};
  composed.setDevicePixelRatio(ratio);
  // The union can leave uncovered corners when the popup is wider than the
  // dialog (or vice versa); fill rather than leave them undefined.
  composed.fill(dialog->palette().color(dialog->backgroundRole()));
  {
    QPainter painter{&composed};
    painter.drawPixmap(dialog_rect.topLeft() - bounds.topLeft(), dialog_pixmap);
    painter.drawPixmap(popup_rect.topLeft() - bounds.topLeft(), popup_pixmap);
  }

  // Leave the combo closed: the popup is a top-level widget, and the next
  // capture's scan for a visible dialog walks that same list.
  combo->hidePopup();
  QApplication::processEvents();
  return composed;
}

bool GrabAndCloseVisibleDialog(const DialogSpec& spec) {
  QDialog* dialog = nullptr;
  // The dialog factories are coroutines CoSpawn'd onto MessageLoopQt; pump
  // (see PumpEventsUntil) until the initial resume shows the dialog.
  if (!PumpEventsUntil(
          [&dialog] { return (dialog = FindVisibleDialog()) != nullptr; },
          std::chrono::seconds{2})) {
    return false;
  }

  if (spec.width > 0 && spec.height > 0) {
    dialog->resize(spec.width, spec.height);
    QApplication::processEvents();
  }
  dialog->repaint();
  QApplication::processEvents();

  QPixmap pixmap = spec.expand_combo.empty() ? GrabWhenSettled(dialog)
                                             : GrabDialogWithComboPopupOpen(
                                                   dialog, spec.expand_combo);
  auto path = OutputPathFor(spec.filename);
  pixmap.save(QString::fromStdString(path.string()));

  // LoginDialog::reject() now calls QDialog::reject() before resolving its
  // completion, so it hides the dialog itself — it must, or a cancel issued
  // while the dialog holds an AppKit modal session loses the quit that ends
  // the app. The explicit hide() is kept as belt-and-braces: what this scan
  // needs is isVisible() cleared before the next capture's
  // top-level-widgets sweep, and it should not silently depend on the
  // dialog's own cancel path to get it.
  dialog->hide();
  dialog->reject();
  for (int i = 0; i < 3; ++i)
    QApplication::processEvents();
  return true;
}

bool GrabAndCloseVisibleDialogOrReport(const DialogSpec& spec,
                                       const CapturePublishGuard& guard) {
  if (!guard.ShouldPublish())
    return false;
  if (GrabAndCloseVisibleDialog(spec))
    return true;
  std::ostringstream widgets;
  for (QWidget* w : QApplication::topLevelWidgets()) {
    widgets << " | " << w->metaObject()->className()
            << (w->isVisible() ? " (visible)" : " (hidden)");
  }
  ADD_FAILURE() << "No visible dialog for kind: " << spec.kind
                << " | top-level widgets:" << widgets.str();
  return false;
}

bool SelectDeviceInDialogCombo(const DialogSpec& spec,
                               std::u16string_view needle) {
  QDialog* dialog = FindVisibleDialog();
  if (!dialog) {
    ADD_FAILURE() << "No visible dialog to select a device in, kind: "
                  << spec.kind;
    return false;
  }

  auto* combo =
      dialog->findChild<QComboBox*>(QString{}, Qt::FindChildrenRecursively);
  if (!combo) {
    ADD_FAILURE() << "No QComboBox in " << dialog->metaObject()->className()
                  << " for kind: " << spec.kind;
    return false;
  }

  const QString text = QString::fromStdU16String(std::u16string{needle});
  const int index = combo->findText(text, Qt::MatchContains);
  if (index < 0) {
    QStringList seen;
    for (int i = 0; i < combo->count(); ++i)
      seen << combo->itemText(i);
    ADD_FAILURE() << "No combo entry containing " << text.toStdString()
                  << " for kind: " << spec.kind << " | entries="
                  << seen.join(QLatin1String(", ")).toStdString();
    return false;
  }
  combo->setCurrentIndex(index);
  // ...and say so the way a click does. The dialog listens on
  // QComboBox::activated, which Qt emits only for user interaction, so
  // setCurrentIndex alone moves the combo's text and leaves the list below it
  // showing the previous device's components — a capture that contradicts
  // itself in one image.
  emit combo->activated(index);
  QApplication::processEvents();
  return true;
}

bool ReportIfDialogListEmpty(const DialogSpec& spec) {
  QDialog* dialog = FindVisibleDialog();
  if (!dialog) {
    ADD_FAILURE() << "No visible dialog to check the list of, kind: "
                  << spec.kind;
    return false;
  }

  auto* list =
      dialog->findChild<QListWidget*>(QString{}, Qt::FindChildrenRecursively);
  if (!list) {
    ADD_FAILURE() << "No QListWidget in " << dialog->metaObject()->className()
                  << " for kind: " << spec.kind;
    return false;
  }
  if (list->count() == 0) {
    ADD_FAILURE() << "Empty list in " << dialog->metaObject()->className()
                  << " for kind: " << spec.kind
                  << " - the capture would ship a blank dialog";
    return false;
  }
  return true;
}

}  // namespace scada::screenshot_generator
