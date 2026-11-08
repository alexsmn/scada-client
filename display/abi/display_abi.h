// The C ABI a host uses to show a schematic display without linking `display`
// as source (ADR 0013).
//
// WHY THIS IS C AND NOT C++. The source facade next door
// (`display/view/display_document.h`) hands out `std::unique_ptr`,
// `std::string`, `std::filesystem::path`, `std::optional` and `QPainter&`.
// Every one of those crosses a binary boundary by layout, by allocator or by
// Qt version, so a shared library exporting it links only against a host built
// with the same compiler, the same standard library, the same Qt and — on
// Windows — the same CRT. The public Qt client is built by strangers whose
// toolchain is none of those, which is the whole reason this file exists.
//
// WHAT IT COSTS. Only pixels cross here, so a host cannot paint a display into
// its own QPainter and cannot re-render it as vector on zoom; it scales a
// bitmap instead. ADR 0013 states that cost outright rather than discovering
// it later. The escape, if it is ever wanted, is a recording backend emitting
// a vector display list — strictly better and strictly more work.
//
// TWO QT RUNTIMES IN ONE PROCESS. An implementation of this ABI carries its
// own Qt; if it shared the host's, the host's Qt version would be part of the
// contract again and the ABI would buy nothing. See the note above
// `ScadaDisplayGetApi` for the host-abort that follows from it, which is not
// hypothetical and which any test suite written against this file must cover
// on its FIRST case.
//
// VERSIONING. `abi_version` plus `struct_size`: members are appended and never
// reordered or resized, so an older caller sees a byte-compatible prefix of a
// newer table and a newer caller detects a shorter one. Gate every access to a
// member added after ABI 1 on `struct_size`.
//
// THE PAIR ARE VERSIONED SEPARATELY AND DRIFT IN ONE DIRECTION. A host is built
// from source; a library is downloaded, and is therefore usually the older of
// the two. So the contract is deliberately tolerant of exactly that: a host
// asks for the newest ABI it knows and ACCEPTS ANY TABLE AT OR BELOW IT,
// reading only the members that ABI defined (see SCADA_DISPLAY_ABI_1_SIZE).
// The other direction already worked, by the prefix rule. What a host must
// never do is accept a table ABOVE its own ABI: the members past its knowledge
// are not the ones it thinks they are.
//
// This file is deliberately C, not C++: no namespaces, no templates, no fixed
// enum underlying types, nothing a C compiler would reject. A host may bind to
// it from any language that speaks C.

#ifndef SCADA_DISPLAY_ABI_H_
#define SCADA_DISPLAY_ABI_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The calling convention every entry point uses. Explicit because a Windows
// host compiled with /Gz or /Gr would otherwise disagree with the library.
#ifdef _WIN32
#define SCADA_DISPLAY_CALL __cdecl
#else
#define SCADA_DISPLAY_CALL
#endif

// Marks the one exported symbol. The implementation defines
// SCADA_DISPLAY_ABI_IMPLEMENTATION; a host leaves it undefined and gets a
// plain declaration, because it resolves the symbol at run time rather than
// linking it.
#ifdef SCADA_DISPLAY_ABI_IMPLEMENTATION
#ifdef _WIN32
#define SCADA_DISPLAY_EXPORT __declspec(dllexport)
#else
#define SCADA_DISPLAY_EXPORT __attribute__((visibility("default")))
#endif
#else
#define SCADA_DISPLAY_EXPORT
#endif

// ── Version ─────────────────────────────────────────────────────────────────

// The ABI this header describes. It starts at 1: the `tc_vds_runtime` ABI this
// borrows its shape from reached version 2 and was deleted (ADR 0012 phase 4),
// and nothing implements it any more, so continuing its numbering would claim
// a compatibility that does not exist.
//
// ABI 2 (backlog 915) appends the four members a host needs to put live
// values on a Modus schematic: enumerating what the document is bound to,
// writing a value as an element's text, and finding where an object is
// drawn. Nothing before `runtime_version` moved.
//
// ABI 3 appends three members for what a 2.5.7 schematic did and 2.6 did not:
// a reading delivered as a number (what puts live values on a Vidicon
// schematic, whose formulas compute from the number), an unacknowledged alarm
// marked on its elements, and the hyperlink a double-click on a Modus element
// follows. Nothing before `find_data_source_shape` moved.
//
// ABI 4 (backlog 1006) adds no member: it is the version a runtime claims
// once `set_data_source_value` keeps the QUALITY-MARKING CONTRACT described
// there — a bad reading hatches its shapes, a value field with no value or a
// bad one shows "?", and a lost value puts the drawn state back. Those marks
// are a safety signal, and a runtime that lacks them draws a confident
// schematic with nothing to say so, so the host needs a number to warn on.
// An ABI 3 runtime may have some of the contract (from 2026-11-02 it hatched
// shapes) but not all of it, and a host cannot tell which; it treats every
// ABI 3 table as lacking it. The table layout is ABI 3's, and
// `SCADA_DISPLAY_ABI_4_SIZE` equals `SCADA_DISPLAY_ABI_3_SIZE`.
#define SCADA_DISPLAY_ABI_VERSION 4u

// ── Plain data ──────────────────────────────────────────────────────────────

// Which reader opens the file. `kAuto` decides from the extension; the
// explicit kinds name a reader outright. Mirrors
// scada::display::view::DocumentKind, and the two must stay in step.
//
// `kModus` covers both Modus encodings and deliberately does not distinguish
// them: SDE is a little-endian binary stream and XSDE is XML, two unrelated
// encodings rather than two versions of one, so the extension decides.
typedef enum ScadaDisplayDocumentKind {
  SCADA_DISPLAY_DOCUMENT_KIND_AUTO = 0,
  SCADA_DISPLAY_DOCUMENT_KIND_VDS = 1,
  SCADA_DISPLAY_DOCUMENT_KIND_MODUS = 2
} ScadaDisplayDocumentKind;

// Why a call failed. These are the codes the source facade reports
// (`scada::display::view::kError*`), kept identical so a host that logs one
// sees the same number whichever seam it went through.
typedef enum ScadaDisplayErrorCode {
  SCADA_DISPLAY_ERROR_NONE = 0,
  SCADA_DISPLAY_ERROR_UNEXPECTED = 1,
  SCADA_DISPLAY_ERROR_LOAD_FAILED = 2,
  SCADA_DISPLAY_ERROR_INVALID_ARGUMENT = 3,
  SCADA_DISPLAY_ERROR_RENDER_FAILED = 4
} ScadaDisplayErrorCode;

// A rectangle in page coordinates. Page Y grows upward, as it does in the
// authored document and in the source facade; a host drawing into a
// downward-Y widget flips it.
typedef struct ScadaDisplayRect {
  double x;
  double y;
  double width;
  double height;
} ScadaDisplayRect;

// Failure detail. The message is a fixed-size UTF-8 buffer rather than a
// pointer so that nothing allocated by the library has to be freed by the
// host — the one allocation question a C ABI cannot answer portably. It is
// always NUL-terminated, truncated if it would not fit.
typedef struct ScadaDisplayError {
  int32_t code;
  char message[512];
} ScadaDisplayError;

// What a loaded document is. Every string points into storage the document
// owns and stays valid until `close_document`.
typedef struct ScadaDisplayDocumentInfo {
  const char* title;
  const char* path;
  ScadaDisplayRect page_bounds;
} ScadaDisplayDocumentInfo;

// One shape found beneath a hit-test point. The strings point into storage the
// DOCUMENT owns, and the next `hit_test` on the same document replaces them —
// a host that wants to keep a hit copies them out. (The source facade returns
// owned copies instead; a C ABI cannot, for the same allocation reason as
// `ScadaDisplayError::message`.)
typedef struct ScadaDisplayShapeInfo {
  int32_t id;
  ScadaDisplayRect bounds;
  const char* name;
  const char* text;
  const char* data_source;
} ScadaDisplayShapeInfo;

// Equipment-state colours for single-line / mimic displays, packed 0x00RRGGBB
// with the high byte ignored.
//
// The host injects resolved `sl_*` design tokens once and then reports raw
// state through `set_data_source_state`; the mapping from state to appearance
// stays inside the renderer, so the host never sends per-shape colours. See
// docs/client/ux/design-language.md §2 — the energized colour is a restrained
// amber, never alarm-red, and state reads by shape as well as by colour.
typedef struct ScadaDisplayStatePalette {
  uint32_t sl_live;       // energized primary conductor / live busbar
  uint32_t sl_energized;  // de-energized / idle conductor (neutral)
  uint32_t sl_closed;     // switching device closed / in service
  uint32_t sl_open;       // switching device open (neutral, not an alarm)
  uint32_t bad;           // bad-quality telemetry
  uint32_t uncertain;     // uncertain / intermediate state
} ScadaDisplayStatePalette;

// An opaque loaded document. Never dereferenced by the host.
typedef struct ScadaDisplayDocumentOpaque* ScadaDisplayDocument;

// ── The table ───────────────────────────────────────────────────────────────

// Every entry point, resolved once through `ScadaDisplayGetApi`.
//
// Unless a member says otherwise: a document handle must not be used from two
// threads at once, and every call must happen on the thread that first opened
// a document, as with any Qt painting.
typedef struct ScadaDisplayApi {
  // Always the first two members, at these offsets, in every ABI version.
  uint32_t abi_version;
  uint32_t struct_size;

  // Loads `utf8_path` with the reader named by `kind` (a
  // ScadaDisplayDocumentKind). Returns NULL and fills `error` when non-NULL if
  // the file is missing, the extension is not one this library reads, or the
  // parser rejects the content.
  ScadaDisplayDocument(SCADA_DISPLAY_CALL* open_document)(
      const char* utf8_path,
      int32_t kind,
      ScadaDisplayError* error);

  // Releases a document. NULL is accepted and does nothing.
  void(SCADA_DISPLAY_CALL* close_document)(ScadaDisplayDocument document);

  // Fills `info` with the document's title, path and authored page rectangle.
  // Returns non-zero on success.
  //
  // The page rectangle is always origin-anchored with a finite positive size:
  // a document whose page metrics are absent or unusable reports the 640x480
  // default rather than failing, so a host never has to divide by zero.
  int32_t(SCADA_DISPLAY_CALL* get_document_info)(ScadaDisplayDocument document,
                                                 ScadaDisplayDocumentInfo* info,
                                                 ScadaDisplayError* error);

  // Paints the whole page into `pixels`, scaled to fill `width` x `height`,
  // as premultiplied BGRA with `stride` bytes per row. Returns non-zero on
  // success.
  //
  // `stride` must be at least `width * 4`; the buffer must hold
  // `stride * height` bytes and is fully overwritten, background included. A
  // host wanting a device pixel ratio other than 1 asks for a buffer that many
  // times larger and scales the result down — the boundary carries pixels, so
  // there is no vector path here (ADR 0013).
  int32_t(SCADA_DISPLAY_CALL* render_bgra)(ScadaDisplayDocument document,
                                           uint8_t* pixels,
                                           int32_t width,
                                           int32_t height,
                                           int32_t stride,
                                           ScadaDisplayError* error);

  // Finds the topmost shape containing the page point. Returns 1 and fills
  // `info` on a hit, 0 when the point is over bare page, and a negative value
  // on failure with `error` filled.
  //
  // Three outcomes rather than two because "no shape there" is an ordinary
  // answer a host acts on (it clears the selection) and must not be confused
  // with a call that went wrong.
  int32_t(SCADA_DISPLAY_CALL* hit_test)(ScadaDisplayDocument document,
                                        double page_x,
                                        double page_y,
                                        ScadaDisplayShapeInfo* info,
                                        ScadaDisplayError* error);

  // Installs the equipment-state palette used to recolour shapes bound to live
  // telemetry, or clears it with a NULL `palette` so the authored appearance
  // is painted verbatim. Returns non-zero on success.
  int32_t(SCADA_DISPLAY_CALL* set_state_palette)(
      ScadaDisplayDocument document,
      const ScadaDisplayStatePalette* palette,
      ScadaDisplayError* error);

  // Records the latest value and quality for one data source and reports the
  // region needing repaint in `invalidated_bounds`. Returns non-zero on
  // success.
  //
  // `value` is an equipment-state keyword ("closed"/"open"/"on"/"off"/
  // "energized"/"dead"/…, parsed by ParseEquipmentState); `quality` is an OPC
  // UA StatusCode whose severity bits select good/uncertain/bad.
  //
  // In a VDS document a data source is a tag address (see
  // `set_data_source_value`), and a position reaches the shapes bound to it as
  // 1 or 0; in a VDS document without tags it is a shape's name, and the state
  // shows on the next `render_bgra` only while a palette is installed. A
  // library before ABI 3 knows only the shape-name meaning. In a Modus
  // document it is a formula from an element's `ключ_привязки` (the strings
  // `get_data_source` enumerates): a position flips each `положение`-bound
  // element's own symbol, palette or not, and a reading that is not good is
  // also marked in the palette's quality colours while one is installed. A
  // library serving ABI 1 knows only the VDS meaning.
  //
  // The reported region is the whole page: state colouring can change any
  // shape's appearance and there is no per-shape spatial index, so a narrower
  // answer would be a wrong one.
  int32_t(SCADA_DISPLAY_CALL* set_data_source_state)(
      ScadaDisplayDocument document,
      const char* data_source,
      const char* value,
      int32_t quality,
      ScadaDisplayRect* invalidated_bounds,
      ScadaDisplayError* error);

  // A human-readable build identity for this library — shown in diagnostics
  // and logged on a version mismatch, never parsed. Points into static
  // storage and is valid for the life of the process.
  //
  // Present from ABI 1 because the alternative is a host that can report
  // "the display runtime was rejected" without being able to say which one it
  // found.
  const char*(SCADA_DISPLAY_CALL* runtime_version)(void);

  // ── ABI 2 ──
  // Gate every member below on `struct_size >= SCADA_DISPLAY_ABI_2_SIZE`.

  // How many data sources the document names — what a host subscribes to.
  // A Modus document's are its binding formulas. A VDS document's are the
  // addresses its shapes' `Tag` rows name in their `Raw` cells; a VDS document
  // without tags names its shapes at any depth instead, since a shape's name
  // was its binding before tags were read (a library before ABI 3 always
  // answers names), and a host keeps only the ones it can resolve. Zero for a
  // NULL document. A runtime built before 2026-10-26 answers zero for every
  // VDS document, so a host must read zero as "no objects known", not as "no
  // objects".
  int32_t(SCADA_DISPLAY_CALL* get_data_source_count)(
      ScadaDisplayDocument document);

  // The `index`th data source, in document order, as UTF-8 in storage the
  // document owns until `close_document`; NULL when `index` is out of range.
  const char*(SCADA_DISPLAY_CALL* get_data_source)(
      ScadaDisplayDocument document,
      int32_t index);

  // Writes `text` as the text of every Modus element whose `текст` or
  // `значение_базовое` is bound to `data_source`, and reports the region
  // needing repaint in `invalidated_bounds` (empty when nothing is bound to
  // it). Returns non-zero on success. The host formats the value, quality
  // mark included; the library draws it in the element's own font.
  int32_t(SCADA_DISPLAY_CALL* set_data_source_text)(
      ScadaDisplayDocument document,
      const char* data_source,
      const char* text,
      ScadaDisplayRect* invalidated_bounds,
      ScadaDisplayError* error);

  // Finds the first element bound to `data_source`. Returns 1 and fills
  // `info` (strings owned as for `hit_test`, and replaced by the next
  // `hit_test` or `find_data_source_shape`), 0 when nothing is bound to it,
  // and a negative value on failure with `error` filled.
  int32_t(SCADA_DISPLAY_CALL* find_data_source_shape)(
      ScadaDisplayDocument document,
      const char* data_source,
      ScadaDisplayShapeInfo* info,
      ScadaDisplayError* error);

  // ── ABI 3 ──
  // Gate every member below on `struct_size >= SCADA_DISPLAY_ABI_3_SIZE`.

  // Records one data source's latest reading: `value` as a number when
  // `has_value` is non-zero, `text` as the host formats it (units and quality
  // mark included; NULL or empty for none), and `quality` as an OPC UA
  // StatusCode. Reports the region needing repaint in `invalidated_bounds`
  // (empty when nothing is bound to the data source). Returns non-zero on
  // success.
  //
  // In a VDS document this is what makes the schematic live: every shape
  // whose `Tag` row names `data_source` in its `Raw` cell takes the reading,
  // and its own formulas — a breaker's fill, a blade's visibility, a value's
  // text field — follow on the next `render_bgra`.
  //
  // THE QUALITY-MARKING CONTRACT, which a table claiming ABI 4 keeps. A
  // reading with `has_value` zero puts back each bound row's value as the
  // file was saved, so a shape whose value is lost returns to its drawn
  // state, and its value fields show "?" rather than the number the designer
  // saved. A reading whose `quality` is not good hatches the bound shapes in
  // the palette's quality colours while one is installed; their value fields
  // are not hatched but show "?" after the text. A runtime serving ABI 3 may
  // keep part of this or none of it — before 2026-11-02 it ignored `quality`
  // here and kept the last value on `has_value` zero — so a host warns that
  // bad quality may not be marked when it is served ABI 3 or older.
  //
  // In a Modus document it is `set_data_source_state` with the position a
  // non-zero value reads as, plus `set_data_source_text` marked the same way.
  int32_t(SCADA_DISPLAY_CALL* set_data_source_value)(
      ScadaDisplayDocument document,
      const char* data_source,
      double value,
      int32_t has_value,
      const char* text,
      int32_t quality,
      ScadaDisplayRect* invalidated_bounds,
      ScadaDisplayError* error);

  // Frames every element bound to `data_source` in `colour` (packed
  // 0x00RRGGBB) when `alarm` is non-zero, or removes the frame when it is
  // zero, and reports the region needing repaint (empty when nothing is bound
  // to it). Returns non-zero on success. The library keeps no clock: a host
  // makes an unacknowledged alarm blink by toggling it in phase with its own
  // blinker, and takes the colour from its own alarm palette.
  int32_t(SCADA_DISPLAY_CALL* set_data_source_alarm)(
      ScadaDisplayDocument document,
      const char* data_source,
      int32_t alarm,
      uint32_t colour,
      ScadaDisplayRect* invalidated_bounds,
      ScadaDisplayError* error);

  // The hyperlink of the shape `shape_id` names (an id from `hit_test`) as
  // UTF-8 in storage the document owns until `close_document`, or NULL when
  // it has none. A Modus element's is its `гиперссылка` as authored — usually
  // a schematic's file name, relative to this document — which a host
  // resolves and opens on a double-click, as 2.5.7 did.
  const char*(SCADA_DISPLAY_CALL* get_shape_hyperlink)(
      ScadaDisplayDocument document,
      int32_t shape_id);
} ScadaDisplayApi;

// How many bytes of the table ABI 1 defined — through `runtime_version`, its
// last member. A host speaking a later ABI checks a served ABI-1 table's
// `struct_size` against THIS rather than against `sizeof(ScadaDisplayApi)`,
// which by then describes a longer table the library never wrote.
//
// Computed from the member rather than written as a number because appending a
// member must not be able to change it: `offsetof` of an existing member is
// fixed by the no-reorder rule, which is the same rule this whole scheme rests
// on. A future ABI 2 adds its own `SCADA_DISPLAY_ABI_2_SIZE` beside this and
// never edits it.
#define SCADA_DISPLAY_ABI_1_SIZE                \
  (offsetof(ScadaDisplayApi, runtime_version) + \
   sizeof(((const ScadaDisplayApi*)0)->runtime_version))

// How many bytes of the table ABI 2 defined — through
// `find_data_source_shape`, its last member. Same rule as ABI 1's.
#define SCADA_DISPLAY_ABI_2_SIZE                       \
  (offsetof(ScadaDisplayApi, find_data_source_shape) + \
   sizeof(((const ScadaDisplayApi*)0)->find_data_source_shape))

// How many bytes of the table ABI 3 defined — through `get_shape_hyperlink`,
// its last member. Same rule as ABI 1's.
#define SCADA_DISPLAY_ABI_3_SIZE                    \
  (offsetof(ScadaDisplayApi, get_shape_hyperlink) + \
   sizeof(((const ScadaDisplayApi*)0)->get_shape_hyperlink))

// How many bytes of the table ABI 4 defined. ABI 4 added no member (see
// SCADA_DISPLAY_ABI_VERSION), so it is ABI 3's size; it is spelled out so a
// host measures each version against its own constant and a later ABI that
// does append a member never has to reason about which one to borrow.
#define SCADA_DISPLAY_ABI_4_SIZE SCADA_DISPLAY_ABI_3_SIZE

// ── Entry point ─────────────────────────────────────────────────────────────

// The one exported symbol. Returns the NEWEST table whose `abi_version` is at
// most `requested_abi_version`, or NULL when it has none that old — which is
// to say, when the library is newer than the host and has dropped the host's
// ABI entirely.
//
// Two uses follow from "newest at or below", and a library must serve both.
// A host asks for SCADA_DISPLAY_ABI_VERSION to get something it can drive; and
// a host that was just refused asks for UINT32_MAX to get the library's own
// newest table, for no reason but to be able to SAY what it found. The second
// is why the refusal path can name a version at all, and it must not be
// special-cased: it is the first rule read at its limit.
//
// THE HOST MUST NOT CREATE A QT APPLICATION OBJECT FOR THIS LIBRARY, AND
// CANNOT. The library carries its own Qt, so its Qt globals are not the
// host's: a host QApplication is invisible to it and vice versa. Painting text
// needs a QGuiApplication in ITS copy, which `render_bgra` creates offscreen
// on first use and never destroys; it owns no window and runs no event loop.
//
// That is not defensive prose. Before the equivalent existed in the retired
// `tc_vds_runtime` ABI, rendering a document containing text reached
// QFontDatabase with no application object and `qFatal()`'d — aborting the
// HOST process, which presents to a user as the client crashing rather than as
// a failed render. A shapes-only document never reached it, which is why the
// defect survived that ABI's own tests. Any suite written against this file
// renders TEXT on its first case.
SCADA_DISPLAY_EXPORT const ScadaDisplayApi* SCADA_DISPLAY_CALL
ScadaDisplayGetApi(uint32_t requested_abi_version);

// The name to resolve after loading the library, so a host spells it once.
#define SCADA_DISPLAY_GET_API_SYMBOL "ScadaDisplayGetApi"

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SCADA_DISPLAY_ABI_H_
