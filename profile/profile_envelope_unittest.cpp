#include "profile/profile_envelope.h"

#include "profile/profile.h"

#include <boost/json.hpp>
#include <gtest/gtest.h>

namespace profile_envelope {
namespace {

// A minimal envelope of the shape the web client writes, with a `web` section
// this client must never touch.
boost::json::value MakeEnvelope() {
  return boost::json::parse(R"({
    "version": 1,
    "qt": {"profile": {"showWriteOk": false, "pages": []}},
    "web": {"portfolios": [{"name": "critical", "nodes": ["ns=2;s=A"]}]},
    "extras": {"fromANewerClient": 7}
  })");
}

TEST(ProfileEnvelopeTest, RecognisesAnEnvelopeByItsSections) {
  EXPECT_TRUE(IsEnvelope(MakeEnvelope()));
  // A section either client owns is enough: a profile written by the web
  // client before this one has ever saved carries `qt` with an empty profile,
  // and one written by a build that knew only the web section carries `web`.
  EXPECT_TRUE(IsEnvelope(boost::json::parse(R"({"qt": {"profile": {}}})")));
  EXPECT_TRUE(IsEnvelope(boost::json::parse(R"({"web": {}})")));
}

TEST(ProfileEnvelopeTest, DoesNotMistakeAFlatQtDocumentForAnEnvelope) {
  // The shape `Profile::SaveToValue` produces. Nothing under `client/` writes
  // a `qt` or `web` key, which is what makes the discrimination safe.
  EXPECT_FALSE(IsEnvelope(boost::json::parse(
      R"({"showWriteOk": true, "pages": [], "favorites": []})")));
  EXPECT_FALSE(IsEnvelope(boost::json::value{boost::json::object{}}));
  // A non-object cannot be an envelope, and must not crash the question.
  EXPECT_FALSE(IsEnvelope(boost::json::value{nullptr}));
  EXPECT_FALSE(IsEnvelope(boost::json::value{boost::json::array{}}));
}

TEST(ProfileEnvelopeTest, UnwrapReturnsTheQtSection) {
  const boost::json::value inner = Unwrap(MakeEnvelope());
  ASSERT_TRUE(inner.is_object());
  EXPECT_FALSE(inner.as_object().at("showWriteOk").as_bool());
  // And nothing of the envelope's own comes with it.
  EXPECT_FALSE(inner.as_object().contains("web"));
  EXPECT_FALSE(inner.as_object().contains("qt"));
}

TEST(ProfileEnvelopeTest, UnwrapPassesAFlatDocumentThrough) {
  const boost::json::value flat =
      boost::json::parse(R"({"showWriteOk": true, "pages": []})");
  EXPECT_EQ(Unwrap(flat), flat);
}

// An envelope with no Qt section must yield an empty object, NOT the envelope.
// Returning the envelope is the silent failure this codec exists to prevent:
// every key `Profile::Load` looks for would miss, the profile would come up
// empty, and Save() would write that emptiness back over the real one.
TEST(ProfileEnvelopeTest, UnwrapYieldsEmptyWhenThereIsNoQtSection) {
  for (const char* json : {R"({"web": {"portfolios": []}})", R"({"qt": {}})",
                           R"({"qt": {"profile": 5}})"}) {
    const boost::json::value inner = Unwrap(boost::json::parse(json));
    ASSERT_TRUE(inner.is_object()) << json;
    EXPECT_TRUE(inner.as_object().empty()) << json;
  }
}

// The reason this codec exists rather than a pair of inline lambdas: a client
// that writes its own document over the variable destroys the other client's
// settings, and nothing reports it -- the next read simply comes back without
// them.
TEST(ProfileEnvelopeTest, WrapPreservesEverySectionThisClientDoesNotOwn) {
  const boost::json::value flat =
      boost::json::parse(R"({"showWriteOk": true, "pages": [{"id": 1}]})");
  const boost::json::value wrapped = Wrap(flat, MakeEnvelope());

  ASSERT_TRUE(wrapped.is_object());
  const boost::json::object& out = wrapped.as_object();

  // The web section, untouched, down to its contents.
  const boost::json::value& web = out.at("web");
  EXPECT_EQ(web.at("portfolios").as_array().size(), 1u);
  EXPECT_EQ(web.at("portfolios").as_array()[0].at("name").as_string(),
            "critical");
  // And the forward-compat carry-over an unknown client left behind.
  EXPECT_EQ(out.at("extras").at("fromANewerClient").as_int64(), 7);

  // The Qt section is replaced, not merged: the old `showWriteOk` was false.
  EXPECT_TRUE(out.at("qt").at("profile").at("showWriteOk").as_bool());
  EXPECT_EQ(out.at("qt").at("profile").at("pages").as_array().size(), 1u);
}

TEST(ProfileEnvelopeTest, WrapKeepsQtOwnedKeysBesideProfile) {
  // A future key under `qt` but outside `profile` belongs to whoever wrote it,
  // the same way `web` does.
  const boost::json::value envelope =
      boost::json::parse(R"({"qt": {"profile": {}, "somethingElse": 1}})");
  const boost::json::value wrapped =
      Wrap(boost::json::parse(R"({"pages": []})"), envelope);
  EXPECT_EQ(wrapped.at("qt").at("somethingElse").as_int64(), 1);
}

TEST(ProfileEnvelopeTest, WrapBuildsAFreshEnvelopeWhenThereWasNone) {
  // The first save against a server that has never held a profile.
  for (const boost::json::value none :
       {boost::json::value{nullptr}, boost::json::value{boost::json::array{}},
        boost::json::value{"not an object"}}) {
    const boost::json::value wrapped =
        Wrap(boost::json::parse(R"({"pages": []})"), none);
    ASSERT_TRUE(wrapped.is_object());
    EXPECT_EQ(wrapped.at("version").as_int64(), 1);
    EXPECT_TRUE(wrapped.at("qt").at("profile").is_object());
    // Written unconditionally so a Qt-first document is not one the web side
    // has to special-case.
    EXPECT_TRUE(wrapped.at("extras").is_object());
  }
}

TEST(ProfileEnvelopeTest, WrapThenUnwrapIsTheIdentityOnTheQtDocument) {
  const boost::json::value flat = boost::json::parse(
      R"({"showWriteOk": false, "pages": [{"id": 3}], "favorites": []})");
  EXPECT_EQ(Unwrap(Wrap(flat, MakeEnvelope())), flat);
}

// The round trip that matters in production: a profile stored as an envelope
// opens, and saving it back preserves the other client's section.
TEST(ProfileEnvelopeTest, ProfileLoadsFromAnEnvelopeAndSurvivesTheRoundTrip) {
  Profile profile;
  profile.Load(MakeEnvelope());

  // `showWriteOk` is false in the envelope's Qt section and defaults true, so
  // reading it proves the Qt section was actually opened rather than skipped.
  EXPECT_FALSE(profile.show_write_ok);
  // `data_` is the FLAT document: an envelope here would put `showWriteOk`
  // beside `web` on the next save.
  EXPECT_FALSE(profile.data().as_object().contains("web"));

  const boost::json::value rewrapped =
      Wrap(profile.SaveToValue(), MakeEnvelope());
  EXPECT_EQ(rewrapped.at("web").at("portfolios").as_array().size(), 1u);
  EXPECT_FALSE(rewrapped.at("qt").at("profile").at("showWriteOk").as_bool());
}

// Handing `Profile::Load` an envelope used to be indistinguishable from handing
// it an empty profile -- and the difference only showed up later, when Save()
// wrote the emptiness back. Pinned because the failure has no symptom at the
// point it happens.
TEST(ProfileEnvelopeTest, AnEnvelopeIsNotLoadedAsAnEmptyProfile) {
  Profile with_envelope;
  with_envelope.Load(MakeEnvelope());

  Profile with_empty;
  with_empty.Load(boost::json::value{boost::json::object{}});

  EXPECT_NE(with_envelope.show_write_ok, with_empty.show_write_ok);
}

}  // namespace
}  // namespace profile_envelope
