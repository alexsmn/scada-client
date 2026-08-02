#include "administration/administration_sections.h"

#include "resources/common_resources.h"

#include <algorithm>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

namespace {

std::vector<unsigned> CommandIds(
    std::span<const AdministrationSection> sections) {
  std::vector<unsigned> ids;
  for (const AdministrationSection& section : sections) {
    ids.push_back(section.command_id);
  }
  return ids;
}

TEST(AdministrationSectionsTest, EverySectionNamesACommandAndALabel) {
  const auto sections = GetAdministrationSections();
  ASSERT_FALSE(sections.empty());

  std::set<unsigned> seen;
  for (const AdministrationSection& section : sections) {
    // A section with no command cannot open anything: it would be a row that
    // states a capability the client does not have.
    EXPECT_NE(section.command_id, 0u);
    EXPECT_NE(section.label, nullptr);
    EXPECT_FALSE(std::string_view{section.label}.empty());
    EXPECT_TRUE(seen.insert(section.command_id).second)
        << "duplicate command " << section.command_id;
  }
}

// Users leads: the admin screen's subject is the account list, and the pane's
// first row is what the mode opens onto.
TEST(AdministrationSectionsTest, UsersComesFirst) {
  ASSERT_FALSE(GetAdministrationSections().empty());
  EXPECT_EQ(GetAdministrationSections().front().command_id, ID_USERS_VIEW);
}

// The pane shows only what the shell can actually open. This is what keeps a
// door out of the list while the surface behind it is still being built, and
// what makes the admin gate a single rule rather than one the pane repeats.
TEST(AdministrationSectionsTest, UnresolvableSectionsAreNotOffered) {
  const auto sections = ResolvableAdministrationSections(
      GetAdministrationSections(),
      [](unsigned command_id) { return command_id == ID_USERS_VIEW; });

  EXPECT_EQ(CommandIds(sections), (std::vector<unsigned>{ID_USERS_VIEW}));
}

// A session that can open nothing sees an empty pane rather than a list of
// rows that all fail.
TEST(AdministrationSectionsTest, NothingResolvableYieldsNoSections) {
  EXPECT_TRUE(ResolvableAdministrationSections(GetAdministrationSections(),
                                               [](unsigned) { return false; })
                  .empty());
}

TEST(AdministrationSectionsTest, ResolvingEverythingPreservesOrder) {
  const auto sections = ResolvableAdministrationSections(
      GetAdministrationSections(), [](unsigned) { return true; });

  EXPECT_EQ(CommandIds(sections), CommandIds(GetAdministrationSections()));
}

}  // namespace
