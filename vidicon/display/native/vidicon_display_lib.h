#pragma once

#include <TeleClient.h>
#include <Windows.h>
#include <filesystem>
#include <functional>
#include <span>

namespace vidicon {

class display;

struct display_library {
  display_library() {
    if (!lib) {
      throw std::runtime_error("Cannot load display library");
    }

    if (!get_version || get_version() != 5) {
      throw std::runtime_error("Unsupported DisplayLib.dll version");
    }

    if (!open_display || !destroy_display || !draw_display ||
        !get_viewport_rect || !release_shape || !get_shape_at ||
        !get_shape_metadata || !exec_shape_action) {
      throw std::runtime_error("Cannot find required DisplayLib.dll functions");
    }
  }

  const HMODULE lib = ::LoadLibraryW(kLibraryPath);

  enum DisplayEventType { EVENT_INVALIDATE = 9, EVENT_COMMAND = 14 };

  struct DisplayRect {
    float left;
    float top;
    float width;
    float height;
  };

  struct Viewport {
    RECT rect;
  };

  struct InvalidateEvent {
    DisplayRect rect;
  };

  struct CommandEvent {
    BSTR command_name;
    int argument_count;
    VARIANT* arguments;
  };

  using DisplayEventHandler = void(__cdecl*)(int event_type,
                                             const void* event_data,
                                             void* argument);

  struct DisplayOpenContext {
    LPCWSTR path;
    void* teleclient;
    DisplayEventHandler event_handler;
    void* event_handler_argument;
  };

  struct DisplayShapeAction {
    BSTR title;
    int checked;
    int enabled;
  };

  struct DisplayShapeMetadata {
    BSTR data_source;
    int action_count;
    DisplayShapeAction* actions;
  };

  using GetVersionFunc = int(__cdecl*)();
  const GetVersionFunc get_version =
      reinterpret_cast<GetVersionFunc>(GetProcAddress(lib, "GetVersion"));

  using OpenDisplayFunc = int(__cdecl*)(const DisplayOpenContext* context);
  const OpenDisplayFunc open_display =
      reinterpret_cast<OpenDisplayFunc>(GetProcAddress(lib, "OpenDisplay"));

  using DestroyDisplayFunc = void(__cdecl*)(int handle);
  const DestroyDisplayFunc destroy_display =
      reinterpret_cast<DestroyDisplayFunc>(
          GetProcAddress(lib, "DestroyDisplay"));

  using DrawDisplayFunc = void(__cdecl*)(int handle,
                                         HDC dc,
                                         const Viewport* viewport);
  const DrawDisplayFunc draw_display =
      reinterpret_cast<DrawDisplayFunc>(GetProcAddress(lib, "DrawDisplay"));

  using GetViewportRectFunc = void(__cdecl*)(int handle,
                                             const Viewport* viewport,
                                             const DisplayRect* display_rect,
                                             RECT* viewport_rect);
  const GetViewportRectFunc get_viewport_rect =
      reinterpret_cast<GetViewportRectFunc>(
          GetProcAddress(lib, "GetViewportRect"));

  using ReleaseShape = void(__cdecl*)(int handle);
  const ReleaseShape release_shape =
      reinterpret_cast<ReleaseShape>(GetProcAddress(lib, "ReleaseShape"));

  using GetShapeAt = int(__cdecl*)(int handle,
                                   const Viewport* viewport,
                                   const POINT* view_point);
  const GetShapeAt get_shape_at =
      reinterpret_cast<GetShapeAt>(GetProcAddress(lib, "GetShapeAt"));

  using GetShapeMetadata = int(__cdecl*)(int handle,
                                         DisplayShapeMetadata* metadata);
  const GetShapeMetadata get_shape_metadata =
      reinterpret_cast<GetShapeMetadata>(
          GetProcAddress(lib, "GetShapeMetadata"));

  using ExecShapeAction = int(__cdecl*)(int handle, int action_index);
  const ExecShapeAction exec_shape_action =
      reinterpret_cast<ExecShapeAction>(GetProcAddress(lib, "ExecShapeAction"));

#if defined(NDEBUG)
  inline static const wchar_t kLibraryPath[] = L"DisplayLib.dll";
#else
  inline static const wchar_t kLibraryPath[] =
      LR"(c:\tc\vidicon\Vidicon\bin\debug\DisplayLib.dll)";
#endif
};

using display_rect = display_library::DisplayRect;
using display_viewport = display_library::Viewport;

struct shape_action {
  std::wstring title;
  bool checked = false;
  bool enabled = true;
};

struct shape_metadata {
  std::wstring data_source;
  std::vector<shape_action> actions;
};

class shape {
 public:
  ~shape() {
    if (handle_ >= 0) {
      library_->release_shape(handle_);
    }
  }

  shape(const shape&) = delete;
  shape& operator=(const shape&) = delete;

  shape(shape&& other) noexcept
      : library_{other.library_}, handle_{other.handle_} {
    other.handle_ = -1;
  }

  shape& operator=(shape&& other) noexcept {
    if (&other != this) {
      library_ = other.library_;
      handle_ = other.handle_;
      other.handle_ = -1;
    }
    return *this;
  }

  bool is_null() const { return handle_ < 0; }

  shape_metadata metadata() const {
    if (handle_ < 0) {
      return {};
    }

    display_library::DisplayShapeMetadata metadata = {};
    if (!library_->get_shape_metadata(handle_, &metadata)) {
      return {};
    }

    std::span metadata_actions{metadata.actions,
                               metadata.actions + metadata.action_count};

    shape_metadata result{
        .data_source = metadata.data_source ? std::wstring{metadata.data_source}
                                            : std::wstring{},
        .actions = convert_shape_actions(metadata_actions)};

    ::SysFreeString(metadata.data_source);
    std::ranges::for_each(metadata_actions,
                          [](display_library::DisplayShapeAction& action) {
                            ::SysFreeString(action.title);
                          });
    ::CoTaskMemFree(metadata.actions);

    return result;
  }

  bool exec_action(int action_index) const {
    if (handle_ < 0) {
      return false;
    }

    return !!library_->exec_shape_action(handle_, action_index);
  }

 private:
  shape(const display_library& library, int handle)
      : library_{&library}, handle_{handle} {}

  void release() { handle_ = -1; }

  static shape_action convert_shape_action(
      const display_library::DisplayShapeAction& action) {
    return {.title = action.title,
            .checked = !!action.checked,
            .enabled = !!action.enabled};
  }

  static std::vector<shape_action> convert_shape_actions(
      std::span<const display_library::DisplayShapeAction> actions) {
    std::vector<shape_action> result;
    result.reserve(actions.size());
    std::ranges::transform(actions, std::back_inserter(result),
                           &convert_shape_action);
    return result;
  }

  const display_library* library_ = nullptr;
  int handle_ = -1;

  friend class display;
};

class display {
 public:
  explicit display(const display_library& library) : library_{library} {}
  ~display() { library_.destroy_display(handle_); }

  display(const display&) = delete;
  display& operator=(const display&) = delete;

  bool is_opened() const { return handle_ >= 0; }

  using invalidate_handler = std::function<void(const display_rect& rect)>;

  void set_invalidate_handler(invalidate_handler handler) {
    invalidate_ = std::move(handler);
  }

  using exec_command_handler =
      std::function<void(std::wstring_view command_name,
                         std::span<const VARIANT> arguments)>;

  void set_exec_command_handler(exec_command_handler handler) {
    exec_command_ = std::move(handler);
  }

  void open(const std::filesystem::path& path, IClient& teleclient) {
    if (is_opened()) {
      throw std::runtime_error{"The display is already opened"};
    }

    auto path_wstr = path.wstring();

    display_library::DisplayOpenContext context{
        .path = path_wstr.c_str(),
        .teleclient = &teleclient,
        .event_handler = &display::event_handler,
        .event_handler_argument = this};

    auto handle = library_.open_display(&context);
    if (handle < 0) {
      throw std::runtime_error{"Cannot open display"};
    }

    handle_ = handle;
  }

  void draw(HDC dc, const display_viewport& viewport) const {
    check_opened();
    library_.draw_display(handle_, dc, &viewport);
  }

  RECT viewport_rect(const display_viewport& viewport,
                     const display_rect& rect) const {
    check_opened();
    RECT viewport_rect = {};
    library_.get_viewport_rect(handle_, &viewport, &rect, &viewport_rect);
    return viewport_rect;
  }

  shape shape_at(const display_viewport& viewport, const POINT& point) const {
    check_opened();
    auto shape_handle = library_.get_shape_at(handle_, &viewport, &point);
    return shape{library_, shape_handle};
  }

 private:
  void check_opened() const {
    if (!is_opened()) {
      throw std::runtime_error{"The display is not opened"};
    }
  }

  static void __cdecl event_handler(int event_type,
                                    const void* event_data,
                                    void* argument) {
    auto* disp = static_cast<display*>(argument);
    switch (event_type) {
      case display_library::EVENT_INVALIDATE:
        if (disp->invalidate_) {
          const auto* invalidate_event =
              static_cast<const display_library::InvalidateEvent*>(event_data);
          disp->invalidate_(invalidate_event->rect);
        }
        break;
      case display_library::EVENT_COMMAND:
        if (disp->exec_command_) {
          const auto* command_event =
              static_cast<const display_library::CommandEvent*>(event_data);
          disp->exec_command_(
              command_event->command_name,
              std::span{command_event->arguments,
                        static_cast<size_t>(command_event->argument_count)});
          break;
        }
    }
  }

  const display_library& library_;
  int handle_ = -1;

  invalidate_handler invalidate_;
  exec_command_handler exec_command_;
};

}  // namespace vidicon
