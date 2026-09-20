#include "profile/profile.h"

#include <boost/json.hpp>
#include <gtest/gtest.h>

namespace {

// Regression (backlog 717): a profile.json holding valid JSON that is not an
// object used to become `data_` as-is. Every reader guards is_object(), but
// SerializeToValue writes through as_object(), which throws -- from
// Profile::Save() in ~ClientApplication, where it is std::terminate.
TEST(ProfileTest, NonObjectRootIsIgnoredAndSaveStillProducesAnObject) {
  const boost::json::value roots[] = {
      boost::json::value{boost::json::array{}},
      boost::json::value{nullptr},
      boost::json::value{1},
      boost::json::value{"x"},
  };
  for (const boost::json::value& root : roots) {
    Profile profile;
    profile.Load(root);
    EXPECT_TRUE(profile.data().is_object()) << root;

    boost::json::value saved;
    EXPECT_NO_THROW(saved = profile.SaveToValue()) << root;
    EXPECT_TRUE(saved.is_object()) << root;
  }
}

TEST(ProfileTest, ObjectRootIsLoadedAndRoundTrips) {
  Profile profile;
  profile.Load(
      boost::json::parse(R"({"showWriteOk": false, "topology": false})"));
  EXPECT_FALSE(profile.show_write_ok);
  EXPECT_FALSE(profile.modus.topology);

  const boost::json::value saved = profile.SaveToValue();
  ASSERT_TRUE(saved.is_object());
  EXPECT_EQ(saved.as_object().at("showWriteOk").as_bool(), false);
  EXPECT_EQ(saved.as_object().at("topology").as_bool(), false);
}

// A profile written by a client that still had the «Use Modus runtime
// renderer» flag loads unchanged, and the retired key is carried through
// rather than deleted: SerializeToValue starts from the loaded document, so a
// key this client no longer recognises survives the round trip. Backlog 491.
TEST(ProfileTest, ARetiredModus2KeyIsCarriedThroughUnread) {
  Profile profile;
  EXPECT_NO_THROW(profile.Load(
      boost::json::parse(R"({"modus2": true, "topology": false})")));
  EXPECT_FALSE(profile.modus.topology);

  const boost::json::value saved = profile.SaveToValue();
  ASSERT_TRUE(saved.is_object());
  EXPECT_EQ(saved.as_object().at("modus2").as_bool(), true);
}

}  // namespace
