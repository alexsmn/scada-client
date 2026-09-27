#include "export/configuration/configuration_transfer_commands.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "opcua_bridge/conversion.h"
#include "services/core_ui_text.h"

#include <fstream>
#include <iterator>

namespace {

// Message-box titles. Functions, not constants: Translate() reads the
// installed catalog and so needs a running QApplication.
std::u16string ExportTitle() {
  return Translate("Export Configuration");
}
std::u16string ImportTitle() {
  return Translate("Import Configuration");
}

// At most this many refused operations are listed; a message box is not a
// report, and the first few say what kind of problem it is.
constexpr std::size_t kMaxListedFailures = 10;

// The client's words for a status the server put in the result document as
// an OPC UA wire code (Part 4 §7.38.2 Common StatusCodes,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/7.38.2).
std::u16string WireStatusText(std::uint32_t code) {
  return StatusText(
      scada::opcua_bridge::ToScada(static_cast<opcua::StatusCode>(code)));
}

std::u16string FileName(const std::filesystem::path& path) {
  return path.filename().u16string();
}

// `report`, a blank line, then `question`: what a confirmation box says.
std::u16string WithQuestion(std::u16string report,
                            const std::u16string& question) {
  report.append(2, u'\n');
  report += question;
  return report;
}

bool NothingToDo(const scada::ImportResultSummary& result) {
  return result.added.empty() && result.modified.empty() &&
         result.deleted.empty() && result.references_added == 0 &&
         result.references_deleted == 0;
}

}  // namespace

std::filesystem::path ExportPath(std::filesystem::path chosen,
                                 bool compressed) {
  if (compressed && chosen.extension() != ".gz") {
    chosen += ".gz";
  }
  return chosen;
}

std::u16string ImportCheckReport(
    const ConfigurationTransferClient::ImportOutcome& outcome,
    const std::u16string& file_name) {
  std::u16string report;
  const auto& result = outcome.result;
  if (outcome.status) {
    if (result && NothingToDo(*result)) {
      return u16format(Translate("{} matches the current configuration. There "
                                 "is nothing to import."),
                       file_name);
    }
    if (result) {
      report = u16format(
          Translate("{}: {} to add, {} to change, {} to delete, {} reference "
                    "changes."),
          file_name, result->added.size(), result->modified.size(),
          result->deleted.size(),
          result->references_added + result->references_deleted);
    }
  } else if (outcome.status.code() == scada::StatusCode::Bad_InvalidState) {
    report = u16format(Translate("The configuration has changed since {} was "
                                 "exported. Importing it would overwrite "
                                 "those changes."),
                       file_name);
  } else {
    report = u16format(Translate("{} cannot be imported: {}"), file_name,
                       StatusText(outcome.status.code()));
  }

  if (result && !result->failures.empty()) {
    const std::size_t listed =
        std::min(result->failures.size(), kMaxListedFailures);
    for (std::size_t i = 0; i < listed; ++i) {
      const auto& failure = result->failures[i];
      report += u"\n";
      report += UtfConvert<char16_t>(failure.target);
      report += u": ";
      report += WireStatusText(failure.status.code);
      if (!failure.status.details.empty()) {
        report += u" ";
        report += UtfConvert<char16_t>(failure.status.details);
      }
    }
    if (result->failures.size() > listed) {
      report += u"\n";
      report += u16format(Translate("…and {} more."),
                          result->failures.size() - listed);
    }
  }
  return report;
}

Awaitable<void> ExportConfiguration(const ConfigurationTransferClient& client,
                                    DialogService& dialog_service) {
  // Named local: SaveParams::title is a u16string_view.
  const std::u16string title = ExportTitle();
  // A dismissed dialog completes by throwing (aui/qt/dialog_util.h), or with
  // an empty path; either is the operator cancelling.
  std::filesystem::path chosen;
  try {
    chosen = co_await dialog_service.SelectSaveFile(
        {.title = title, .default_path = "configuration.xml"});
  } catch (const std::exception&) {
    co_return;
  }
  if (chosen.empty()) {
    co_return;
  }

  auto exported = co_await client.Export();
  if (!exported.ok()) {
    co_await dialog_service.RunMessageBox(
        u16format(Translate("The configuration could not be exported: {}"),
                  StatusText(exported.status().code())),
        title, MessageBoxMode::Error);
    co_return;
  }

  const auto path = ExportPath(std::move(chosen), IsGzipData(*exported));
  std::ofstream stream{path, std::ios::binary};
  stream.write(exported->data(),
               static_cast<std::streamsize>(exported->size()));
  if (!stream) {
    co_await dialog_service.RunMessageBox(Translate("Failed to write file."),
                                          title, MessageBoxMode::Error);
    co_return;
  }
  co_await dialog_service.RunMessageBox(
      u16format(Translate("The configuration was exported to {}."),
                FileName(path)),
      title, MessageBoxMode::Info);
}

Awaitable<void> ImportConfiguration(const ConfigurationTransferClient& client,
                                    DialogService& dialog_service) {
  const std::u16string title = ImportTitle();
  std::filesystem::path path;
  try {
    path = co_await dialog_service.SelectOpenFile(title);
  } catch (const std::exception&) {
    co_return;
  }
  if (path.empty()) {
    co_return;
  }
  std::ifstream stream{path, std::ios::binary};
  if (!stream) {
    co_await dialog_service.RunMessageBox(Translate("Failed to open file."),
                                          title, MessageBoxMode::Error);
    co_return;
  }
  const std::string contents{std::istreambuf_iterator<char>{stream},
                             std::istreambuf_iterator<char>{}};
  const std::u16string file_name = FileName(path);

  // ADR 0014: every import is checked by a dry run first.
  ConfigurationTransferClient::ImportOptions options{.dry_run = true};
  auto check = co_await client.Import(contents, options);

  if (check.status.code() == scada::StatusCode::Bad_InvalidState) {
    auto again = co_await dialog_service.RunMessageBox(
        WithQuestion(ImportCheckReport(check, file_name),
                     Translate("Check it anyway?")),
        title, MessageBoxMode::QuestionYesNoDefaultNo);
    if (again != MessageBoxResult::Yes) {
      co_return;
    }
    options.ignore_version = true;
    check = co_await client.Import(contents, options);
  }

  if (!check.status) {
    co_await dialog_service.RunMessageBox(ImportCheckReport(check, file_name),
                                          title, MessageBoxMode::Error);
    co_return;
  }
  if (check.result && NothingToDo(*check.result)) {
    co_await dialog_service.RunMessageBox(ImportCheckReport(check, file_name),
                                          title, MessageBoxMode::Info);
    co_return;
  }

  auto apply = co_await dialog_service.RunMessageBox(
      WithQuestion(ImportCheckReport(check, file_name),
                   Translate("Apply changes?")),
      title, MessageBoxMode::QuestionYesNoDefaultNo);
  if (apply != MessageBoxResult::Yes) {
    co_return;
  }

  options.dry_run = false;
  auto applied = co_await client.Import(contents, options);
  if (!applied.status) {
    // The check passed and the real run did not: the configuration changed
    // in between. Say why, as the check would have.
    co_await dialog_service.RunMessageBox(ImportCheckReport(applied, file_name),
                                          title, MessageBoxMode::Error);
    co_return;
  }
  const auto& result = applied.result;
  co_await dialog_service.RunMessageBox(
      u16format(Translate("Imported: {} added, {} changed, {} deleted."),
                result ? result->added.size() : 0,
                result ? result->modified.size() : 0,
                result ? result->deleted.size() : 0),
      title, MessageBoxMode::Info);
}
