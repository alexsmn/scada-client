#include "user_access/password_policy.h"

#include "scada/authorization.h"

#include <gtest/gtest.h>

namespace {

PasswordPolicy Policy(double min_length,
                      double max_length,
                      scada::PasswordOptions options =
                          scada::PasswordOptions::kNone) {
  return PasswordPolicy{.min_length = min_length,
                        .max_length = max_length,
                        .options = options};
}

// A non-positive bound is "unconstrained", which is what a server with no
// length policy publishes — NOT a zero-length requirement. Reading it as a
// rule would reject every password on a server that imposes none.
TEST(PasswordPolicy, UnsetBoundsImposeNoLength) {
  EXPECT_EQ(PasswordPolicyViolation(Policy(0, 0), u""), nullptr);
  EXPECT_EQ(PasswordPolicyViolation(Policy(0, 0), u"x"), nullptr);
}

TEST(PasswordPolicy, EnforcesTheLengthRange) {
  const PasswordPolicy policy = Policy(8, 16);

  EXPECT_STREQ(PasswordPolicyViolation(policy, u"short"),
               "The password is too short");
  EXPECT_EQ(PasswordPolicyViolation(policy, u"just-right-1"), nullptr);
  EXPECT_STREQ(
      PasswordPolicyViolation(policy, u"far-too-long-to-be-accepted-here"),
      "The password is too long");
}

TEST(PasswordPolicy, EnforcesTheRequiredCharacterClasses) {
  const PasswordPolicy upper =
      Policy(0, 0, scada::PasswordOptions::kRequiresUpperCaseCharacters);
  EXPECT_STREQ(PasswordPolicyViolation(upper, u"lower"),
               "The password needs an upper-case letter");
  EXPECT_EQ(PasswordPolicyViolation(upper, u"Upper"), nullptr);

  const PasswordPolicy digit =
      Policy(0, 0, scada::PasswordOptions::kRequiresDigitCharacters);
  EXPECT_STREQ(PasswordPolicyViolation(digit, u"none"),
               "The password needs a digit");
  EXPECT_EQ(PasswordPolicyViolation(digit, u"has1"), nullptr);
}

// The special-character test is deliberately "not a letter or digit" rather
// than an ASCII punctuation list: these deployments carry Cyrillic names, and
// rejecting a non-ASCII symbol the SERVER accepts would block a valid password.
TEST(PasswordPolicy, ANonAsciiSymbolCountsAsSpecial) {
  const PasswordPolicy policy =
      Policy(0, 0, scada::PasswordOptions::kRequiresSpecialCharacters);

  EXPECT_STREQ(PasswordPolicyViolation(policy, u"plain123"),
               "The password needs a special character");
  EXPECT_EQ(PasswordPolicyViolation(policy, u"plain!"), nullptr);
  EXPECT_EQ(PasswordPolicyViolation(policy, u"plain§"), nullptr);
}

// Only the kRequires* bits are policy; the kSupport* bits describe what the
// server implements and must never be read as a password rule.
TEST(PasswordPolicy, SupportBitsAreNotRequirements) {
  const PasswordPolicy policy =
      Policy(0, 0, scada::PasswordOptions::kSupportDisableUser |
                       scada::PasswordOptions::kSupportDescriptionForUser);

  EXPECT_EQ(PasswordPolicyViolation(policy, u"a"), nullptr);
  for (const PasswordRequirement& requirement :
       PasswordRequirementsFor(policy.options)) {
    EXPECT_FALSE(requirement.required) << requirement.label;
  }
}

TEST(PasswordPolicy, RequirementBreakdownReflectsTheMask) {
  const auto requirements = PasswordRequirementsFor(
      scada::PasswordOptions::kRequiresUpperCaseCharacters |
      scada::PasswordOptions::kRequiresDigitCharacters);

  ASSERT_EQ(requirements.size(), 4u);
  EXPECT_TRUE(requirements[0].required);   // upper
  EXPECT_FALSE(requirements[1].required);  // lower
  EXPECT_TRUE(requirements[2].required);   // digit
  EXPECT_FALSE(requirements[3].required);  // special
}

}  // namespace
