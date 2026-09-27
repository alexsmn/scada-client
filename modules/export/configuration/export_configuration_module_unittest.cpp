#include "export/configuration/export_configuration_module.h"

#include "aui/dialog_service_mock.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/test/scoped_temp_dir.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "core/global_command_context.h"
#include "export/configuration/configuration_transfer_commands.h"
#include "model/namespace_uris.h"
#include "model/namespaces.h"
#include "node_service/static/static_node_service.h"
#include "resources/common_resources.h"

#include <deque>
#include <fstream>
#include <gmock/gmock.h>
#include <map>

using namespace testing;

namespace {

// gmock's HasSubstr has no char16_t overload.
MATCHER_P(HasSubstr16, needle, "") {
  return std::u16string_view{arg}.find(needle) != std::u16string_view::npos;
}

scada::CoStatusOr<scada::CallResult> ReturnCall(
    scada::StatusOr<scada::CallResult> result) {
  co_return result;
}

Awaitable<MessageBoxResult> ReturnAnswer(MessageBoxResult answer) {
  co_return answer;
}

Awaitable<std::filesystem::path> ReturnPath(std::filesystem::path path) {
  co_return path;
}

// A configuration server's transfer object, as far as a client can tell:
// the Part 20 temporary-file methods over in-memory files, an export to read,
// and per commit a scripted outcome and result document, kept under the
// write file's NodeId the way the server keeps it.
class FakeTransferObject {
 public:
  struct Commit {
    // CloseAndCommit's answer, and the result document kept for it.
    scada::Status status = scada::StatusCode::Good;
    scada::ConfigurationNodeSetChanges result;
  };

  std::string export_contents;
  std::deque<Commit> commits;

  // What each CloseAndCommit received.
  struct Received {
    std::string contents;
    scada::Variant options;
  };
  std::vector<Received> received;

  ConfigurationTransferCall MakeCall() {
    // A plain lambda returning a helper's awaitable (CLAUDE.md: a gmock or
    // std::function action must not be a capturing coroutine lambda).
    return [this](scada::NodeId object_id, scada::NodeId method_id,
                  std::vector<scada::Variant> arguments) {
      return ReturnCall(Handle(object_id, method_id.numeric_id(), arguments));
    };
  }

 private:
  struct File {
    std::string contents;
    std::size_t position = 0;
    scada::Variant options;
  };

  scada::StatusOr<scada::CallResult> Open(std::string contents,
                                          scada::Variant options = {}) {
    const scada::UInt32 handle = next_handle_++;
    const scada::NodeId node_id{"tmp-" + std::to_string(handle),
                                scada::NamespaceIndexes::SECURITY};
    files_[handle] = {std::move(contents), 0, std::move(options)};
    node_ids_[handle] = node_id;
    return scada::CallResult{{scada::Variant{node_id}, scada::Variant{handle},
                              scada::Variant{scada::NodeId{}}}};
  }

  scada::StatusOr<scada::CallResult> Handle(
      const scada::NodeId& object_id,
      scada::NumericId method,
      const std::vector<scada::Variant>& arguments) {
    const auto* handle_argument =
        arguments.empty() ? nullptr : arguments[0].get_if<scada::UInt32>();
    const scada::UInt32 handle = handle_argument ? *handle_argument : 0;
    switch (method) {
      case 15746:  // GenerateFileForRead
        if (const auto* write_file = arguments[0].get_if<scada::NodeId>()) {
          const auto kept = results_.find(*write_file);
          if (kept == results_.end()) {
            return scada::StatusCode::Bad_WrongNodeId;
          }
          return Open(kept->second);
        }
        return Open(export_contents);
      case 15749:  // GenerateFileForWrite
        return Open({}, arguments.empty() ? scada::Variant{} : arguments[0]);
      case 11585: {  // Read
        File& file = files_.at(handle);
        const std::size_t length =
            std::min<std::size_t>(arguments[1].get<scada::Int32>(),
                                  file.contents.size() - file.position);
        scada::ByteString chunk{file.contents.begin() + file.position,
                                file.contents.begin() + file.position + length};
        file.position += length;
        return scada::CallResult{{scada::Variant{std::move(chunk)}}};
      }
      case 11588: {  // Write
        const auto& data = arguments[1].get<scada::ByteString>();
        files_.at(handle).contents.append(data.begin(), data.end());
        return scada::CallResult{};
      }
      case 11583:  // Close
        files_.erase(handle);
        return scada::CallResult{};
      case 15751: {  // CloseAndCommit
        EXPECT_EQ(object_id.string_id(), "ConfigurationTransfer");
        File file = std::move(files_.at(handle));
        files_.erase(handle);
        received.push_back({file.contents, file.options});
        Commit commit = std::move(commits.front());
        commits.pop_front();
        auto document = scada::WriteConfigurationNodeSetChanges(
            commit.result, scada::model::GetCanonicalNamespaceUris(), {});
        EXPECT_TRUE(document.ok());
        results_[node_ids_.at(handle)] = *document;
        return scada::MakeCallResult(commit.status,
                                     {scada::Variant{scada::NodeId{}}});
      }
      default:
        return scada::StatusCode::Bad_WrongMethodId;
    }
  }

  scada::UInt32 next_handle_ = 1;
  std::map<scada::UInt32, File> files_;
  std::map<scada::UInt32, scada::NodeId> node_ids_;
  std::map<scada::NodeId, std::string> results_;
};

// A result document: `added` created, `modified` changed in place.
scada::ConfigurationNodeSetChanges Result(
    std::vector<scada::NumericId> added,
    std::vector<scada::NumericId> modified,
    bool dry_run) {
  scada::ConfigurationNodeSetChanges result;
  const auto node = [](scada::NumericId id) {
    return scada::NodeState{
        .node_id = {id, scada::NamespaceIndexes::TS},
        .node_class = scada::NodeClass::Variable,
        .attributes = {.browse_name = scada::QualifiedName{"Item"}}};
  };
  for (scada::NumericId id : added) {
    result.nodes_to_add.push_back(node(id));
  }
  for (scada::NumericId id : modified) {
    result.nodes_to_add.push_back(node(id));
    result.nodes_to_delete.push_back(
        {.node_id = node(id).node_id, .delete_reverse_references = false});
  }
  result.outcome = scada::ConfigurationImportOutcome{.committed = !dry_run,
                                                     .dry_run = dry_run};
  return result;
}

scada::Variant Flags(std::vector<scada::String> flags) {
  return scada::Variant{std::move(flags)};
}

class ExportConfigurationModuleTest : public Test {
 protected:
  std::filesystem::path WriteFile(std::string_view name,
                                  std::string_view contents) {
    const auto path = temp_dir_.path() / name;
    std::ofstream{path, std::ios::binary} << contents;
    return path;
  }

  // Answers the next message boxes with `answers` in turn, recording what
  // each said.
  void ExpectMessages(std::vector<MessageBoxResult> answers) {
    auto& expectation = EXPECT_CALL(dialog_service_, RunMessageBox(_, _, _))
                            .Times(static_cast<int>(answers.size()));
    for (MessageBoxResult answer : answers) {
      expectation.WillOnce([this, answer](std::u16string_view message,
                                          std::u16string_view,
                                          MessageBoxMode mode) {
        messages_.emplace_back(message);
        modes_.push_back(mode);
        return ReturnAnswer(answer);
      });
    }
  }

  void Run(Awaitable<void> command) {
    WaitAwaitable(executor_, std::move(command));
  }

  // First member: the fixture's files live under it.
  scada::ScopedTempDir temp_dir_{"scada_export_configuration_test"};

  TestExecutor executor_;
  FakeTransferObject server_;
  ConfigurationTransferClient client_{server_.MakeCall()};
  StrictMock<MockDialogService> dialog_service_;
  std::vector<std::u16string> messages_;
  std::vector<MessageBoxMode> modes_;
};

TEST_F(ExportConfigurationModuleTest, RegistersBothCommands) {
  StaticNodeService node_service;
  BasicCommandRegistry<GlobalCommandContext> global_commands;
  UiCommandRegistry ui_command_registry;
  ExportConfigurationModule module{{.executor_ = executor_,
                                    .node_service_ = node_service,
                                    .global_commands_ = global_commands,
                                    .ui_command_registry_ = ui_command_registry,
                                    .call_ = server_.MakeCall()}};

  EXPECT_TRUE(global_commands.FindCommand(ID_EXPORT_CONFIGURATION));
  EXPECT_TRUE(global_commands.FindCommand(ID_IMPORT_CONFIGURATION));
}

// A compressed export is saved under the name the operator chose plus the
// ".gz" that says what it is, read in chunks across the transfer.
TEST_F(ExportConfigurationModuleTest, ExportSavesWhatTheServerGenerated) {
  server_.export_contents =
      std::string{"\x1f\x8b"} + std::string(200 * 1024, 'x');
  const auto chosen = temp_dir_.path() / "saved.xml";
  EXPECT_CALL(dialog_service_, SelectSaveFile(_))
      .WillOnce([chosen](const DialogService::SaveParams&) {
        return ReturnPath(chosen);
      });
  ExpectMessages({MessageBoxResult::Ok});

  Run(ExportConfiguration(client_, dialog_service_));

  std::ifstream saved{temp_dir_.path() / "saved.xml.gz", std::ios::binary};
  ASSERT_TRUE(saved);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>{saved}, {}),
            server_.export_contents);
  EXPECT_EQ(modes_, std::vector{MessageBoxMode::Info});
}

// ADR 0014: a dry run first, the operator shown what it would change, and
// only on their Yes the same file for real.
TEST_F(ExportConfigurationModuleTest, ImportChecksThenApplies) {
  const auto file = WriteFile("config.xml", "<UANodeSet/>");
  EXPECT_CALL(dialog_service_, SelectOpenFile(_))
      .WillOnce([file](std::u16string_view) { return ReturnPath(file); });
  server_.commits = {{.result = Result({8}, {7}, /*dry_run=*/true)},
                     {.result = Result({8}, {7}, /*dry_run=*/false)}};
  ExpectMessages({MessageBoxResult::Yes, MessageBoxResult::Ok});

  Run(ImportConfiguration(client_, dialog_service_));

  ASSERT_THAT(server_.received, SizeIs(2));
  EXPECT_EQ(server_.received[0].contents, "<UANodeSet/>");
  EXPECT_EQ(server_.received[0].options, Flags({"dryRun"}));
  EXPECT_TRUE(server_.received[1].options.is_null());
  EXPECT_THAT(messages_[0], HasSubstr16(u"config.xml: 1 to add, 1 to change"));
  EXPECT_EQ(modes_[0], MessageBoxMode::QuestionYesNoDefaultNo);
  EXPECT_EQ(messages_[1], u"Imported: 1 added, 1 changed, 0 deleted.");
}

TEST_F(ExportConfigurationModuleTest, DecliningTheCheckAppliesNothing) {
  const auto file = WriteFile("config.xml", "<UANodeSet/>");
  EXPECT_CALL(dialog_service_, SelectOpenFile(_))
      .WillOnce([file](std::u16string_view) { return ReturnPath(file); });
  server_.commits = {{.result = Result({8}, {}, /*dry_run=*/true)}};
  ExpectMessages({MessageBoxResult::No});

  Run(ImportConfiguration(client_, dialog_service_));

  EXPECT_THAT(server_.received, SizeIs(1));
}

// Part 12 §7.8.5's version check, borrowed by ADR 0014: a stale file is
// checked again without it only when the operator says so.
TEST_F(ExportConfigurationModuleTest, AStaleFileIsCheckedAgainOnlyOnRequest) {
  const auto file = WriteFile("old.xml", "<UANodeSet/>");
  EXPECT_CALL(dialog_service_, SelectOpenFile(_))
      .WillOnce([file](std::u16string_view) { return ReturnPath(file); });
  server_.commits = {{.status = scada::StatusCode::Bad_InvalidState,
                      .result = Result({}, {}, /*dry_run=*/true)},
                     {.result = Result({}, {7}, /*dry_run=*/true)}};
  ExpectMessages({MessageBoxResult::Yes, MessageBoxResult::No});

  Run(ImportConfiguration(client_, dialog_service_));

  ASSERT_THAT(server_.received, SizeIs(2));
  EXPECT_EQ(server_.received[1].options, Flags({"dryRun", "ignoreVersion"}));
  EXPECT_THAT(messages_[0], HasSubstr16(u"has changed since old.xml"));
}

// Each refused operation is listed against the node it names, in the
// client's words for the status.
TEST_F(ExportConfigurationModuleTest, TheReportListsEachRefusal) {
  scada::ImportResultSummary summary;
  summary.failures.push_back(
      {.list = "NodesToAdd",
       .target = "ns=5;i=9",
       .status = {.code = 0x801f0000, .details = "in a security table"}});
  const ConfigurationTransferClient::ImportOutcome outcome{
      .status = scada::StatusCode::Bad_UserAccessDenied, .result = summary};

  const std::u16string report = ImportCheckReport(outcome, u"f.xml");

  EXPECT_THAT(report, HasSubstr16(u"f.xml cannot be imported"));
  EXPECT_THAT(report, HasSubstr16(u"\nns=5;i=9: "));
  EXPECT_THAT(report, HasSubstr16(u"in a security table"));
}

TEST_F(ExportConfigurationModuleTest, ADismissedFileDialogDoesNothing) {
  EXPECT_CALL(dialog_service_, SelectOpenFile(_));  // The default throws.

  Run(ImportConfiguration(client_, dialog_service_));

  EXPECT_THAT(server_.received, IsEmpty());
}

TEST(ExportPathTest, AddsGzOnlyWhenCompressedAndUnsaid) {
  EXPECT_EQ(ExportPath("a.xml", true), "a.xml.gz");
  EXPECT_EQ(ExportPath("a.xml.gz", true), "a.xml.gz");
  EXPECT_EQ(ExportPath("a.xml", false), "a.xml");
}

}  // namespace
