#pragma once

#include <QPixmap>

#include <string>
#include <string_view>

class CapturePublishGuard;
class QDialog;
struct DialogSpec;

namespace scada::screenshot_generator {

// The first visible top-level QDialog, or null when there is none.
//
// Every capture in this tool reaches its dialog the same way: the component
// factories call show() themselves and hand back nothing, so the dialog has to
// be recovered from QApplication::topLevelWidgets(). One helper rather than a
// loop per caller — there were three, and they had already drifted apart in
// how they reported a miss.
QDialog* FindVisibleDialog();

// Grabs `dialog` with the drop-down of the combo box named `combo_object`
// open, composing the two into one pixmap.
//
// Qt gives a combo popup its own top-level window (`view()->window()`, a
// Qt::Popup frame), so `dialog->grab()` renders the widget tree with the list
// closed however the combo was opened — calling showPopup() before the grab
// is not enough, and that is why the login capture shipped a closed combo.
// The composed image is the union of the two windows' geometries, which means
// it is taller than the spec's `height` whenever the list hangs below the
// dialog, exactly as the manual's login image shows it.
QPixmap GrabDialogWithComboPopupOpen(QDialog* dialog,
                                     const std::string& combo_object);

// Scans top-level widgets for a visible QDialog, resizes it to the spec dims,
// grabs a pixmap, and rejects the dialog so whoever called `show()` can finish
// the cleanup path (deleteLater in most factories).
bool GrabAndCloseVisibleDialog(const DialogSpec& spec);

// GrabAndCloseVisibleDialog, plus the publish guard and the diagnostic every
// caller wants on a miss.
//
// The guard is checked here rather than at each of the per-kind branches: this
// is the one funnel every dialog capture passes through on its way to a save.
bool GrabAndCloseVisibleDialogOrReport(const DialogSpec& spec,
                                       const CapturePublishGuard& guard);

// Selects the combo entry whose text contains `needle`, for a dialog whose
// default selection is a collation accident rather than a choice. Reports and
// returns false when there is no such entry, so a renamed or re-parented
// fixture device fails the capture instead of quietly documenting another one.
bool SelectDeviceInDialogCombo(const DialogSpec& spec,
                               std::u16string_view needle);

// Fails when the dialog waiting to be grabbed shows an empty list.
//
// An empty list is the failure mode this generator cannot see by itself: the
// capture still writes a well-formed PNG of exactly the spec's dimensions, so
// `check_screenshots.py` passes and the blank image ships — which is how
// limits.png shipped with every field empty. The service-object dialog carries
// the same risk in a sharper form. Its component list is the selected device's
// HasComponent data variables, and the fixture has to state those references
// explicitly (`references` in screenshot_data.json); a device wired only by
// Organizes, which is the fixture's default, yields a populated device combo
// above a completely empty list.
bool ReportIfDialogListEmpty(const DialogSpec& spec);

}  // namespace scada::screenshot_generator
