#include "display_view/display_runtime.h"

#include "base/no_destructor.h"
#include "base/path_service.h"

#include <cstdint>
#include <cstring>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

using GetApiFn = const ScadaDisplayApi*(SCADA_DISPLAY_CALL*)(uint32_t);

// The library's file name, which is the linker's convention rather than ours.
#ifdef _WIN32
constexpr const char* kLibraryName = "display_runtime.dll";
#elif defined(__APPLE__)
constexpr const char* kLibraryName = "libdisplay_runtime.dylib";
#else
constexpr const char* kLibraryName = "libdisplay_runtime.so";
#endif

// Overrides the search entirely, with a full path to a library. For a developer
// pointing the client at a build tree, and for the tests.
constexpr const char* kPathOverrideVariable = "SCADA_DISPLAY_RUNTIME";

// Copies the ABI's inline message buffer out. Never trusts it to be
// NUL-terminated: it is written by a binary this build did not produce.
std::string MessageFrom(const ScadaDisplayError& error) {
  const size_t limit = sizeof(error.message);
  const size_t length = ::strnlen(error.message, limit);
  return std::string{error.message, length};
}

void SetError(DisplayError* error, int32_t code, std::string message) {
  if (!error)
    return;
  error->code = code;
  error->message = std::move(message);
}

DisplayRect ToRect(const ScadaDisplayRect& rect) {
  return DisplayRect{rect.x, rect.y, rect.width, rect.height};
}

int32_t ToAbiKind(DisplayDocumentKind kind) {
  switch (kind) {
    case DisplayDocumentKind::kVds:
      return SCADA_DISPLAY_DOCUMENT_KIND_VDS;
    case DisplayDocumentKind::kModus:
      return SCADA_DISPLAY_DOCUMENT_KIND_MODUS;
    case DisplayDocumentKind::kAuto:
      break;
  }
  return SCADA_DISPLAY_DOCUMENT_KIND_AUTO;
}

// Copies an ABI string, tolerating a null. The runtime owns these and a
// malformed one is a bug in a binary we did not build, so nothing here assumes
// they are present.
std::string OwnedString(const char* text) {
  return text ? std::string{text} : std::string{};
}

// How many bytes a table claiming `abi_version` must be for this client to read
// the members that version defined.
//
// Not `sizeof(ScadaDisplayApi)`: a client built against a later ABI has a
// LONGER struct than an older library ever wrote, and measuring an ABI-1 table
// against an ABI-3 struct rejects a library that is perfectly usable. The
// per-version sizes the ABI header publishes are what this is for.
size_t RequiredStructSize(uint32_t abi_version) {
  // One entry today. Each future ABI appends its own; none is ever edited,
  // because a member is only ever appended and offsetof of an existing one
  // cannot move.
  switch (abi_version) {
    case 1:
      return SCADA_DISPLAY_ABI_1_SIZE;
    default:
      break;
  }
  // A version this client has never heard of is rejected before it gets here,
  // so the only way in is an abi_version of 0 -- a table that does not claim to
  // implement anything. Demand more than exists so it cannot be used.
  return SIZE_MAX;
}

// What a library is, for a message an operator reads. Never parsed.
//
// Takes a table rather than a loaded runtime because the caller that needs it
// most has just been REFUSED one, and its only handle on the library is
// whatever `ScadaDisplayGetApi(UINT32_MAX)` hands back. Every field is gated:
// this is a binary that has already failed to agree with us once.
std::string Describe(const ScadaDisplayApi* api) {
  if (!api)
    return "a runtime that reports no version at all";

  std::string text = "display ABI " + std::to_string(api->abi_version);
  if (api->struct_size >= SCADA_DISPLAY_ABI_1_SIZE && api->runtime_version) {
    const std::string version = OwnedString(api->runtime_version());
    if (!version.empty())
      text += " (" + version + ")";
  }
  return text;
}

}  // namespace

// ── Loading ─────────────────────────────────────────────────────────────────

std::string DisplayRuntimeTableRejection(const ScadaDisplayApi& api) {
  if (api.abi_version > SCADA_DISPLAY_ABI_VERSION) {
    // Not a drift this client can absorb: past its own ABI the members are not
    // the ones it thinks they are. A well-behaved library never does this,
    // having just been asked for a table at or below -- which is why it is a
    // rejection and not a fallback.
    return "served display ABI " + std::to_string(api.abi_version) +
           " after being asked for " +
           std::to_string(SCADA_DISPLAY_ABI_VERSION) + " or older";
  }

  // An OLDER library is accepted on purpose, and this is the direction the
  // pair actually drift in: a host is built from source and a library is
  // downloaded, so the library is usually the older of the two. It is measured
  // against what ITS ABI defined, never against sizeof(ScadaDisplayApi), which
  // by a later ABI describes a longer table it never wrote.
  const size_t required = RequiredStructSize(api.abi_version);
  if (api.struct_size < required) {
    // Shorter than the ABI it claims: the members past its end are whatever
    // happens to follow in the library's memory, and calling one is a jump to
    // an address nobody set.
    return "its display ABI " + std::to_string(api.abi_version) + " table is " +
           std::to_string(api.struct_size) + " bytes, short of the " +
           std::to_string(required) + " that version defines";
  }
  return {};
}

// Does the one-time load, so DisplayRuntime itself holds no loading state.
//
// A separate class rather than statics in a function because the outcome has
// two halves -- the runtime or the reason there is none -- and both have to
// survive for the life of the process.
class DisplayRuntimeLoader {
 public:
  DisplayRuntimeLoader() {
    for (const std::filesystem::path& candidate :
         DisplayRuntime::SearchPaths()) {
      std::error_code ec;
      if (!std::filesystem::exists(candidate, ec))
        continue;
      if (TryLoad(candidate))
        return;
      // Kept and reported: a runtime that is PRESENT and unusable is a
      // different problem from one that is absent, and the operator needs to
      // know which. The search continues in case a later candidate works.
    }
    if (reason_.empty()) {
      reason_ = "no display runtime was found beside the application";
    }
  }

  const DisplayRuntime* runtime() const { return runtime_.get(); }
  const std::string& reason() const { return reason_; }

 private:
  bool TryLoad(const std::filesystem::path& path) {
    const std::string native = path.string();

    // Loaded with LOCAL visibility, which is what keeps the runtime's own Qt
    // out of this process's global symbol namespace. With RTLD_GLOBAL the two
    // Qts would interpose on each other and the C ABI would buy nothing.
#ifdef _WIN32
    HMODULE module = ::LoadLibraryA(native.c_str());
    if (!module) {
      reason_ = native + ": the library could not be loaded";
      return false;
    }
    auto get_api = reinterpret_cast<GetApiFn>(
        ::GetProcAddress(module, SCADA_DISPLAY_GET_API_SYMBOL));
#else
    void* module = ::dlopen(native.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) {
      const char* detail = ::dlerror();
      reason_ =
          native + ": " + (detail ? detail : "the library could not be loaded");
      return false;
    }
    auto get_api = reinterpret_cast<GetApiFn>(
        ::dlsym(module, SCADA_DISPLAY_GET_API_SYMBOL));
#endif

    if (!get_api) {
      reason_ = native + ": not a display runtime (no " +
                SCADA_DISPLAY_GET_API_SYMBOL + ")";
      return false;
    }

    const ScadaDisplayApi* api = get_api(SCADA_DISPLAY_ABI_VERSION);
    if (!api) {
      // Refused. The library is NEWER than this client and no longer serves an
      // ABI this old -- the only shape a "newest at or below" implementation
      // can refuse. Ask it what it does have, purely so the operator is told
      // which library was rejected rather than that one was.
      reason_ = native + ": this client speaks display ABI " +
                std::to_string(SCADA_DISPLAY_ABI_VERSION) + ", and " +
                Describe(get_api(UINT32_MAX)) + " is newer";
      return false;
    }

    // The table the library handed back must also describe itself sanely.
    if (std::string rejection = DisplayRuntimeTableRejection(*api);
        !rejection.empty()) {
      reason_ = native + ": " + std::move(rejection);
      return false;
    }

    runtime_ = std::unique_ptr<DisplayRuntime>{new DisplayRuntime{
        api,
        OwnedString(api->runtime_version ? api->runtime_version() : nullptr)}};
    reason_.clear();
    return true;
  }

  std::unique_ptr<DisplayRuntime> runtime_;
  std::string reason_;
};

DisplayRuntime::DisplayRuntime(const ScadaDisplayApi* api, std::string version)
    : api_{api}, version_{std::move(version)} {}

std::vector<std::filesystem::path> DisplayRuntime::SearchPaths() {
  std::vector<std::filesystem::path> paths;

  if (const char* override_path = std::getenv(kPathOverrideVariable)) {
    if (*override_path)
      paths.emplace_back(override_path);
  }

  std::filesystem::path exe_dir;
  if (scada::base::PathService::Get(scada::base::DIR_EXE, &exe_dir)) {
    // Beside the executable first, which is where the packaging puts it and
    // where a tier's `<exe dir>/../data` convention would lead someone to look.
    paths.push_back(exe_dir / kLibraryName);
#ifdef __APPLE__
    // Inside the bundle, where a .dylib belongs on macOS. The executable sits
    // in Contents/MacOS, so Frameworks is its sibling.
    paths.push_back(exe_dir.parent_path() / "Frameworks" / kLibraryName);
#endif
    paths.push_back(exe_dir.parent_path() / "lib" / kLibraryName);
  }

  return paths;
}

namespace {

// ONE loader for the process. Two `static NoDestructor<DisplayRuntimeLoader>`
// in two functions would be two loaders and two dlopens of the same library --
// each with its own private Qt and its own QGuiApplication -- so the accessor
// is written once and both entry points go through it.
const DisplayRuntimeLoader& Loader() {
  static const scada::base::NoDestructor<DisplayRuntimeLoader> loader;
  return *loader;
}

}  // namespace

const DisplayRuntime* DisplayRuntime::Get() {
  return Loader().runtime();
}

std::string_view DisplayRuntime::unavailable_reason() {
  return Loader().reason();
}

// ── Documents ───────────────────────────────────────────────────────────────

DisplayRuntimeDocument::DisplayRuntimeDocument(const DisplayRuntime& runtime,
                                               ScadaDisplayDocument document)
    : runtime_{runtime}, document_{document} {}

DisplayRuntimeDocument::~DisplayRuntimeDocument() {
  runtime_.api().close_document(document_);
}

std::unique_ptr<DisplayRuntimeDocument> DisplayRuntimeDocument::Open(
    const std::filesystem::path& path,
    DisplayDocumentKind kind,
    DisplayError* error) {
  const DisplayRuntime* runtime = DisplayRuntime::Get();
  if (!runtime) {
    // Not an error about the document: there is nothing here that could open
    // one. The widget says so differently, which is why the code is distinct.
    SetError(error, SCADA_DISPLAY_ERROR_UNEXPECTED,
             std::string{DisplayRuntime::unavailable_reason()});
    return nullptr;
  }

  ScadaDisplayError abi_error{};
  ScadaDisplayDocument document = runtime->api().open_document(
      path.string().c_str(), ToAbiKind(kind), &abi_error);
  if (!document) {
    SetError(error, abi_error.code, MessageFrom(abi_error));
    return nullptr;
  }

  auto result = std::unique_ptr<DisplayRuntimeDocument>{
      new DisplayRuntimeDocument{*runtime, document}};

  ScadaDisplayDocumentInfo info{};
  if (runtime->api().get_document_info(document, &info, &abi_error)) {
    result->title_ = OwnedString(info.title);
    result->page_bounds_ = ToRect(info.page_bounds);
  } else {
    // A document that opened but will not describe itself is still usable --
    // it can be painted -- so this is not a failure to open. The page bounds
    // fall back to the ABI's own documented default so that nothing downstream
    // divides by zero.
    result->page_bounds_ = DisplayRect{0, 0, 640, 480};
  }
  return result;
}

bool DisplayRuntimeDocument::RenderBgra(uint8_t* pixels,
                                        int32_t width,
                                        int32_t height,
                                        int32_t stride,
                                        DisplayError* error) {
  ScadaDisplayError abi_error{};
  if (runtime_.api().render_bgra(document_, pixels, width, height, stride,
                                 &abi_error)) {
    return true;
  }
  SetError(error, abi_error.code, MessageFrom(abi_error));
  return false;
}

std::optional<DisplayShapeHit> DisplayRuntimeDocument::HitTest(
    double page_x,
    double page_y) const {
  ScadaDisplayShapeInfo info{};
  ScadaDisplayError abi_error{};
  const int32_t result =
      runtime_.api().hit_test(document_, page_x, page_y, &info, &abi_error);
  if (result != 1) {
    // 0 is bare page and a negative value is a failure. Both mean "no shape
    // here" to a caller, and a failed hit test is not something an operator can
    // act on -- the source facade collapsed the two the same way.
    return std::nullopt;
  }

  DisplayShapeHit hit;
  hit.id = info.id;
  hit.bounds = ToRect(info.bounds);
  // Copied, not aliased: the ABI says the next hit_test replaces them.
  hit.name = OwnedString(info.name);
  hit.text = OwnedString(info.text);
  hit.data_source = OwnedString(info.data_source);
  return hit;
}

void DisplayRuntimeDocument::SetStatePalette(
    const DisplayStatePalette* palette) {
  ScadaDisplayError abi_error{};
  if (!palette) {
    runtime_.api().set_state_palette(document_, nullptr, &abi_error);
    return;
  }

  const ScadaDisplayStatePalette abi_palette{
      .sl_live = palette->sl_live,
      .sl_energized = palette->sl_energized,
      .sl_closed = palette->sl_closed,
      .sl_open = palette->sl_open,
      .bad = palette->bad,
      .uncertain = palette->uncertain,
  };
  runtime_.api().set_state_palette(document_, &abi_palette, &abi_error);
}

DisplayRect DisplayRuntimeDocument::SetDataSourceState(
    std::string_view data_source,
    std::string_view value,
    int32_t quality) {
  // The ABI takes C strings, and a string_view is not NUL-terminated.
  const std::string data_source_text{data_source};
  const std::string value_text{value};

  ScadaDisplayRect invalidated{};
  ScadaDisplayError abi_error{};
  if (runtime_.api().set_data_source_state(document_, data_source_text.c_str(),
                                           value_text.c_str(), quality,
                                           &invalidated, &abi_error)) {
    return ToRect(invalidated);
  }
  // The caller repaints what it is told to; the whole page is the safe answer
  // and matches what the renderer reports on success.
  return page_bounds_;
}
