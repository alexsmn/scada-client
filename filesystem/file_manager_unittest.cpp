#include "filesystem/file_manager_impl.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "base/test/scoped_path_override.h"
#include "filesystem/file_util.h"
#include "model/filesystem_node_ids.h"
#include "scada/attribute_service_mock.h"
#include "scada/client.h"
#include "scada/view_service_mock.h"

#include <fstream>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class FileManagerTest : public Test {
 protected:
  base::ScopedPathOverride public_dir_override_{client::DIR_PUBLIC};

  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockViewService> view_service_;

  FileManagerImpl file_manager_{
      FileManagerContext{.scada_client_ = scada::client{scada::services{
                             .attribute_service = &attribute_service_,
                             .view_service = &view_service_}}}};
};

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

  EXPECT_CALL(
      view_service_,
      /*inputs=*/TranslateBrowsePaths(
          ElementsAre(scada::BrowsePath{
              .node_id = filesystem::id::FileSystem,
              .relative_path = {scada::RelativePathElement{
                                    .reference_type_id = scada::id::Organizes,
                                    .target_name = "some"},
                                scada::RelativePathElement{
                                    .reference_type_id = scada::id::Organizes,
                                    .target_name = "long"},
                                scada::RelativePathElement{
                                    .reference_type_id = scada::id::Organizes,
                                    .target_name = "path"}}}),
          /*callback=*/_))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Good,
                                  std::vector{scada::BrowsePathResult{
                                      .targets = {scada::BrowsePathTarget{
                                          .target_id = file_node_id}}}}));

  file_manager_.DownloadFileFromServer(path).get();

  auto public_path = GetPublicFilePath(path);
  std::ifstream ifs{public_path};
  std::string actual_contents{std::istreambuf_iterator<char>{ifs}, {}};
  ASSERT_TRUE(ifs.is_open());
  EXPECT_EQ(actual_contents, file_contents);
}