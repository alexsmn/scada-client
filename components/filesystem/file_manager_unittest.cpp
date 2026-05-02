#include "filesystem/file_manager_impl.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "base/test/awaitable_test.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "filesystem/file_util.h"
#include "model/filesystem_node_ids.h"
#include "scada/attribute_service_mock.h"
#include "scada/client.h"
#include "scada/status_exception.h"
#include "scada/view_service_mock.h"

#include <fstream>
#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

class FileManagerTest : public Test {
 protected:
  base::ScopedPathOverride public_dir_override_{client::DIR_PUBLIC};

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockViewService> view_service_;

  FileManagerImpl file_manager_{FileManagerContext{
      .executor_ = executor_,
      .scada_client_ = scada::client{
          scada::services{.attribute_service = &attribute_service_,
                          .view_service = &view_service_}}}};
};

namespace {

// Matches the three-element translate-browse-path input produced by the
// filesystem manager's `Organizes` walk for the path "some/long/path".
const auto kSomeLongPathBrowse = ElementsAre(scada::BrowsePath{
    .node_id = filesystem::id::FileSystem,
    .relative_path = {
        scada::RelativePathElement{.reference_type_id = scada::id::Organizes,
                                   .target_name = "some"},
        scada::RelativePathElement{.reference_type_id = scada::id::Organizes,
                                   .target_name = "long"},
        scada::RelativePathElement{.reference_type_id = scada::id::Organizes,
                                   .target_name = "path"}}});

}  // namespace

TEST_F(FileManagerTest, DownloadFileFromServer_SendsExpectedServerRequests) {
  std::filesystem::path path = "some/long/path";
  scada::NodeId file_node_id{111, 22};
  std::string file_contents = "hello";

  EXPECT_CALL(
      attribute_service_,
      Read(/*context=*/_,
           /*inputs=*/Pointee(ElementsAre(scada::ReadValueId{file_node_id})),
           /*callback=*/_))
      .WillOnce(InvokeArgument<2>(
          scada::StatusCode::Good,
          std::vector{scada::MakeReadResult(scada::ByteString(
              std::begin(file_contents), std::end(file_contents)))}));

  EXPECT_CALL(view_service_,
              TranslateBrowsePaths(/*inputs=*/kSomeLongPathBrowse,
                                   /*callback=*/_))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Good,
                                  std::vector{scada::BrowsePathResult{
                                      .targets = {scada::BrowsePathTarget{
                                          .target_id = file_node_id}}}}));

  WaitPromise(executor_, file_manager_.DownloadFileFromServer(path));

  auto public_path = GetPublicFilePath(path);
  std::ifstream ifs{public_path};
  std::string actual_contents{std::istreambuf_iterator<char>{ifs}, {}};
  ASSERT_TRUE(ifs.is_open());
  EXPECT_EQ(actual_contents, file_contents);
}

TEST_F(FileManagerTest, DownloadFileFromServer_TranslateBrowsePathFails) {
  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(InvokeArgument<1>(
          scada::StatusCode::Bad,
          std::vector<scada::BrowsePathResult>{}));

  EXPECT_THROW(WaitPromise(executor_, file_manager_.DownloadFileFromServer("some/long/path")),
               scada::status_exception);
}

TEST_F(FileManagerTest, DownloadFileFromServer_TranslateBrowsePathReturnsNoTarget) {
  // A successful translate that resolves to zero (or more than one) target
  // must surface as a rejection, not silently succeed.
  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(InvokeArgument<1>(
          scada::StatusCode::Good,
          std::vector{scada::BrowsePathResult{.targets = {}}}));

  EXPECT_THROW(WaitPromise(executor_, file_manager_.DownloadFileFromServer("some/long/path")),
               scada::status_exception);
}

TEST_F(FileManagerTest, DownloadFileFromServer_ReadFails) {
  const scada::NodeId file_node_id{111, 22};

  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Good,
                                  std::vector{scada::BrowsePathResult{
                                      .targets = {scada::BrowsePathTarget{
                                          .target_id = file_node_id}}}}));

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(InvokeArgument<2>(scada::StatusCode::Bad,
                                  std::vector<scada::DataValue>{}));

  EXPECT_THROW(WaitPromise(executor_, file_manager_.DownloadFileFromServer("some/long/path")),
               scada::status_exception);
}

TEST_F(FileManagerTest, DownloadFileFromServer_WrongValueType) {
  // Server returns a value of the wrong type (not ByteString) — caller
  // must see the rejection rather than a silently-written empty file.
  const scada::NodeId file_node_id{111, 22};

  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Good,
                                  std::vector{scada::BrowsePathResult{
                                      .targets = {scada::BrowsePathTarget{
                                          .target_id = file_node_id}}}}));

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(InvokeArgument<2>(
          scada::StatusCode::Good,
          std::vector{scada::MakeReadResult(std::string{"not a byte string"})}));

  EXPECT_THROW(WaitPromise(executor_, file_manager_.DownloadFileFromServer("some/long/path")),
               scada::status_exception);
}

TEST_F(FileManagerTest, DownloadFileFromServer_EmptyPathRejected) {
  // An empty path never reaches the server — reject at the coroutine
  // boundary instead of sending a translate-browse-path with no elements.
  EXPECT_THROW(WaitPromise(executor_, file_manager_.DownloadFileFromServer(std::filesystem::path{})),
               scada::status_exception);
}
