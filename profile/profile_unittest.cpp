#include "profile/profile.h"

#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"

#include <boost/json.hpp>
#include <cstddef>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Backlog 1014: an account-keyed file is separate from the shared one and from
// every other account's, including accounts whose ids differ only in a byte
// the file name cannot carry.
TEST(ProfileTest, AccountKeyedFilesAreSeparate) {
  scada::base::ScopedPathOverride private_dir{client::DIR_PRIVATE};

  Profile shared;
  shared.event_play_sound = false;
  shared.Save();

  Profile account;
  account.SetAccountKey("ns=1;i=100");
  EXPECT_FALSE(account.HasFile());
  account.Save();
  EXPECT_TRUE(account.HasFile());

  Profile reloaded;
  reloaded.SetAccountKey("ns=1;i=100");
  reloaded.Load();
  EXPECT_TRUE(reloaded.event_play_sound);

  // ';' and '_' must not collapse to one name.
  Profile lookalike;
  lookalike.SetAccountKey("ns=1_i=100");
  EXPECT_FALSE(lookalike.HasFile());

  Profile shared_again;
  EXPECT_TRUE(shared_again.HasFile());
  shared_again.Load();
  EXPECT_FALSE(shared_again.event_play_sound);
}

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

// Speech is an account preference like the alarm tone, so it survives a
// save and a fresh Profile's load -- which is what every sign-in does. It was
// read and written by nothing until backlog 1046, so an operator who turned
// it off heard it again after the next sign-in.
TEST(ProfileTest, SpeechSurvivesARoundTripThroughAFreshProfile) {
  Profile before;
  ASSERT_TRUE(before.speech_enabled);
  before.speech_enabled = false;
  const boost::json::value saved = before.SaveToValue();
  ASSERT_TRUE(saved.is_object());
  EXPECT_EQ(saved.as_object().at("speech").as_bool(), false);

  Profile after;
  after.Load(saved);
  EXPECT_FALSE(after.speech_enabled);
}

// A profile saved before the key existed keeps the default (on).
TEST(ProfileTest, SpeechDefaultsOnWhenTheProfileHasNoKey) {
  Profile profile;
  profile.Load(boost::json::parse(R"({"soundOnEvents": true})"));
  EXPECT_TRUE(profile.speech_enabled);
}

// Backlog 552: a profile with no `soundOnEvents` key -- a fresh account, or
// one saved before the key existed -- sounds the alarm tone. A new operator
// must not start with a silent annunciator.
TEST(ProfileTest, AlarmToneDefaultsOnWhenTheProfileHasNoKey) {
  EXPECT_TRUE(Profile{}.event_play_sound);

  Profile profile;
  profile.Load(boost::json::parse(R"({"showWriteOk": true})"));
  EXPECT_TRUE(profile.event_play_sound);
}

// Backlog 552: the new default reaches absent keys only. An operator who
// switched the tone off -- or whose profile was saved while the default was
// off, which wrote the key -- keeps it off.
TEST(ProfileTest, AnExplicitAlarmToneSettingIsKept) {
  Profile off;
  off.Load(boost::json::parse(R"({"soundOnEvents": false})"));
  EXPECT_FALSE(off.event_play_sound);

  Profile on;
  on.Load(boost::json::parse(R"({"soundOnEvents": true})"));
  EXPECT_TRUE(on.event_play_sound);
}

// Switching the tone off survives a save and a fresh Profile's load, which is
// what every sign-in does: the key is written either way.
TEST(ProfileTest, AlarmToneOffSurvivesARoundTripThroughAFreshProfile) {
  Profile before;
  before.event_play_sound = false;
  const boost::json::value saved = before.SaveToValue();
  ASSERT_TRUE(saved.is_object());
  EXPECT_EQ(saved.as_object().at("soundOnEvents").as_bool(), false);

  Profile after;
  after.Load(saved);
  EXPECT_FALSE(after.event_play_sound);
}

// Control confirmation is per session by design: it is not written, so a
// fresh Profile -- every sign-in -- starts with it on again.
TEST(ProfileTest, ControlConfirmationIsNotCarriedAcrossSignIns) {
  Profile before;
  before.control_confirmation = false;
  const boost::json::value saved = before.SaveToValue();

  Profile after;
  after.Load(saved);
  EXPECT_TRUE(after.control_confirmation);
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

// One connection's namespace table: `uris[i]` is the URI it gave index i.
// Both connections below give the model's own namespaces their canonical
// indexes; past those, the server's namespaces arrive in its own order.
struct TestConnection {
  std::vector<std::string> uris;

  void Attach(Profile& profile) const {
    profile.SetNamespaceLookups(
        [this](std::string_view uri) -> std::optional<scada::NamespaceIndex> {
          for (std::size_t i = 0; i < uris.size(); ++i) {
            if (uris[i] == uri)
              return static_cast<scada::NamespaceIndex>(i);
          }
          return std::nullopt;
        },
        [this](scada::NamespaceIndex index) -> std::optional<std::string> {
          if (index >= uris.size() || uris[index].empty())
            return std::nullopt;
          return uris[index];
        });
  }
};

// A workspace saved against a third-party server reopens on the same nodes
// when the next connect hands out the server's namespaces in another order:
// the profile stores them by URI, never by this session's index
// (docs/product/foreign-node-references.md, phase 3).
TEST(ProfileTest, ForeignReferencesReopenOnTheSameNodesAfterAReorder) {
  constexpr scada::NamespaceIndex kFirst = 200;
  constexpr scada::NamespaceIndex kSecond = 201;
  TestConnection before;
  before.uris.resize(kSecond + 1);
  before.uris[kFirst] = "urn:plant";
  before.uris[kSecond] = "urn:boiler";
  TestConnection after;
  after.uris.resize(kSecond + 1);
  after.uris[kFirst] = "urn:boiler";
  after.uris[kSecond] = "urn:plant";

  Profile saving;
  before.Attach(saving);
  Page page;
  page.id = 1;
  WindowDefinition& window = page.AddWindow(WindowDefinition{"Graph"});
  // What MakeNodeIdFormula and NodeIdToScadaString write this session.
  window.AddItem("Item").SetString("path", "{NS200.Plant.Temperature}");
  window.AddItem("Item").SetString("path", "NS201.7");
  window.AddItem("Item").SetString("path", "TS.1379");
  window.AddItem("Cell").SetString("text", "={NS201.Pressure} * 2");
  saving.AddPage(page);
  saving.node_table.default_sort_property_id =
      scada::NodeId{std::string{"Plant.Unit"}, kFirst};

  const boost::json::value saved = saving.SaveToValue();
  const std::string text = boost::json::serialize(saved);
  EXPECT_NE(text.find("{nsu=urn:plant;s=Plant.Temperature}"), std::string::npos)
      << text;
  EXPECT_EQ(text.find("NS200"), std::string::npos) << text;
  EXPECT_EQ(text.find("NS201"), std::string::npos) << text;
  // Our own namespaces keep the form older Clients read.
  EXPECT_NE(text.find("\"TS.1379\""), std::string::npos) << text;

  Profile loading;
  after.Attach(loading);
  loading.Load(saved);

  ASSERT_EQ(loading.pages.size(), 1u);
  const WindowDefinition& reopened = loading.pages.begin()->second.GetWindow(0);
  ASSERT_EQ(reopened.items.size(), 4u);
  EXPECT_EQ(reopened.items[0].GetString("path"), "{NS201.Plant.Temperature}");
  EXPECT_EQ(reopened.items[1].GetString("path"), "NS200.7");
  EXPECT_EQ(reopened.items[2].GetString("path"), "TS.1379");
  EXPECT_EQ(reopened.items[3].GetString("text"), "={NS200.Pressure} * 2");
  EXPECT_EQ(loading.node_table.default_sort_property_id,
            (scada::NodeId{std::string{"Plant.Unit"}, kSecond}));
}

// A profile saved before the URI form keeps reading: its `NS<n>` references
// are this-session indexes still, as they always were.
TEST(ProfileTest, SessionScopedReferencesFromAnOlderProfileStillLoad) {
  Profile profile;
  profile.Load(boost::json::parse(R"({
    "pages": [{"id": 1, "windows": [{"type": "Graph", "id": 1,
      "items": [{"name": "Item", "path": "{NS200.Plant.Temperature}"}]}]}],
    "nodeTable": {"sort-property-id": "NS200.5"}
  })"));

  ASSERT_EQ(profile.pages.size(), 1u);
  EXPECT_EQ(
      profile.pages.begin()->second.GetWindow(0).items[0].GetString("path"),
      "{NS200.Plant.Temperature}");
  EXPECT_EQ(profile.node_table.default_sort_property_id,
            (scada::NodeId{5u, 200}));
}

}  // namespace
