// scada.client.services — named C++20 module facade over the
// client_services_qt headers (the Qt flavor; the wt flavor stays
// header-only).
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): headers stay the source of truth, the global
// module fragment includes them, the purview re-exports names with
// `export using`. The `export import`s of scada.client.aui / scada.common
// mirror client_services' PUBLIC links (the third PUBLIC link, transport,
// has no facade — its headers stay textual for consumers).
//
// Deliberately NOT in this facade:
//  - speech_service_mock.h, task_manager_mock.h: test mocks (excluded from
//    the library target by scada_module()'s *_mock rule).
//  - sapi.h: Windows-only COM type-library header (unguarded
//    #include <comdef.h>); its SpeechLib::* names are third-party COM
//    surface and would never be exported anyway. Include it textually in
//    Windows-only TUs.
//  - atl_module.h: ATL plumbing. Its meaningful content (CAtlExeModuleT and
//    the ATL-required `_Module` global) is Windows-only; the non-Windows
//    branch is an empty stub. Include-only.
//
// Included with a note:
//  - speech_service_impl.h is self-guarded internally (#ifdef _WIN32 around
//    <wrl/client.h> and the voice_ member) and only forward-declares
//    SpeechLib::ISpVoice, so it does NOT pull sapi.h and compiles on macOS.
//    The SpeechLib namespace (third-party COM) is not exported.
//
// Not exported (include the header textually / import the owning facade):
//  - the scada::id node-id constants pulled in via create_tree.h
//    (model/scada_node_ids.h): namespace-scope constexpr => internal
//    linkage, ill-formed to export;
//  - TimedDataSpec (timed_data/timed_data_spec.h via device_state_notifier.h)
//    and the node_service names (node_ref.h, node_util.h): owned and
//    exported by scada.timed_data / scada.node_service respectively;
//  - the LOG_* / LOG_NAME macros (base/boost_log.h via
//    device_state_notifier.h): macros cannot be exported.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "services/alias_resolver_factory.h"
#include "services/alias_service.h"
#include "services/connection_state_reporter.h"
#include "services/create_tree.h"
#include "services/device_state_notifier.h"
#include "services/speech_service.h"
#include "services/speech_service_impl.h"
#include "services/task_manager.h"
#include "services/task_manager_impl.h"
#include "services/telemetry.h"
#include "services/telemetry_client.h"

export module scada.client.services;

// Mirror client_services' PUBLIC link transitivity.
export import scada.client.aui;
export import scada.common;

export {
  // alias_resolver_factory.h
  using ::CreateAliasResolver;

  // alias_service.h
  using ::AliasService;
  using ::AliasServiceContext;

  // connection_state_reporter.h
  using ::ConnectionStateReporter;
  using ::ConnectionStateReporterContext;

  // create_tree.h
  using ::CreateTree;

  // device_state_notifier.h (ToString/ToLocalizedString extend the global
  // overload sets also exported by scada.core; exporting the name here
  // captures every overload visible in this GMF, matching the ::Format
  // precedent in scada.common)
  using ::DeviceState;
  using ::DeviceStateNotifier;
  using ::ToLocalizedString;
  using ::ToString;

  // speech_service.h / speech_service_impl.h
  using ::Speech;
  using ::SpeechService;

  // task_manager.h / task_manager_impl.h
  using ::TaskManager;
  using ::TaskManagerImpl;
  using ::TaskManagerImplContext;

  // telemetry.h / telemetry_client.h
  using ::TelemetryClient;
  using ::TelemetryEvent;
  using ::TelemetrySender;
  using ::TelemetryType;
}  // export
