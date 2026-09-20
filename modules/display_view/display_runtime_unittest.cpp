#include "display_view/display_runtime.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <stdlib.h>
#endif

// The client's half of the display C ABI (ADR 0013 phase 4), driven against the
// real runtime library when the configure named one.
//
// WHAT THIS COVERS THAT THE `display` PRODUCT'S OWN SUITE DOES NOT. That suite
// drives the ABI directly; this drives the CLIENT's loader over it, which is
// where the client-specific decisions live: how a library is found, what
// happens when none is, and that the ABI's borrowed strings are copied before
// the next call overwrites them. A bug in any of those is invisible from the
// other side of the boundary.

namespace {

// Points the loader at the library the configure named, before anything calls
// DisplayRuntime::Get() -- which loads once and answers the same way for the
// rest of the process, so this cannot be done inside a test case without making
// the outcome depend on test order.
//
// A global environment rather than a fixture for exactly that reason: gtest
// runs these before the first test, whichever suite that turns out to be.
class RuntimeUnderTest : public ::testing::Environment {
 public:
  void SetUp() override {
#ifdef SCADA_DISPLAY_RUNTIME_UNDER_TEST
    const char* library = SCADA_DISPLAY_RUNTIME_UNDER_TEST;
    if (!library || !*library)
      return;
#ifdef _WIN32
    _putenv_s("SCADA_DISPLAY_RUNTIME", library);
#else
    ::setenv("SCADA_DISPLAY_RUNTIME", library, /*overwrite=*/1);
#endif
#endif
  }
};

const ::testing::Environment* kRuntimeUnderTest =
    ::testing::AddGlobalTestEnvironment(new RuntimeUnderTest);

// Whether this build has a runtime to drive at all. A client configured without
// one is a supported configuration, so the cases that need a renderer say so
// rather than failing.
bool HaveRuntime() {
#ifdef SCADA_DISPLAY_RUNTIME_UNDER_TEST
  return *SCADA_DISPLAY_RUNTIME_UNDER_TEST != '\0';
#else
  return false;
#endif
}

std::filesystem::path FixtureDocument() {
#ifdef SCADA_DISPLAY_TEST_DOCUMENT
  return std::filesystem::path{SCADA_DISPLAY_TEST_DOCUMENT};
#else
  return {};
#endif
}

// ── The search ──────────────────────────────────────────────────────────────
//
// Asserted on the LIST rather than on what a machine happens to have, so the
// order is pinned without the test depending on anything being installed.

TEST(DisplayRuntimeSearchTest, TheOverrideComesFirst) {
  // A developer pointing the client at a build tree, or a test doing what the
  // environment above does, must win over anything installed beside the
  // executable -- otherwise a stale staged copy silently shadows the library
  // under test.
  const std::vector<std::filesystem::path> paths =
      DisplayRuntime::SearchPaths();
  ASSERT_FALSE(paths.empty());
  if (const char* override_path = std::getenv("SCADA_DISPLAY_RUNTIME")) {
    if (*override_path)
      EXPECT_EQ(paths.front(), std::filesystem::path{override_path});
  }
}

TEST(DisplayRuntimeSearchTest, LooksBesideTheExecutable) {
  // Beside the executable is where the packaging puts it, so it must be in the
  // list whether or not an override is set.
  const std::vector<std::filesystem::path> paths =
      DisplayRuntime::SearchPaths();
  bool named_a_library = false;
  for (const std::filesystem::path& path : paths) {
    if (path.filename().string().find("display_runtime") != std::string::npos)
      named_a_library = true;
  }
  EXPECT_TRUE(named_a_library);
}

// ── Loading ─────────────────────────────────────────────────────────────────

TEST(DisplayRuntimeTest, LoadsTheConfiguredRuntime) {
  if (!HaveRuntime())
    GTEST_SKIP() << "this client was configured without a display runtime";

  const DisplayRuntime* runtime = DisplayRuntime::Get();
  ASSERT_NE(runtime, nullptr) << DisplayRuntime::unavailable_reason();

  // Empty once one is loaded: a non-empty reason alongside a loaded runtime
  // would put a contradiction in front of the operator.
  EXPECT_TRUE(DisplayRuntime::unavailable_reason().empty());

  // What the library says it is. Not parsed -- the loader negotiates on
  // abi_version -- but it is what a log or an About box shows, so it must not
  // come back empty.
  EXPECT_FALSE(runtime->version().empty());
}

// ── The version handshake ───────────────────────────────────────────────────
//
// Driven against tables written HERE rather than against a library, because
// every case worth testing is a library that disagrees with this client, and
// the only such library in existence would have to be built to lie. The
// decision is a free function for exactly that reason; see the note on
// DisplayRuntimeTableRejection.
//
// What these pin is the DIRECTION of tolerance, which is the whole of phase 5's
// versioning answer: a host is built from source and a runtime is downloaded,
// so the runtime is the older of the pair, and an older runtime must keep
// working.

// A table this client would accept: its own ABI, its own size.
ScadaDisplayApi CurrentTable() {
  ScadaDisplayApi api{};
  api.abi_version = SCADA_DISPLAY_ABI_VERSION;
  api.struct_size = sizeof(ScadaDisplayApi);
  return api;
}

TEST(DisplayRuntimeVersionTest, AcceptsATableOfItsOwnAbi) {
  EXPECT_EQ(DisplayRuntimeTableRejection(CurrentTable()), "");
}

TEST(DisplayRuntimeVersionTest, RejectsATableAboveItsOwnAbi) {
  // Past this client's ABI the members are not the ones it thinks they are, so
  // there is nothing safe to read however large the table claims to be. A
  // library that has just been asked for "at or below" never answers this way,
  // which is why it is a rejection rather than a fallback.
  ScadaDisplayApi api = CurrentTable();
  api.abi_version = SCADA_DISPLAY_ABI_VERSION + 1;
  api.struct_size = sizeof(ScadaDisplayApi) * 2;

  const std::string rejection = DisplayRuntimeTableRejection(api);
  EXPECT_NE(rejection, "");
  // The operator is told which way round the mismatch is, because the action
  // differs: too new means update the client, too old means update the runtime.
  EXPECT_NE(rejection.find(std::to_string(SCADA_DISPLAY_ABI_VERSION + 1)),
            std::string::npos)
      << rejection;
}

TEST(DisplayRuntimeVersionTest, RejectsATableShorterThanItsOwnAbiDefines) {
  // Below `struct_size` the members are whatever follows in the library's
  // memory. Calling one is a jump to an address nobody set, so this is the one
  // check that has to happen before any call through the table.
  ScadaDisplayApi api = CurrentTable();
  api.struct_size = SCADA_DISPLAY_ABI_1_SIZE - 1;
  EXPECT_NE(DisplayRuntimeTableRejection(api), "");
}

TEST(DisplayRuntimeVersionTest, AcceptsATableLongerThanThisClientKnows) {
  // A NEWER runtime serving this client's ABI is allowed to have appended
  // members this build has never heard of -- that is the prefix rule, and
  // refusing it would break the pairing the other way round.
  ScadaDisplayApi api = CurrentTable();
  api.struct_size = sizeof(ScadaDisplayApi) + 64;
  EXPECT_EQ(DisplayRuntimeTableRejection(api), "");
}

TEST(DisplayRuntimeVersionTest, RejectsATableClaimingNoAbiAtAll) {
  // Zero is what a default-initialised struct holds, so a library that
  // exported its table without ever filling it in presents this way. There is
  // no version 0, so no size can make it readable.
  ScadaDisplayApi api = CurrentTable();
  api.abi_version = 0;
  api.struct_size = sizeof(ScadaDisplayApi);
  EXPECT_NE(DisplayRuntimeTableRejection(api), "");
}

TEST(DisplayRuntimeVersionTest,
     TheAbiOneSizeIsMeasuredFromTheLastAbiOneMember) {
  // The size a served ABI-1 table is measured against must be the one ABI 1
  // defined, and it must not move when a later ABI appends a member. With one
  // version the two coincide -- which is why this is worth stating now, since
  // no build can yet tell a correct implementation from a wrong one.
  EXPECT_EQ(SCADA_DISPLAY_ABI_1_SIZE, sizeof(ScadaDisplayApi));
  EXPECT_EQ(SCADA_DISPLAY_ABI_1_SIZE,
            offsetof(ScadaDisplayApi, runtime_version) +
                sizeof(ScadaDisplayApi::runtime_version));
}

// ── Documents ───────────────────────────────────────────────────────────────

class DisplayRuntimeDocumentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!HaveRuntime())
      GTEST_SKIP() << "this client was configured without a display runtime";
    if (FixtureDocument().empty() ||
        !std::filesystem::exists(FixtureDocument())) {
      GTEST_SKIP() << "the fixture document is not in this build";
    }

    DisplayError error;
    document_ = DisplayRuntimeDocument::Open(
        FixtureDocument(), DisplayDocumentKind::kAuto, &error);
    ASSERT_NE(document_, nullptr) << error.message;
  }

  std::unique_ptr<DisplayRuntimeDocument> document_;
};

TEST_F(DisplayRuntimeDocumentTest, ReportsATitleAndAUsablePage) {
  EXPECT_FALSE(document_->Title().empty());

  // Never zero, whatever the document says: the widget divides by these to map
  // a click into page coordinates, and the ABI promises a positive default for
  // a document whose page metrics are unusable.
  const DisplayRect bounds = document_->PageBounds();
  EXPECT_GT(bounds.w, 0.0);
  EXPECT_GT(bounds.h, 0.0);
}

TEST_F(DisplayRuntimeDocumentTest, RendersIntoTheCallersBuffer) {
  constexpr int32_t kWidth = 120;
  constexpr int32_t kHeight = 90;
  std::vector<uint8_t> pixels(static_cast<size_t>(kWidth) * kHeight * 4, 0);

  DisplayError error;
  ASSERT_TRUE(
      document_->RenderBgra(pixels.data(), kWidth, kHeight, kWidth * 4, &error))
      << error.message;

  // Something other than the white page was drawn. A render that reported
  // success and left the buffer blank is the failure this catches, and it is
  // the shape a missing font database or a silently skipped shape takes.
  bool marked = false;
  for (size_t i = 0; i + 3 < pixels.size(); i += 4) {
    if (pixels[i] != 0xFF || pixels[i + 1] != 0xFF || pixels[i + 2] != 0xFF) {
      marked = true;
      break;
    }
  }
  EXPECT_TRUE(marked);
}

TEST_F(DisplayRuntimeDocumentTest, HitStringsSurviveTheNextHitTest) {
  // The ABI's shape strings belong to the DOCUMENT and the next hit_test
  // replaces them, so the loader copies them out. This is the case that fails
  // if it ever stops: hit one shape, keep the result, hit somewhere else, and
  // read the first one back.
  const DisplayRect bounds = document_->PageBounds();

  std::optional<DisplayShapeHit> first;
  for (int step = 1; step < 20 && !first; ++step) {
    const double fraction = step / 20.0;
    first = document_->HitTest(bounds.w * fraction, bounds.h * fraction);
  }
  if (!first)
    GTEST_SKIP() << "the fixture has no shape on its diagonal";

  const std::string name_before = first->name;
  const int32_t id_before = first->id;

  // Somewhere else, hit or miss -- either way the ABI is free to rewrite its
  // buffers.
  for (int step = 1; step < 20; ++step)
    document_->HitTest(bounds.w * (step / 20.0), bounds.h * 0.97);

  EXPECT_EQ(first->name, name_before);
  EXPECT_EQ(first->id, id_before);
}

TEST_F(DisplayRuntimeDocumentTest, MissingDocumentsReportRatherThanThrow) {
  DisplayError error;
  const std::unique_ptr<DisplayRuntimeDocument> absent =
      DisplayRuntimeDocument::Open(
          FixtureDocument().parent_path() / "definitely-not-here.vds",
          DisplayDocumentKind::kAuto, &error);
  EXPECT_EQ(absent, nullptr);
  EXPECT_NE(error.code, 0);
  EXPECT_FALSE(error.message.empty());
}

TEST_F(DisplayRuntimeDocumentTest, StateColouringChangesWhatIsDrawn) {
  // The palette and the per-source state are the only part of the ABI whose
  // effect is visible rather than reported, so the assertion is on the pixels:
  // render once as authored, then again with every data source driven, and
  // require the two to differ. Which shapes recolour is the renderer's
  // decision and deliberately not asserted here.
  constexpr int32_t kWidth = 160;
  constexpr int32_t kHeight = 120;
  const size_t size = static_cast<size_t>(kWidth) * kHeight * 4;

  std::vector<uint8_t> authored(size, 0);
  DisplayError error;
  ASSERT_TRUE(document_->RenderBgra(authored.data(), kWidth, kHeight,
                                    kWidth * 4, &error))
      << error.message;

  const DisplayStatePalette palette{
      .sl_live = 0xE6B24B,
      .sl_energized = 0x8FA3B4,
      .sl_closed = 0x44C091,
      .sl_open = 0x8FA3A8,
      .bad = 0xD64545,
      .uncertain = 0xC8A03C,
  };
  document_->SetStatePalette(&palette);

  // Every shape the fixture names, found by sweeping the page rather than by
  // knowing the document: this test must not encode one fixture's tag names.
  const DisplayRect bounds = document_->PageBounds();
  int driven = 0;
  for (int x = 1; x < 20; ++x) {
    for (int y = 1; y < 20; ++y) {
      const std::optional<DisplayShapeHit> hit =
          document_->HitTest(bounds.w * (x / 20.0), bounds.h * (y / 20.0));
      if (hit && !hit->data_source.empty()) {
        document_->SetDataSourceState(hit->data_source, "closed", 0);
        ++driven;
      }
    }
  }
  if (driven == 0)
    GTEST_SKIP() << "the fixture binds no data sources";

  std::vector<uint8_t> coloured(size, 0);
  ASSERT_TRUE(document_->RenderBgra(coloured.data(), kWidth, kHeight,
                                    kWidth * 4, &error))
      << error.message;

  EXPECT_NE(authored, coloured)
      << "driving every data source changed nothing on the diagram";
}

}  // namespace
