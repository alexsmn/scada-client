#include "controller/command_registry.h"

#include <gtest/gtest.h>

namespace {

// A command with no reason handler says nothing — the default for every
// command that has not opted in.
TEST(CommandDisabledReasonTest, CommandWithoutHandlerHasNoReason) {
  CommandRegistry registry;
  registry.AddCommand(Command{1}.set_enabled_handler([] { return false; }));

  EXPECT_FALSE(registry.IsCommandEnabled(1));
  EXPECT_TRUE(registry.GetCommandDisabledReason(1).empty());
}

// An unknown command likewise, rather than asserting.
TEST(CommandDisabledReasonTest, UnknownCommandHasNoReason) {
  CommandRegistry registry;
  EXPECT_TRUE(registry.GetCommandDisabledReason(42).empty());
}

// The registry hands back whatever the command's own handler says, so the
// sentence stays with the rule that produced it.
TEST(CommandDisabledReasonTest, ReasonComesFromTheCommand) {
  CommandRegistry registry;
  registry.AddCommand(Command{1}
                          .set_enabled_handler([] { return false; })
                          .set_disabled_reason_handler(
                              [] { return std::u16string{u"no channel"}; }));

  EXPECT_EQ(registry.GetCommandDisabledReason(1), u"no channel");
}

// The reason is re-read on every ask, so it tracks the state that disabled the
// command rather than a snapshot taken at registration.
TEST(CommandDisabledReasonTest, ReasonFollowsTheCurrentState) {
  bool has_channel = false;
  CommandRegistry registry;
  registry.AddCommand(Command{1}
                          .set_enabled_handler([&] { return has_channel; })
                          .set_disabled_reason_handler([&] {
                            return has_channel ? std::u16string{}
                                               : std::u16string{u"no channel"};
                          }));

  EXPECT_EQ(registry.GetCommandDisabledReason(1), u"no channel");

  has_channel = true;
  EXPECT_TRUE(registry.IsCommandEnabled(1));
  EXPECT_TRUE(registry.GetCommandDisabledReason(1).empty());
}

}  // namespace
