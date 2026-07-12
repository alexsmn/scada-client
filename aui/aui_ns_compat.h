#pragma once

// Transitional compatibility shim for the `aui` -> `scada::aui` migration.
//
// The client's abstract-UI namespace `aui` was nested under `scada::` (see the
// core/common/server scada:: namespace initiative). This using-directive lets
// the ~hundreds of unqualified `aui::` call sites — including typedefs and code
// at global scope — keep compiling without a repo-wide requalification. Any
// header that declares or forward-declares an `scada::aui` type includes this
// shim, so consumers that pull such a header transitively see the alias.
//
// Remove once all `aui::` call sites have been requalified to `scada::aui::`.

namespace scada::aui {}

namespace aui {
using namespace scada::aui;
}  // namespace aui
