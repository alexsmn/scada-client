#pragma once

#include "events/event_statistics.h"

#include <string>

class QDialog;
class QWidget;

// The event journal's statistics dialog: a snapshot of what the journal shows,
// counted per alarm band and per object, with a Copy button that puts the
// tables on the clipboard as tab-separated text for a spreadsheet.
//
// `period` describes what was counted (the journal's title) and heads both the
// dialog and the copied text. The dialog deletes itself on close.
QDialog* MakeEventStatisticsDialog(const events::EventStatistics& statistics,
                                   std::u16string period,
                                   QWidget* parent);
