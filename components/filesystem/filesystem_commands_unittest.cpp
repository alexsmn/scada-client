#include "filesystem/filesystem_commands.h"

#include "aui/dialog_service.h"
#include "aui/types.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "filesystem/file_manager.h"
#include "filesystem/file_registry.h"
#include "main_window/main_window_interface.h"
#include "model/filesystem_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "scada/status_exception.h"

#include <gtest/gtest.h>

#include "base/debug_util.h"

namespace {

// Minimal dialog-service fake that records `RunMessageBox` calls and
// immediately resolves their promises. Skip pulling in `MockDialogService`
// and its link-time dependencies — the coroutine-internals tests here
// only care about *whether* the dialog was triggered, not how it was
// styled.
class FakeDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  promise<MessageBoxResult> RunMessageBox(std::u16string_view /*message*/,
                                          std::u16string_view /*title*/,
                                          MessageBoxMode mode) override {
    ++message_box_calls;
    last_mode = mode;
    return make_resolved_promise(MessageBoxResult::Ok);
  }

  promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view /*title*/) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }

  promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& /*params*/) override {
    return make_rejected_promise<std::filesystem::path>(std::exception{});
  }

  int message_box_calls = 0;
  MessageBoxMode last_mode = MessageBoxMode::Info;
};

// Minimal `MainWindowInterface` fake. None of the paths under test drive
// the main window — they return before calling `OpenView` — but the
// `OpenFileCommandContext.main_window` pointer still has to be non-null
// or `OpenFileAsync` throws immediately with no chance to exercise the
// branches we care about.
class FakeMainWindow : public MainWindowInterface {
 public:
  int GetMainWindowId() const override { return 0; }
  const Page& GetCurrentPage() const override { std::terminate(); }
  void OpenPage(const Page&) override {}
  void SetCurrentPageTitle(std::u16string_view) override {}
  void SaveCurrentPage() override {}
  void DeleteCurrentPage() override {}
  OpenedViewInterface* GetActiveView() const override { return nullptr; }
  OpenedViewInterface* GetActiveDataView() const override { return nullptr; }
  void ActivateView(const OpenedViewInterface&) override {}
  std::vector<OpenedViewInterface*> GetOpenedViews() const override {
    return {};
  }
  promise<OpenedViewInterface*> OpenView(const WindowDefinition&,
                                         bool) override {
    return make_resolved_promise<OpenedViewInterface*>(nullptr);
  }
  OpenedViewInterface* FindViewByType(std::string_view) const override {
    return nullptr;
  }
  void SplitView(OpenedViewInterface&, bool) override {}
};

// `OpenFileCommandImpl` does not use the file manager on any path that
// these tests exercise — the download-error branch is driven by
// rejecting `OpenView`, not by the file manager. A placeholder is
// enough to satisfy the member reference.
class DummyFileManager : public FileManager {
 public:
  promise<void> DownloadFileFromServer(
      const std::filesystem::path&) const override {
    return make_resolved_promise();
  }
};

}  // namespace

// Exercises the coroutine internals of `OpenFileCommandImpl`. The public
// `Execute()` entry still returns `promise<void>` for compatibility with
// the rest of the client, but the parse/branch/dialog pipeline is now
// expressed as a coroutine, so these tests drive `WaitPromise` to poll
// the executor between `co_await`s.
class OpenFileCommandTest : public ::testing::Test {
 protected:
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  FakeDialogService dialog_service_;
  FakeMainWindow main_window_;

  FileRegistry file_registry_;
  DummyFileManager file_manager_;

  OpenFileCommandImpl command_{file_registry_, file_manager_};

  // Build an ad-hoc file node. The filesystem util composes the file path
  // from `display_name` walking up `FileDirectoryType` parents, so a
  // single node whose type is `FileType` produces a path equal to its
  // display name — good enough to drive extension-based branching.
  NodeRef MakeFileNode(std::u16string display_name) {
    nodes_ = std::make_unique<StaticNodeService>();
    nodes_->Add({.node_id = scada::NodeId{42, 1},
                 .type_definition_id = filesystem::id::FileType,
                 .attributes = {.display_name = std::move(display_name)}});
    return nodes_->GetNode(scada::NodeId{42, 1});
  }

  std::unique_ptr<StaticNodeService> nodes_;
};

TEST_F(OpenFileCommandTest, Execute_UnknownExtensionShowsErrorDialog) {
  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u"foo.unknown"),
                                 .key_modifiers = {}};

  // The unknown-extension branch reports via the dialog and then the
  // outer catch rejects the promise with `StatusCode::Bad`. Only the
  // inner "Unknown file type" dialog should fire because the inner
  // branch `co_return`s instead of throwing.
  EXPECT_NO_THROW(WaitPromise(executor_, command_.Execute(context)));
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}

TEST_F(OpenFileCommandTest, Execute_EmptyFilePathShowsDownloadErrorDialog) {
  // `MakeFileNode` with an empty display name produces an empty
  // filesystem path, which short-circuits `OpenFileAsync` with an
  // exception. `ExecuteAsync` must convert that into the generic
  // "download failed" dialog and a rejected promise.
  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u""),
                                 .key_modifiers = {}};

  EXPECT_THROW(WaitPromise(executor_, command_.Execute(context)),
               scada::status_exception);
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}

TEST_F(OpenFileCommandTest, Execute_NullMainWindowShowsDownloadErrorDialog) {
  // `OpenFileAsync` throws for a null main window. The error path still
  // pops the download-failed dialog.
  OpenFileCommandContext context{.main_window = nullptr,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u"foo.unknown"),
                                 .key_modifiers = {}};

  EXPECT_THROW(WaitPromise(executor_, command_.Execute(context)),
               scada::status_exception);
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}

TEST_F(OpenFileCommandTest, Execute_MissingWorkplaceFileShowsInvalidFormatDialog) {
  // A `.workplace` file that doesn't exist on disk takes the
  // "invalid format" branch inside `OpenJsonFileAsync`, which pops its
  // own error dialog and `co_return`s so `ExecuteAsync`'s outer catch
  // does not fire. Only the inner dialog is expected.
  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(
                                     u"no-such-file.workplace"),
                                 .key_modifiers = {}};

  EXPECT_NO_THROW(WaitPromise(executor_, command_.Execute(context)));
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}
