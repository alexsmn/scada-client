#pragma once

#include "profile/page.h"

// Builds the operator Overview page — the reshell's landing cockpit: a dominant
// trend (Graph) and the active-alarm table (the EventJournal in "Current" mode,
// which shows only unacknowledged/actionable alarms). The severity KPI strip is
// provided ambiently by the context bar's tiles, so the page itself carries the
// trend and the alarm surface. Constructed in code (Qt-free) so it can be
// opened without a saved profile and unit-tested for its structure.
Page MakeOverviewPage();
