#pragma once

#include "base/settings_store.h"

#include <filesystem>
#include <memory>
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

}  // namespace client
