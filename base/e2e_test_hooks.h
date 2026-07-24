#pragma once

#include "base/settings_store.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace client {

bool IsE2eTestMode();
std::shared_ptr<SettingsStore> CreateE2eSettingsStore();

void ReportE2eStatus(std::string_view status);
void ReportE2eStatusIfUnset(std::string_view status);

std::filesystem::path GetE2eOperatorUseCasesReportPath();
std::filesystem::path GetE2eObjectViewValuesReportPath();
std::filesystem::path GetE2eObjectTreeLabelsReportPath();
std::filesystem::path GetE2eHardwareTreeDevicesReportPath();
std::filesystem::path GetE2eHistoricalTimedDataReportPath();
// End of the historical window the E2E timed-data check must read
// (--test-historical-timed-data-end, a scada::Time internal value).
// The harness passes a timestamp predating the client's launch so live
// monitored-item updates cannot satisfy the check — only history served by
// the server (the historian, in the Cluster topology) can.
std::optional<int64_t> GetE2eHistoricalTimedDataEndTime();
std::filesystem::path GetE2eProfileSaveReportPath();
std::string GetE2eProfileSaveUserId();

}  // namespace client
