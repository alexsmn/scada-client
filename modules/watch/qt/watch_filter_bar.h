#pragma once

#include "modules/watch/watch_model.h"

#include <functional>

class QWidget;

// Builds the device-log filter bar: frame kind, errors-only, and free text,
// as in docs/product/ui-mockups/screens/device-protocol-trace.html.
//
// Stock widgets in their conventional roles, laid out with the style's own
// metrics and no stylesheet (docs/client/ux/principles.md §9).
//
// `on_change` is called with the whole filter whenever any control changes —
// the whole filter, not a delta, so no control can silently drop another's
// state. The returned widget is unparented; give it to a layout.
QWidget* CreateWatchFilterBar(std::function<void(WatchFilter)> on_change);
