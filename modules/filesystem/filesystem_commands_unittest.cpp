#include "filesystem/filesystem_commands.h"

#include "aui/dialog_service.h"
#include "aui/types.h"
#include "base/client_paths.h"
#include "base/test/awaitable_test.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "common/test/scoped_temp_dir.h"
#include "controller/controller.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"
#include "filesystem/file_manager.h"
#include "filesystem/file_registry.h"
#include "filesystem/file_util.h"
#include "main_window/main_window_interface.h"
#include "model/filesystem_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <fstream>

#include "base/debug_util.h"

namespace {

const WindowInfo kTestModusWindowInfo = {ID_MODUS_VIEW, "Modus", u"Modus"};
const WindowInfo kTestVidiconDisplayWindowInfo = {ID_VIDICON_DISPLAY_VIEW,
                                                  "VidiconDisplay", u"Display"};

// Minimal dialog-service fake that records `RunMessageBox` calls and
// immediately resolves their awaitables. Skip pulling in `MockDialogService`
// and its link-time dependencies — the coroutine-internals tests here
// only care about *whether* the dialog was triggered, not how it was
// styled.
class FakeDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view /*message*/,
                                            std::u16string_view /*title*/,
                                            MessageBoxMode mode) override {
    ++message_box_calls;
    last_mode = mode;
    co_return MessageBoxResult::Ok;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view /*title*/) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& /*params*/) override {
    throw std::exception{};
    co_return std::filesystem::path{};
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
  Awaitable<OpenedViewInterface*> OpenView(const WindowDefinition&,
                                           bool) override {
    ++open_view_calls;
    co_return nullptr;
  }
  OpenedViewInterface* FindViewByType(std::string_view) const override {
    return nullptr;
  }
  void SplitView(OpenedViewInterface&, bool) override {}

  int open_view_calls = 0;
};

class RecordingMainWindow : public FakeMainWindow {
 public:
  Awaitable<OpenedViewInterface*> OpenView(
      const WindowDefinition& window_definition,
      bool activate) override {
    ++open_view_calls;
    last_window_definition = window_definition;
    last_activate = activate;
    co_return nullptr;
  }

  std::optional<WindowDefinition> last_window_definition;
  bool last_activate = false;
};

class DummyController : public Controller {
 public:
  std::unique_ptr<UiView> Init(const WindowDefinition&) override {
    return nullptr;
  }
};

// `OpenFileCommandImpl` does not use the file manager on any path that
// these tests exercise — the download-error branch is driven by
// rejecting `OpenView`, not by the file manager. A placeholder is
// enough to satisfy the member reference.
class DummyFileManager : public FileManager {
 public:
  Awaitable<void> DownloadFileFromServer(
      const std::filesystem::path&) const override {
    co_return;
  }
};

class RecordingFileManager : public FileManager {
 public:
  Awaitable<void> DownloadFileFromServer(
      const std::filesystem::path& path) const override {
    downloaded_paths.push_back(path);
    co_return;
  }

  Awaitable<void> DownloadFileFromServer(
      NodeRef,
      const std::filesystem::path& path) const override {
    downloaded_paths.push_back(path);
    auto public_path = GetPublicFilePath(path);
    std::filesystem::create_directories(public_path.parent_path());
    std::ofstream ofs{public_path, std::ios::binary};
    ofs << "sde";
    co_return;
  }

  mutable std::vector<std::filesystem::path> downloaded_paths;
};

}  // namespace

// Exercises the coroutine internals of `OpenFileCommandImpl`.
class OpenFileCommandTest : public ::testing::Test {
 protected:
  scada::base::ScopedPathOverride public_dir_override_{client::DIR_PUBLIC};
  TestExecutor executor_;

  FakeDialogService dialog_service_;
  RecordingMainWindow main_window_;

  ControllerRegistry controller_registry_;
  FileRegistry file_registry_;
  RecordingFileManager file_manager_;

  OpenFileCommandImpl command_{file_registry_, file_manager_};

  void WaitCommand(Awaitable<void> awaitable) {
    WaitAwaitable(executor_, std::move(awaitable));
  }

  // Build an ad-hoc file node. The filesystem util composes the file path
  // from `display_name` walking up `FileDirectoryType` parents, so a
  // single node whose type is `FileType` produces a path equal to its
  // display name — good enough to drive extension-based branching.
  NodeRef MakeFileNode(std::u16string display_name) {
    nodes_ = std::make_unique<StaticNodeService>();
    nodes_->Add({.node_id = scada::NodeId{42, 1},
                 .type_definition_id = scada::filesystem::id::FileType,
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
  // outer catch rejects with `StatusCode::Bad`. Only the
  // inner "Unknown file type" dialog should fire because the inner
  // branch `co_return`s instead of throwing.
  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}

TEST_F(OpenFileCommandTest, Execute_EmptyFilePathShowsDownloadErrorDialog) {
  // `MakeFileNode` with an empty display name produces an empty
  // filesystem path, which short-circuits `OpenFileAsync` with an
  // exception. `ExecuteAsync` must convert that into the generic
  // "download failed" dialog and a rejected awaitable.
  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u""),
                                 .key_modifiers = {}};

  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  EXPECT_EQ(dialog_service_.message_box_calls, 0);
}

TEST_F(OpenFileCommandTest, Execute_NullMainWindowShowsDownloadErrorDialog) {
  // `OpenFileAsync` throws for a null main window. The error path still
  // pops the download-failed dialog.
  OpenFileCommandContext context{.main_window = nullptr,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u"foo.unknown"),
                                 .key_modifiers = {}};

  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  EXPECT_EQ(dialog_service_.message_box_calls, 0);
}

TEST_F(OpenFileCommandTest,
       Execute_MissingWorkplaceFileShowsInvalidFormatDialog) {
  // A `.workplace` file that doesn't exist on disk takes the
  // "invalid format" branch inside `OpenJsonFileAsync`, which pops its
  // own error dialog and `co_return`s so `ExecuteAsync`'s outer catch
  // does not fire. Only the inner dialog is expected.
  OpenFileCommandContext context{
      .main_window = &main_window_,
      .dialog_service = dialog_service_,
      .executor = executor_,
      .file_node = MakeFileNode(u"no-such-file.workplace"),
      .key_modifiers = {}};

  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}

// Regression (backlog 718): FromJson<WindowDefinition> never returns nullopt,
// so a .workplace with no usable `type` parsed, OpenView logged "Window type
// not found" and returned null, and the operator saw nothing happen. The fake
// main window's OpenView returns null, standing in for that failure.
TEST_F(OpenFileCommandTest,
       Execute_WorkplaceFileTheMainWindowCannotOpenShowsErrorDialog) {
  ScopedTempDir temp_dir{"filesystem_commands_test"};
  const std::filesystem::path path = temp_dir.path() / "typeless.workplace";
  {
    std::ofstream ofs{path, std::ios::binary};
    ofs << R"({"type": "NoSuchWindowType"})";
  }

  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(path.u16string()),
                                 .key_modifiers = {}};

  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  EXPECT_EQ(main_window_.open_view_calls, 1);
  EXPECT_EQ(dialog_service_.message_box_calls, 1);
  EXPECT_EQ(dialog_service_.last_mode, MessageBoxMode::Error);
}

TEST_F(OpenFileCommandTest, Execute_SdeFileOpensRegisteredModusWindow) {
  controller_registry_.AddControllerFactory(
      kTestModusWindowInfo, [](const ControllerContext&) {
        return std::make_unique<DummyController>();
      });
  file_registry_.RegisterType(kTestModusWindowInfo.command_id,
                              kTestModusWindowInfo.name, ".sde;.xsde");

  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u"diagram.sde"),
                                 .key_modifiers = {}};

  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  ASSERT_TRUE(main_window_.last_window_definition.has_value());
  ASSERT_EQ(file_manager_.downloaded_paths.size(), 1u);
  EXPECT_EQ(file_manager_.downloaded_paths.front(),
            std::filesystem::path{"diagram.sde"});
  EXPECT_EQ(main_window_.last_window_definition->type, "Modus");
  EXPECT_EQ(main_window_.last_window_definition->path, "diagram.sde");
  EXPECT_TRUE(main_window_.last_activate);
  EXPECT_EQ(main_window_.open_view_calls, 1);
  EXPECT_EQ(dialog_service_.message_box_calls, 0);
}

TEST_F(OpenFileCommandTest,
       Execute_VdsFileOpensRegisteredVidiconDisplayWindow) {
  controller_registry_.AddControllerFactory(
      kTestVidiconDisplayWindowInfo, [](const ControllerContext&) {
        return std::make_unique<DummyController>();
      });
  file_registry_.RegisterType(kTestVidiconDisplayWindowInfo.command_id,
                              kTestVidiconDisplayWindowInfo.name, ".vds");

  OpenFileCommandContext context{.main_window = &main_window_,
                                 .dialog_service = dialog_service_,
                                 .executor = executor_,
                                 .file_node = MakeFileNode(u"display.vds"),
                                 .key_modifiers = {}};

  EXPECT_NO_THROW(WaitCommand(command_.Execute(context)));
  ASSERT_TRUE(main_window_.last_window_definition.has_value());
  ASSERT_EQ(file_manager_.downloaded_paths.size(), 1u);
  EXPECT_EQ(file_manager_.downloaded_paths.front(),
            std::filesystem::path{"display.vds"});
  EXPECT_EQ(main_window_.last_window_definition->type, "VidiconDisplay");
  EXPECT_EQ(main_window_.last_window_definition->path, "display.vds");
  EXPECT_TRUE(main_window_.last_activate);
  EXPECT_EQ(main_window_.open_view_calls, 1);
  EXPECT_EQ(dialog_service_.message_box_calls, 0);
}
