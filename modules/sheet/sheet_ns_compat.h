#pragma once

// Transitional compatibility shim for the `ui` -> `scada::ui` migration.
// The sheet module's `ui` namespace was nested under `scada::`; this
// using-directive keeps unqualified `ui::` call sites compiling until they are
// requalified to `scada::ui::`. Included from every header that declares an
// `scada::ui` type so consumers see the alias transitively.

namespace scada::ui {}

namespace ui {
using namespace scada::ui;
}  // namespace ui
