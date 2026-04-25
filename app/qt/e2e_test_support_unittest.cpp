#include "app/qt/e2e_test_support.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace client {
namespace {

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

class E2eTestSupportTest : public testing::Test {
 public:
  void SetUp() override {
    report_path_ = std::filesystem::temp_directory_path() /
                   ("scada_operator_use_cases_" +
                    std::to_string(std::chrono::steady_clock::now()
                                       .time_since_epoch()
                                       .count()) +
                    ".txt");
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(report_path_, ec);
  }

 protected:
  OperatorUseCaseSmokeContext MakeContext() {
    return OperatorUseCaseSmokeContext{
        .executor = executor_,
        .open_window =
            [this](std::string_view window_type) {
              opened_windows_.emplace_back(window_type);
              return make_resolved_promise(
                  OperatorUseCaseSmokeResult{.ok = true, .detail = "opened"});
            },
        .is_window_registered = [](std::string_view window_type) {
          return window_type == "Registered";
        },
        .has_selection_command = [](unsigned command_id) {
          return command_id == 11;
        },
        .has_global_command = [](unsigned command_id) {
          return command_id == 22;
        },
        .has_main_window_command = [](unsigned command_id) {
          return command_id == 33;
        },
        .is_window_printable = [](std::string_view window_type) {
          return window_type == "Printable";
        },
    };
  }

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  std::filesystem::path report_path_;
  std::vector<std::string> opened_windows_;
};

}  // namespace

TEST_F(E2eTestSupportTest, OperatorUseCaseSmokeWritesSuccessfulReport) {
  std::vector<OperatorUseCaseSmokeCheck> checks{
      {.id = "UC-X",
       .description = "exercise test seam",
       .open_window_types = {"OpenMe"},
       .registered_window_types = {"Registered"},
       .registered_selection_commands = {11},
       .registered_global_commands = {22},
       .main_window_commands = {33},
       .printable_window_types = {"Printable"}}};

  WaitPromise(executor_, RunE2eOperatorUseCaseSmoke(
                             MakeContext(), report_path_, std::move(checks)));

  EXPECT_EQ(opened_windows_, std::vector<std::string>{"OpenMe"});
  const auto report = ReadFile(report_path_);
  EXPECT_NE(report.find("operator-use-cases: ok"), std::string::npos);
  EXPECT_NE(report.find("OpenMe ok opened"), std::string::npos);
  EXPECT_NE(report.find("UC-X ok exercise test seam registered Registered"),
            std::string::npos);
  EXPECT_NE(report.find(" command 11"), std::string::npos);
  EXPECT_NE(report.find(" global-command 22"), std::string::npos);
  EXPECT_NE(report.find(" main-window-command 33"), std::string::npos);
  EXPECT_NE(report.find(" printable Printable"), std::string::npos);
}

TEST_F(E2eTestSupportTest, OperatorUseCaseSmokeRecordsOpenWindowFailure) {
  auto context = MakeContext();
  context.open_window = [this](std::string_view window_type) {
    opened_windows_.emplace_back(window_type);
    return make_resolved_promise(
        OperatorUseCaseSmokeResult{.ok = false,
                                   .detail = "open returned null"});
  };

  std::vector<OperatorUseCaseSmokeCheck> checks{
      {.id = "UC-Y",
       .description = "window open failure",
       .open_window_types = {"BrokenWindow"}}};

  WaitPromise(executor_, RunE2eOperatorUseCaseSmoke(
                             std::move(context), report_path_,
                             std::move(checks)));

  EXPECT_EQ(opened_windows_, std::vector<std::string>{"BrokenWindow"});
  const auto report = ReadFile(report_path_);
  EXPECT_NE(report.find("operator-use-cases: failure"), std::string::npos);
  EXPECT_NE(report.find("BrokenWindow failure open returned null"),
            std::string::npos);
  EXPECT_NE(report.find("UC-Y ok window open failure"), std::string::npos);
}

TEST_F(E2eTestSupportTest, ObjectViewValuesCheckWritesSuccessfulReport) {
  WaitPromise(executor_, RunE2eObjectViewValuesCheck(
                             ObjectViewValuesCheckContext{
                                 .executor = executor_,
                                 .get_first_value_text =
                                     [] { return std::optional{u"value"}; },
                                 .timeout = std::chrono::milliseconds{0},
                                 .poll_interval = std::chrono::milliseconds{0},
                             },
                             report_path_));

  const auto report = ReadFile(report_path_);
  EXPECT_NE(report.find("object-view-values: ok"), std::string::npos);
  EXPECT_NE(report.find("value text present"), std::string::npos);
}

TEST_F(E2eTestSupportTest, ObjectViewValuesCheckWritesTimeoutReport) {
  WaitPromise(executor_, RunE2eObjectViewValuesCheck(
                             ObjectViewValuesCheckContext{
                                 .executor = executor_,
                                 .get_first_value_text =
                                     [] { return std::optional<std::u16string>{}; },
                                 .timeout = std::chrono::milliseconds{0},
                                 .poll_interval = std::chrono::milliseconds{0},
                             },
                             report_path_));

  const auto report = ReadFile(report_path_);
  EXPECT_NE(report.find("object-view-values: failure"), std::string::npos);
  EXPECT_NE(report.find("timed out waiting for value text"), std::string::npos);
}

TEST_F(E2eTestSupportTest, ObjectTreeLabelsCheckWritesSuccessfulReport) {
  WaitPromise(executor_, RunE2eObjectTreeLabelsCheck(
                             ObjectTreeLabelsCheckContext{
                                 .executor = executor_,
                                 .get_expanded_labels =
                                     [] {
                                       return std::vector<std::u16string>{
                                           u"Root", u"Area", u"Device", u"Point"};
                                     },
                                 .timeout = std::chrono::milliseconds{0},
                                 .poll_interval = std::chrono::milliseconds{0},
                             },
                             report_path_));

  const auto report = ReadFile(report_path_);
  EXPECT_NE(report.find("object-tree-labels: ok"), std::string::npos);
  EXPECT_NE(report.find("expanded first rendered path"), std::string::npos);
  EXPECT_NE(report.find("label[3]=Point"), std::string::npos);
}

TEST_F(E2eTestSupportTest, ObjectTreeLabelsCheckWritesTimeoutReport) {
  WaitPromise(executor_, RunE2eObjectTreeLabelsCheck(
                             ObjectTreeLabelsCheckContext{
                                 .executor = executor_,
                                 .get_expanded_labels =
                                     [] {
                                       return std::vector<std::u16string>{
                                           u"Root", u"[loading]"};
                                     },
                                 .timeout = std::chrono::milliseconds{0},
                                 .poll_interval = std::chrono::milliseconds{0},
                             },
                             report_path_));

  const auto report = ReadFile(report_path_);
  EXPECT_NE(report.find("object-tree-labels: failure"), std::string::npos);
  EXPECT_NE(report.find("timed out waiting for rendered labels"),
            std::string::npos);
  EXPECT_NE(report.find("label[1]=[loading]"), std::string::npos);
}

}  // namespace client
