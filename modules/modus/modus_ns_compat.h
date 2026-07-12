#pragma once

// Transitional compatibility shim for the `modus` -> `scada::modus` migration.
//
// The client's Modus (scheme rendering / ActiveX bridge) namespace `modus` was
// nested under `scada::`. This using-directive keeps unqualified `modus::` call
// sites — including code at global scope — compiling without a repo-wide
// requalification. Any header that declares or forward-declares a
// `scada::modus` type includes this shim, so consumers see the alias
// transitively. Remove once all `modus::` call sites use `scada::modus::`.
//
// NB: does not touch the external COM interface namespaces (SDECore, htsde2)
// used by the ActiveX bridge — those stay in global scope.

namespace scada::modus {}

namespace modus {
using namespace scada::modus;
}  // namespace modus
