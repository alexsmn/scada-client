// scada.client.base — named C++20 module facade over the client/base headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): the headers stay the source of truth, the global
// module fragment includes them, the purview re-exports the public names with
// `export using`. `export import scada.base;` mirrors client_base's PUBLIC
// link on scada_base. Note that "base/<name>.h" resolves to client/base/ here:
// the client include root precedes core's on the include path (core-owned
// headers reach this GMF only transitively, via scada_base's include dirs).
//
// Deliberate exclusions (include the header textually where needed):
//  - memory_istream.h — unguarded Windows-only content (#include <Windows.h>),
//    AND its global ::MemoryIStream collides with the identical-named class
//    that scada.base already exports from core/base/memory_istream.h;
//    exporting both would make TUs importing the two facades together
//    ill-formed. Include-only, never in this GMF.
//  - blinker_mock.h — test mock, not part of the client_base library.
//  - the global ::operator<< overloads for TimeRange / TimeRange::Type
//    (time_range.h) — exporting ::operator<< would drag the entire global
//    overload set (boost_log/debug_util streams); same policy as
//    scada.base / scada.core.
//  - excel.h's non-Windows stub declarations (::HRESULT, ::DATE, ::VARIANT,
//    ::IDispatch, the base::win::ScopedVariant and Microsoft::WRL::ComPtr
//    stand-ins) — platform/third-party surrogates, not client_base API; on
//    Windows the real types come from the Windows SDK / core's win headers.
//    Only the Excel / ExcelSheetModel classes are exported.
//
// `using ::ToString;` below exports the overload set visible in this GMF:
// the TimeRange overloads declared by time_range.h plus the scada-core
// global ToString overloads dragged in transitively (date_time.h etc.).
// Those transitive overloads are the same global-module entities that
// scada.core exports, so TUs importing both facades see one merged set.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "base/blinker.h"
#include "base/client_paths.h"
#include "base/e2e_test_hooks.h"
#include "base/excel.h"  // self-stubbed: COM types are faked on non-Windows
#include "base/file_settings_store.h"
#include "base/json.h"
#include "base/memory_settings_store.h"
#include "base/pool.h"
#include "base/program_options.h"
#include "base/settings_store.h"
#include "base/relative_time_range.h"
#include "base/utils.h"
#include "base/web_util.h"

export module scada.client.base;

// Mirror client_base's PUBLIC link transitivity.
export import scada.base;

// ---- namespace client ----
export namespace client {

// client_paths.h. The DIR_* / PATH_* path keys are enumerators of an
// unnamed namespace-scope enum — no linkage, ill-formed to export
// ([module.interface]); include the header textually to use them.
using client::RegisterPathProvider;

// e2e_test_hooks.h
using client::CreateE2eSettingsStore;
using client::GetE2eHardwareTreeDevicesReportPath;
using client::GetE2eHistoricalTimedDataReportPath;
using client::GetE2eObjectTreeLabelsReportPath;
using client::GetE2eObjectViewValuesReportPath;
using client::GetE2eOperatorUseCasesReportPath;
using client::GetE2eProfileSaveReportPath;
using client::GetE2eProfileSaveUserId;
using client::IsE2eTestMode;
using client::ReportE2eStatus;
using client::ReportE2eStatusIfUnset;

// program_options.h
using client::GetOptionValue;
using client::HasOption;
using client::InitProgramOptions;

}  // namespace client

// ---- global namespace ----
export {
  // blinker.h
  using ::Blinker;
  using ::BlinkerManager;
  using ::BlinkerManagerImpl;

  // excel.h (the COM/stub surrogate types are deliberately not exported)
  using ::Excel;
  using ::ExcelSheetModel;

  // file_settings_store.h
  using ::FileSettingsStore;

  // json.h
  using ::FromJson;
  using ::LoadJsonFromFile;
  using ::LoadJsonFromString;
  using ::SaveJsonToFile;
  using ::SaveJsonToString;

  // memory_settings_store.h
  using ::MemorySettingsStore;

  // pool.h
  using ::Pool;
  using ::PoolItem;

  // settings_store.h
  using ::SettingsStore;
#ifdef _WIN32
  using ::RegistrySettingsStore;
#endif

  // relative_time_range.h — only the ToString overloads stay at global scope
  // (deliberately, so they do not hide the other global ToString overloads;
  // see that header's comment). The range type and its helpers moved into
  // namespace scada and are exported below. The global operator<< overloads
  // are still deliberately not exported.
  using ::ToString;

  // utils.h
  using ::HumanCompareText;
  using ::HumanCompareTextT;

  // web_util.h
  using ::IsWebUrl;
  using ::MakeFileUrl;
}  // export

// The chrono migration renamed the old global ::TimeRange to
// scada::RelativeTimeRange — `TimeRange` now names core's Interval<Time>, a
// different type — and renamed its converters ToDateTimeRange /
// ToDateTimeRangeWithOpenRange to ToTimeRange / ToTimeRangeWithOpenRange.
export namespace scada {

// relative_time_range.h
using scada::ParseTimeRangeType;
using scada::RelativeTimeRange;
using scada::ToTimeRange;
using scada::ToTimeRangeWithOpenRange;

}  // namespace scada
