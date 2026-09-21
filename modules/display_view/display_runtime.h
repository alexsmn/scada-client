#pragma once

#include "display/abi/display_abi.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The client's half of the `display` C ABI (ADR 0013 phase 4).
//
// The schematic renderer arrives as a shared library loaded at run time, not as
// source, because `display`'s SOURCE is never published and the public Qt
// client must still build. The binary is published separately, as a release
// asset, so absence of the library is a state a user fixes by downloading one
// rather than a capability this client does not have. Everything below is a thin C++ skin over
// `display/abi/display_abi.h` whose only job is to keep the ABI's rules -- the
// handle to close, the strings that belong to the document, the error buffer to
// copy out of -- from leaking into widget code.
//
// NOTHING HERE NAMES A `display` TYPE. That is the point: the types are the
// client's own, so the client's source closure contains one C header and no
// private product. When a field looks redundant against
// `scada::display::view::ShapeHit`, it is deliberately a separate declaration.

// A rectangle in a document's page coordinates. Page Y grows UPWARD, as it does
// in the authored document; a widget flips it.
struct DisplayRect {
  double x = 0;
  double y = 0;
  double w = 0;
  double h = 0;
};

// Which reader opens a file. `kAuto` decides from the extension.
enum class DisplayDocumentKind { kAuto, kVds, kModus };

// One shape found under a hit-test point. The strings are OWNED COPIES, unlike
// the ABI's, so a caller may keep a hit for as long as it likes.
struct DisplayShapeHit {
  int32_t id = 0;
  DisplayRect bounds;
  std::string name;
  std::string text;
  std::string data_source;
};

// Equipment-state colours, packed 0x00RRGGBB. The host supplies resolved design
// tokens and the renderer decides how a state uses them.
struct DisplayStatePalette {
  uint32_t sl_live = 0;
  uint32_t sl_energized = 0;
  uint32_t sl_closed = 0;
  uint32_t sl_open = 0;
  uint32_t bad = 0;
  uint32_t uncertain = 0;
};

// Why an operation failed, in a form a dialog can show.
struct DisplayError {
  int32_t code = 0;
  std::string message;
};

// Why this client will not drive the ABI table a library handed it, or empty
// when it will.
//
// Free, and declared here, because the loading path is a process-wide singleton
// over a real shared library: there is no way to hand it a table that is one
// version too new, or one that under-reports its own size, short of building a
// library that lies. A test calls this with a table it wrote itself.
//
// The two members it reads sit at fixed offsets in every ABI version, which is
// what makes them readable at all before anything else about the table is
// known. See `display/abi/display_abi.h`, "VERSIONING".
std::string DisplayRuntimeTableRejection(const ScadaDisplayApi& api);

// The loaded runtime library, or the reason there is none.
//
// ABSENCE IS A SUPPORTED STATE, not a build configuration (ADR 0013 design
// point 3). A client whose runtime is missing or rejected runs normally and
// reports "no display runtime" where a schematic would be, because that is the
// state a stranger who cloned the public repository is in before they download
// one. There is deliberately no way to compile the display surface out.
class DisplayRuntime {
 public:
  // Loads the runtime the first time it is asked for, and answers the same way
  // every time after. Returns nullptr when no runtime could be loaded;
  // `unavailable_reason()` then says why, in operator-facing terms.
  static const DisplayRuntime* Get();

  // Empty while a runtime is loaded. Otherwise the reason, already translated.
  static std::string_view unavailable_reason();

  // Where a runtime is looked for, in order. Exposed so a test can assert on
  // the search rather than on one machine's filesystem.
  static std::vector<std::filesystem::path> SearchPaths();

  const ScadaDisplayApi& api() const { return *api_; }

  // What the loaded library says it is, for diagnostics and logs.
  std::string_view version() const { return version_; }

  DisplayRuntime(const DisplayRuntime&) = delete;
  DisplayRuntime& operator=(const DisplayRuntime&) = delete;

 private:
  friend class DisplayRuntimeLoader;

  DisplayRuntime(const ScadaDisplayApi* api, std::string version);

  const ScadaDisplayApi* api_;
  std::string version_;
};

// One open display, over the ABI.
//
// Mirrors `scada::display::view::DisplayDocument` member for member, except for
// rendering: the ABI carries PIXELS, so a caller supplies a buffer instead of a
// QPainter. That is the cost ADR 0013 states outright -- a zoomed schematic
// scales a bitmap where the linked facade re-rendered as vector.
class DisplayRuntimeDocument {
 public:
  // Loads `path`. Returns nullptr and fills `error` when there is no runtime,
  // when the file is missing, or when the parser rejects it.
  static std::unique_ptr<DisplayRuntimeDocument> Open(
      const std::filesystem::path& path,
      DisplayDocumentKind kind,
      DisplayError* error);

  ~DisplayRuntimeDocument();

  DisplayRuntimeDocument(const DisplayRuntimeDocument&) = delete;
  DisplayRuntimeDocument& operator=(const DisplayRuntimeDocument&) = delete;

  // The document's own title, or the file's stem when it carries none.
  const std::string& Title() const { return title_; }

  // The authored page rectangle, always origin-anchored with a positive size.
  DisplayRect PageBounds() const { return page_bounds_; }

  // Paints the whole page into `pixels` as premultiplied BGRA, scaled to fill
  // `width` x `height`, with `stride` bytes per row. False on failure.
  bool RenderBgra(uint8_t* pixels,
                  int32_t width,
                  int32_t height,
                  int32_t stride,
                  DisplayError* error);

  // The topmost shape containing the page point, or nullopt over bare page.
  std::optional<DisplayShapeHit> HitTest(double page_x, double page_y) const;

  // Installs the equipment-state palette, or clears it with nullptr so the
  // authored appearance is painted verbatim.
  void SetStatePalette(const DisplayStatePalette* palette);

  // Records the latest value and quality for one data source and returns the
  // region needing repaint.
  DisplayRect SetDataSourceState(std::string_view data_source,
                                 std::string_view value,
                                 int32_t quality);

 private:
  DisplayRuntimeDocument(const DisplayRuntime& runtime,
                         ScadaDisplayDocument document);

  const DisplayRuntime& runtime_;
  ScadaDisplayDocument document_;

  // Read once at open: the ABI's strings belong to the document and the page
  // bounds cost a call, while both are asked for on every paint.
  std::string title_;
  DisplayRect page_bounds_;
};
