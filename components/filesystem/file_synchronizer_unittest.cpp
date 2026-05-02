#include "filesystem/file_synchronizer.h"

#include "base/logger.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "model/filesystem_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/status_exception.h"

#include <fstream>
#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NodeId kFileNodeId{9001, 1};

std::filesystem::file_time_type ToFileTime(scada::DateTime time) {
  auto delta = time - scada::DateTime::UnixEpoch();
  return std::filesystem::file_time_type{
      std::chrono::microseconds(delta.InMicroseconds())};
}

std::string ReadFileContents(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  EXPECT_TRUE(input.is_open());
  return std::string{std::istreambuf_iterator<char>{input}, {}};
}

class ScopedTempDir {
 public:
  ScopedTempDir()
      : path_{std::filesystem::temp_directory_path() /
              std::filesystem::path{"file_synchronizer_unittest"}} {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
    std::filesystem::create_directories(path_, ec);
  }

  ~ScopedTempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

scada::NodeState MakeType(scada::NodeId node_id,
                          scada::NodeClass node_class) {
  return {.node_id = node_id, .node_class = node_class};
}

scada::NodeState MakeFileSystemRoot() {
  return {.node_id = filesystem::id::FileSystem,
          .node_class = scada::NodeClass::Object,
          .type_definition_id = filesystem::id::FileDirectoryType,
          .attributes = {.display_name = u""}};
}

scada::NodeState MakeFileNode(scada::DateTime last_update_time) {
  return {.node_id = kFileNodeId,
          .node_class = scada::NodeClass::Variable,
          .type_definition_id = filesystem::id::FileType,
          .parent_id = filesystem::id::FileSystem,
          .reference_type_id = scada::id::Organizes,
          .attributes = {.display_name = u"test.bin"},
          .properties = {{filesystem::id::FileType_LastUpdateTime,
                          last_update_time}}};
}

class FileSynchronizerTest : public Test {
 protected:
  FileSynchronizerTest()
      : node_service_{scada::services{.attribute_service =
                                          &attribute_service_}} {
    node_service_.Add(MakeType(filesystem::id::FileDirectoryType,
                               scada::NodeClass::ObjectType));
    node_service_.Add(
        MakeType(filesystem::id::FileType, scada::NodeClass::VariableType));
    node_service_.Add(MakeFileSystemRoot());
  }

  std::filesystem::path FilePath() const {
    return temp_dir_.path() / "test.bin";
  }

  void AddFile(scada::DateTime last_update_time) {
    node_service_.Add(MakeFileNode(last_update_time));
  }

  void StartSynchronizer() {
    synchronizer_.emplace(FileSynchronizerContext{
        .executor_ = executor_,
        .logger_ = NullLogger::GetInstance(),
        .node_service_ = node_service_,
        .root_dir_ = temp_dir_.path()});
  }

  ScopedTempDir temp_dir_;
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  StrictMock<scada::MockAttributeService> attribute_service_;
  StaticNodeService node_service_;
  std::optional<FileSynchronizer> synchronizer_;
};

}  // namespace

TEST_F(FileSynchronizerTest, DownloadsOutdatedFile) {
  const auto last_update_time =
      scada::DateTime::UnixEpoch() + scada::Duration::FromSeconds(10);
  const std::string contents = "downloaded";
  AddFile(last_update_time);

  EXPECT_CALL(attribute_service_,
              Read(/*context=*/_,
                   /*inputs=*/Pointee(ElementsAre(scada::ReadValueId{
                       .node_id = kFileNodeId,
                       .attribute_id = scada::AttributeId::Value})),
                   /*callback=*/_))
      .WillOnce(InvokeArgument<2>(
          scada::StatusCode::Good,
          std::vector{scada::MakeReadResult(scada::ByteString(
              contents.begin(), contents.end()))}));

  StartSynchronizer();
  Drain(executor_);

  EXPECT_EQ(ReadFileContents(FilePath()), contents);
  EXPECT_EQ(std::filesystem::last_write_time(FilePath()),
            ToFileTime(last_update_time));
}

TEST_F(FileSynchronizerTest, DownloadFailureDoesNotCreateFile) {
  const auto last_update_time =
      scada::DateTime::UnixEpoch() + scada::Duration::FromSeconds(10);
  AddFile(last_update_time);

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(InvokeArgument<2>(scada::StatusCode::Bad,
                                  std::vector<scada::DataValue>{}));

  StartSynchronizer();
  Drain(executor_);

  EXPECT_FALSE(std::filesystem::exists(FilePath()));
}

TEST_F(FileSynchronizerTest, ActualFileSkipsDownload) {
  const auto last_update_time =
      scada::DateTime::UnixEpoch() + scada::Duration::FromSeconds(10);
  AddFile(last_update_time);

  const std::string contents = "cached";
  std::ofstream{FilePath(), std::ios::binary}.write(contents.data(),
                                                    contents.size());
  std::filesystem::last_write_time(FilePath(), ToFileTime(last_update_time));

  EXPECT_CALL(attribute_service_, Read(_, _, _)).Times(0);

  StartSynchronizer();
  Drain(executor_);

  EXPECT_EQ(ReadFileContents(FilePath()), contents);
  EXPECT_EQ(std::filesystem::last_write_time(FilePath()),
            ToFileTime(last_update_time));
}
