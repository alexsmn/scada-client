// scada.client.profile — named C++20 module facade over the client/profile
// headers (Qt flavor only, built for client_profile_qt; the wt flavor stays
// header-only).
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): headers stay the source of truth, the global
// module fragment includes them, the purview re-exports names with
// `export using`. `export import scada.client.aui;` mirrors client_profile's
// PUBLIC link on aui (client_base / scada_common_events are PRIVATE links
// and are not mirrored).
//
// Not exported (documented; textual #include alongside the import):
//  - the global-namespace operator<< overloads for WindowItem /
//    WindowDefinition (window_definition.h) and, in general, all
//    global-namespace ADL operators — exporting ::operator<< would drag the
//    entire global overload set;
//  - the FromJson name: the primary template lives in client/base's
//    base/json.h and is owned/exported by scada.client.base. The explicit
//    FromJson<T> specializations declared here (PageLayout in page_layout.h;
//    base::Time, TimeRange, base::TimeDelta, WindowItems, WindowDefinition in
//    window_definition_util.h) are kept decl-reachable by the static_assert
//    keep-alives below, and the non-template scada::NodeId overload
//    (window_definition_util.h) is include-only — TUs that call FromJson
//    unqualified need `import scada.client.base;` (or the textual include)
//    in addition to this module.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "profile/page.h"
#include "profile/page_layout.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "profile/window_definition_util.h"

export module scada.client.profile;

// Mirror client_profile's PUBLIC link transitivity.
export import scada.client.aui;

// Keep the GMF-attached FromJson<T> explicit-specialization declarations
// decl-reachable for import-only TUs (the primary template is exported by
// scada.client.base; if these declarations were discarded, a caller would
// silently instantiate the undefined primary instead).
static_assert(sizeof(&::FromJson<PageLayout>) > 0);
static_assert(sizeof(&::FromJson<scada::base::Time>) > 0);
static_assert(sizeof(&::FromJson<TimeRange>) > 0);
static_assert(sizeof(&::FromJson<scada::base::TimeDelta>) > 0);
static_assert(sizeof(&::FromJson<WindowItems>) > 0);
static_assert(sizeof(&::FromJson<WindowDefinition>) > 0);

export {
  // page.h
  using ::Page;

  // page_layout.h
  using ::PageLayout;
  using ::PageLayoutBlock;

  // profile.h
  using ::MainWindowDef;
  using ::Profile;

  // window_definition.h
  using ::WindowDefinition;
  using ::WindowItem;
  using ::WindowItems;

  // window_definition_util.h
  using ::RestoreBlob;
  using ::RestoreTimeRange;
  using ::SaveBlob;
  using ::SaveTimeRange;

  // The global ToJson overload set spread across the profile headers
  // (page_layout.h, window_definition_util.h). ToJson is declared by no
  // imported facade's headers (checked: client/base, client/core, client/aui,
  // common, core), so this facade owns the name. Exported once, after all
  // GMF includes, so the full overload set is captured.
  using ::ToJson;
}  // export
