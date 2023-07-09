#pragma once

#include "services/vidicon/teleclient.h"

#include <filesystem>
#include <windows.h>

namespace vidicon {

struct display_library {
  display_library() {
    if (!lib) {
      throw std::runtime_error("Cannot load display library");
    }

    if (!create_display || !destroy_display || !draw_display ||
        !get_viewport_rect) {
      throw std::runtime_error("Cannot find required display functions");
    }
  }

  const HMODULE lib = ::LoadLibraryW(kLibraryPath);

  enum DisplayEventType { EVENT_INVALIDATE = 9 };

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

  using DisplayEventHandler = void(__cdecl*)(int event_type,
                                             const void* event_data,
                                             void* argument);

  struct DisplayContext {
    LPCWSTR path;
    void* teleclient;
    DisplayEventHandler event_handler;
    void* event_handler_argument;
  };

  using CreateDisplayFunc = int(__cdecl*)(const DisplayContext* context);
  const CreateDisplayFunc create_display =
      reinterpret_cast<CreateDisplayFunc>(GetProcAddress(lib, "CreateDisplay"));

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

  inline static const wchar_t kLibraryPath[] = L"DisplayLib.dll";
  // LR"(c:\tc\vidicon\Vidicon\bin\debug\DisplayLib.dll)";
};

using display_rect = display_library::DisplayRect;
using display_viewport = display_library::Viewport;

class display {
 public:
  explicit display(const display_library& library) : library_{library} {}
  ~display() { library_.destroy_display(handle_); }

  bool is_opened() const { return handle_ >= 0; }

  using invalidate_handler = std::function<void(const display_rect& rect)>;
  void set_invalidate_handler(invalidate_handler handler) {
    invalidate_ = std::move(handler);
  }

  void open(const std::filesystem::path& path, IClient& teleclient) {
    if (is_opened()) {
      throw std::runtime_error{"The display is already opened"};
    }

    auto path_wstr = path.wstring();

    display_library::DisplayContext context{
        .path = path_wstr.c_str(),
        .teleclient = &teleclient,
        .event_handler = &display::event_handler,
        .event_handler_argument = this};

    auto handle = library_.create_display(&context);
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
        disp->invalidate_(
            static_cast<const display_library::InvalidateEvent*>(event_data)
                ->rect);
        break;
    }
  }

  const display_library& library_;
  int handle_ = -1;

  invalidate_handler invalidate_;
};

}  // namespace vidicon
