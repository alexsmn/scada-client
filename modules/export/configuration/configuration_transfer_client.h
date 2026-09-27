#pragma once

#include "address_space/configuration_nodeset.h"
#include "base/awaitable.h"
#include "scada/co_result.h"
#include "scada/method_service.h"
#include "scada/node_id.h"
#include "scada/status.h"
#include "scada/variant.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

// Calls `method_id` on `object_id` with `arguments`. The production binding
// goes through the node service's session; tests bind a fake transfer object.
using ConfigurationTransferCall =
    std::function<scada::CoStatusOr<scada::CallResult>(
        scada::NodeId object_id,
        scada::NodeId method_id,
        std::vector<scada::Variant> arguments)>;

// The configuration server's transfer object (ADR 0014): a standard
// TemporaryFileTransferType instance at s=ConfigurationTransfer in the
// security namespace. OPC UA Part 20 §4.4,
// https://reference.opcfoundation.org/Core/Part20/v105/docs/4.4
scada::NodeId ConfigurationTransferObjectId();

// Exports and imports configuration over the transfer object with standard
// Part 20 calls: GenerateFileForRead / Read / Close for an export,
// GenerateFileForWrite / Write / CloseAndCommit for an import, and the
// import's result read back by naming the write file's NodeId.
class ConfigurationTransferClient {
 public:
  explicit ConfigurationTransferClient(ConfigurationTransferCall call);

  // The default export: a UANodeSet document, gzip-compressed (RFC 1952)
  // when the server found it large.
  Awaitable<scada::StatusOr<std::string>> Export() const;

  struct ImportOptions {
    // Apply the whole set and roll it back: what a real import would do,
    // with nothing kept.
    bool dry_run = false;
    // Apply a file exported before the configuration changed.
    bool ignore_version = false;
  };

  struct ImportOutcome {
    // CloseAndCommit's answer: Good when the import applied (or, for a dry
    // run, would have), the reason otherwise.
    scada::Status status = scada::StatusCode::Good;
    // The server's result document, read for display. Absent when the
    // server kept none — a failure before the file reached it.
    std::optional<scada::ImportResultSummary> result;
  };

  // Uploads `contents` and commits it. A refusal is the outcome's status,
  // never an exception; only a failed transfer is a bad status here.
  Awaitable<ImportOutcome> Import(std::string contents,
                                  ImportOptions options) const;

 private:
  // A temporary file as a Generate call returned it.
  struct TemporaryFile {
    scada::NodeId node_id;
    scada::UInt32 handle = 0;
  };

  Awaitable<scada::StatusOr<TemporaryFile>> Generate(
      scada::NumericId method,
      scada::Variant options) const;
  Awaitable<scada::StatusOr<std::string>> ReadAll(
      const TemporaryFile& file) const;
  Awaitable<scada::Status> WriteAll(const TemporaryFile& file,
                                    const std::string& contents) const;
  Awaitable<void> Close(const TemporaryFile& file) const;

  const ConfigurationTransferCall call_;
};

// True when `data` starts with the gzip magic bytes 1F 8B (RFC 1952 §2.3.1).
bool IsGzipData(std::string_view data);
