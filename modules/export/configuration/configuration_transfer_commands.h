#pragma once

#include "base/awaitable.h"
#include "export/configuration/configuration_transfer_client.h"

#include <filesystem>
#include <string>

class DialogService;

// "Export Configuration...": asks where to save, exports the default scope
// from the configuration server (ADR 0014) and writes it — `.xml`, or
// `.xml.gz` when the server compressed it.
Awaitable<void> ExportConfiguration(const ConfigurationTransferClient& client,
                                    DialogService& dialog_service);

// "Import Configuration...": asks for a file and checks it with a dry run on
// the server first, reporting what it would change and what the server
// refused. Only a clean check is offered for applying. A file exported before
// the configuration changed is refused by the server's version check; the
// user may check it again without that check, explicitly.
Awaitable<void> ImportConfiguration(const ConfigurationTransferClient& client,
                                    DialogService& dialog_service);

// The report a dry run's `outcome` reads as, naming `file_name`: what would
// change, or why it cannot be applied, with each refused operation.
std::u16string ImportCheckReport(
    const ConfigurationTransferClient::ImportOutcome& outcome,
    const std::u16string& file_name);

// The path an export is written to: `chosen`, with ".gz" appended when the
// export is compressed and the name does not already say so.
std::filesystem::path ExportPath(std::filesystem::path chosen, bool compressed);
