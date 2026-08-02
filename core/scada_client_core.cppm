// scada.client.core — named C++20 module facade over the client/core headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): the headers stay the source of truth, the global
// module fragment includes them, the purview re-exports the public names with
// `export using`. `export import scada.base;` mirrors client_core's PUBLIC
// link on scada_base.
//
// UI-config requirement: node_command_context.h (and through it
// default_node_command_registry.h) includes aui/key_codes.h, whose contents
// exist only under UI_QT or UI_WT (aui::KeyModifiers is Qt::KeyboardModifiers
// under UI_QT). The facade target must therefore be compiled with the UI
// config macro visible — link aui_qt (which publishes UI_QT) or aui_wt —
// exactly like client_core's UI consumers; a bare scada_base-only target
// cannot compile this GMF.
//
// Deliberate exclusions (include the header textually where needed):
//  - Names owned by other libraries that these headers merely pull in
//    transitively: controller/command_registry.h + command_handler.h
//    (::BasicCommand, ::BasicCommandRegistry, ::Command, ::CommandRegistry,
//    ::CommandHandler, ::MenuGroup, ::UnaryFunction, ::UnaryFunctionImpl,
//    ::CreateUniqueCommandId) and aui/key_codes.h (aui::KeyCode,
//    aui::KeyModifier(s), the modifier constants — mostly Qt/Wt aliases).
//    They belong to the controller and aui libraries and stay include-only
//    until those libraries get facades of their own.
//  - Foreign types that these headers only forward-declare (::Tracer,
//    ::DialogService, ::MainWindowInterface, ::SelectionModel,
//    ::OpenedViewInterface, ::NodeRef) — not client_core API; exporting the
//    incomplete redeclarations would collide with their owning libraries.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "core/core_module.h"
#include "core/default_node_command_registry.h"
#include "core/global_command_context.h"
#include "core/node_command_context.h"
#include "core/progress_host.h"
#include "core/progress_host_impl.h"
#include "core/selection_command_context.h"
#include "core/view_command_context.h"

export module scada.client.core;

// Mirror client_core's PUBLIC link transitivity.
export import scada.base;

export {
  // core_module.h
  using ::CoreModule;

  // default_node_command_registry.h
  using ::DefaultNodeCommandRegistry;

  // global_command_context.h
  using ::GlobalCommandContext;

  // node_command_context.h
  using ::NodeCommandContext;
  using ::NodeCommandHandler;

  // progress_host.h
  using ::ProgressHost;
  using ::ProgressStatus;
  using ::RunningProgress;

  // progress_host_impl.h
  using ::ProgressHostImpl;

  // selection_command_context.h
  using ::SelectionCommandContext;

  // view_command_context.h
  using ::ViewCommandContext;
}  // export
