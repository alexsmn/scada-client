#include "export/configuration/configuration_transfer_client.h"

#include "model/namespaces.h"

#include <algorithm>

namespace {

// Standard method NodeIds (namespace 0, Opc.Ua.NodeIds.csv). A client calls a
// type's method on the instance, so these are what the server expects.
// OPC UA Part 20 §4.2 FileType and §4.4 TemporaryFileTransferType,
// https://reference.opcfoundation.org/Core/Part20/v105/docs/4.2
constexpr scada::NumericId kFileTypeClose = 11583;
constexpr scada::NumericId kFileTypeRead = 11585;
constexpr scada::NumericId kFileTypeWrite = 11588;
constexpr scada::NumericId kGenerateFileForRead = 15746;
constexpr scada::NumericId kGenerateFileForWrite = 15749;
constexpr scada::NumericId kCloseAndCommit = 15751;

// Well inside the smallest transport limit, as ADR 0014 decision 8 asks and
// as the web client's UaFile does.
constexpr std::size_t kChunkSize = 64 * 1024;

scada::Variant OptionsVariant(
    const ConfigurationTransferClient::ImportOptions& options) {
  std::vector<scada::String> flags;
  if (options.dry_run) {
    flags.emplace_back("dryRun");
  }
  if (options.ignore_version) {
    flags.emplace_back("ignoreVersion");
  }
  return flags.empty() ? scada::Variant{} : scada::Variant{std::move(flags)};
}

}  // namespace

scada::NodeId ConfigurationTransferObjectId() {
  return scada::NodeId{"ConfigurationTransfer",
                       scada::NamespaceIndexes::SECURITY};
}

bool IsGzipData(std::string_view data) {
  return data.size() >= 2 && static_cast<unsigned char>(data[0]) == 0x1f &&
         static_cast<unsigned char>(data[1]) == 0x8b;
}

ConfigurationTransferClient::ConfigurationTransferClient(
    ConfigurationTransferCall call)
    : call_{std::move(call)} {}

Awaitable<scada::StatusOr<ConfigurationTransferClient::TemporaryFile>>
ConfigurationTransferClient::Generate(scada::NumericId method,
                                      scada::Variant options) const {
  // GenerateFileForRead / GenerateFileForWrite(generateOptions) ->
  // (fileNodeId, fileHandle, ...), Part 20 §4.4.3 and §4.4.4.
  auto result = co_await call_(ConfigurationTransferObjectId(),
                               scada::NodeId{method}, {std::move(options)});
  if (!result.ok()) {
    co_return result.status();
  }
  const auto& outputs = result->output_arguments;
  const auto* node_id =
      outputs.size() >= 2 ? outputs[0].get_if<scada::NodeId>() : nullptr;
  const auto* handle =
      outputs.size() >= 2 ? outputs[1].get_if<scada::UInt32>() : nullptr;
  if (!node_id || !handle) {
    co_return scada::StatusCode::Bad;
  }
  co_return TemporaryFile{.node_id = *node_id, .handle = *handle};
}

Awaitable<scada::StatusOr<std::string>> ConfigurationTransferClient::ReadAll(
    const TemporaryFile& file) const {
  std::string contents;
  for (;;) {
    // Read(fileHandle, length) -> (data); empty at the end (Part 20 §4.2.4).
    auto result =
        co_await call_(file.node_id, scada::NodeId{kFileTypeRead},
                       {scada::Variant{file.handle},
                        scada::Variant{static_cast<scada::Int32>(kChunkSize)}});
    if (!result.ok()) {
      co_return result.status();
    }
    const auto* chunk =
        result->output_arguments.empty()
            ? nullptr
            : result->output_arguments[0].get_if<scada::ByteString>();
    if (!chunk || chunk->empty()) {
      co_return contents;
    }
    contents.append(chunk->begin(), chunk->end());
  }
}

Awaitable<scada::Status> ConfigurationTransferClient::WriteAll(
    const TemporaryFile& file,
    const std::string& contents) const {
  for (std::size_t offset = 0; offset < contents.size(); offset += kChunkSize) {
    const std::size_t end = std::min(offset + kChunkSize, contents.size());
    // Write(fileHandle, data), Part 20 §4.2.5.
    auto result = co_await call_(
        file.node_id, scada::NodeId{kFileTypeWrite},
        {scada::Variant{file.handle},
         scada::Variant{scada::ByteString{contents.begin() + offset,
                                          contents.begin() + end}}});
    if (!result.ok()) {
      co_return result.status();
    }
  }
  co_return scada::StatusCode::Good;
}

Awaitable<void> ConfigurationTransferClient::Close(
    const TemporaryFile& file) const {
  // Close(fileHandle), Part 20 §4.2.3; on a write transfer, the abort. Its
  // outcome changes nothing: the transfer is over either way, and the server
  // drops an unclosed temporary file when it times out.
  [[maybe_unused]] auto closed =
      co_await call_(file.node_id, scada::NodeId{kFileTypeClose},
                     {scada::Variant{file.handle}});
}

Awaitable<scada::StatusOr<std::string>> ConfigurationTransferClient::Export()
    const {
  auto file = co_await Generate(kGenerateFileForRead, scada::Variant{});
  if (!file.ok()) {
    co_return file.status();
  }
  auto contents = co_await ReadAll(*file);
  co_await Close(*file);
  co_return contents;
}

Awaitable<ConfigurationTransferClient::ImportOutcome>
ConfigurationTransferClient::Import(std::string contents,
                                    ImportOptions options) const {
  auto file = co_await Generate(kGenerateFileForWrite, OptionsVariant(options));
  if (!file.ok()) {
    co_return ImportOutcome{.status = file.status()};
  }
  if (auto written = co_await WriteAll(*file, contents); !written) {
    co_await Close(*file);
    co_return ImportOutcome{.status = std::move(written)};
  }

  // CloseAndCommit(fileHandle), Part 20 §4.4.5: its status is the import's.
  ImportOutcome outcome;
  auto committed = co_await call_(ConfigurationTransferObjectId(),
                                  scada::NodeId{kCloseAndCommit},
                                  {scada::Variant{file->handle}});
  if (!committed.ok()) {
    outcome.status = committed.status();
  }

  // The result is kept either way, under the write file's NodeId.
  auto result_file =
      co_await Generate(kGenerateFileForRead, scada::Variant{file->node_id});
  if (!result_file.ok()) {
    co_return outcome;
  }
  auto document = co_await ReadAll(*result_file);
  co_await Close(*result_file);
  if (document.ok()) {
    if (auto summary = scada::ReadImportResultSummary(*document);
        summary.ok()) {
      outcome.result = std::move(*summary);
    }
  }
  co_return outcome;
}
