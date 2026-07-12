#pragma once

// Transitional compatibility shim for the `screenshot_generator` ->
// `scada::screenshot_generator` migration. The screenshot-generator tool's
// helpers now live in the nested namespace; this using-directive keeps the
// tool's own unqualified `screenshot_generator::` call sites compiling.
// Remove once they are requalified to `scada::screenshot_generator::`.

namespace scada::screenshot_generator {}

namespace screenshot_generator {
using namespace scada::screenshot_generator;
}  // namespace screenshot_generator
