// A control review is only worth capturing if it reviews a change; this pins
// that without rendering anything.

#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_wait.h"

#include "app/client_application.h"
#include "aui/translation.h"
#include "base/utf_convert.h"
#include "model/node_id_util.h"
#include "modules/write/write_model.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <span>
#include <string>

namespace {

using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;

}  // namespace

// A control review is worth capturing only if it reviews a change. The
// prompt quotes the point's present reading beside the value the command
// would write, and both are rendered by the item's own formatting — so a
// commanded value that formats to the present reading produces a dialog
// showing «Вкл» commanding «Вкл», which is a well-formed PNG of an operator
// being asked to confirm nothing. check_screenshots.py cannot see it: the
// file exists and has the right dimensions.
//
// A discrete item is where this actually bites, because `command_value` is
// the item's raw state while WriteModel labels states by inverting it
// (`get_or(true) ? 0 : 1`). Raw 0 is therefore the *second* label, so the
// obvious reading of "command the other state" is off by one and renders the
// state the point is already in.
TEST_F(ScreenshotGenerator, ControlConfirmationsReviewARealChange) {
  // Straight out of the fixture rather than FixtureConfig().dialogs, which is
  // filtered by the managed-image gate and by --only.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  int reviewed = 0;
  for (const auto& js : FixtureConfig().json.at("dialogs").as_array()) {
    if (js.at("kind").as_string() != "control-confirm")
      continue;
    const std::string filename(js.at("filename").as_string());
    SCOPED_TRACE(filename);

    scada::NodeId node_id = FixtureConfig().dialog_analog_node_id;
    if (const auto* node = js.as_object().if_contains("node"))
      node_id = NodeIdFromScadaString(std::string_view(node->as_string()));
    ASSERT_FALSE(node_id.is_null());
    ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
        executor_, app_.node_service(),
        std::span<const scada::NodeId>{&node_id, 1}));

    // Mirrors BuildControlConfirmation's default; the two have to agree or
    // this test reviews a value the capture never renders.
    double commanded = 12.5;
    if (const auto* value = js.as_object().if_contains("command_value"))
      commanded = value->to_number<double>();
    bool second_stage = false;
    if (const auto* stage = js.as_object().if_contains("second_stage"))
      second_stage = stage->as_bool();

    Profile profile;
    auto model = std::make_shared<WriteModel>(
        WriteContext{.executor_ = executor_,
                     .timed_data_service_ = app_.timed_data_service(),
                     .node_id_ = node_id,
                     .profile_ = profile,
                     .manual_ = false});
    // The present reading arrives over TimedDataService, so it lands on a
    // later turn of the loop. Without this the assertion below compares
    // against the get_or default and passes on a dialog that would render
    // wrongly.
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds{500});

    const std::u16string message =
        model->GetConfirmationMessage(commanded, second_stage);
    // Read the two quoted values back out of the real prompt rather than
    // recomputing them: that is what the reader of the manual sees, and it
    // keeps the check working whichever way WriteModel formats a value.
    auto quoted_after = [&message](std::u16string_view label) {
      const size_t at = message.find(label);
      if (at == std::u16string::npos)
        return std::u16string();
      const size_t from = at + label.size();
      const size_t eol = message.find(u'\n', from);
      std::u16string text =
          message.substr(from, eol == std::u16string::npos ? eol : eol - from);
      const size_t first = text.find_first_not_of(u" \t");
      return first == std::u16string::npos ? std::u16string()
                                           : text.substr(first);
    };
    const std::u16string present = quoted_after(Translate("Present:"));
    const std::u16string command = quoted_after(Translate("Command:"));

    EXPECT_FALSE(present.empty())
        << "the prompt quotes no present reading, so the value never arrived";
    EXPECT_FALSE(command.empty())
        << "the prompt quotes no commanded value, so command_value does not "
           "render for this item";
    EXPECT_NE(present, command)
        << "the capture reviews a no-op: the command formats to the reading "
           "the point already shows ("
        << UtfConvert<char>(present) << ")";
    ++reviewed;
  }
  EXPECT_GT(reviewed, 0) << "the fixture lost every control-confirm capture";
}
