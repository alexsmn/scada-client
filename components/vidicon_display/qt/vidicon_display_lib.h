#pragma once

#include <filesystem>
#include <windows.h>

namespace vidicon {

struct display_library {
  const HMODULE lib = ::LoadLibraryW(kLibraryPath);

  using CreateDisplayFunc = int(__cdecl*)(LPCWSTR path);
  const CreateDisplayFunc create_display =
      reinterpret_cast<CreateDisplayFunc>(GetProcAddress(lib, "CreateDisplay"));

  using DestroyDisplayFunc = void(__cdecl*)(int handle);
  const DestroyDisplayFunc destroy_display =
      reinterpret_cast<DestroyDisplayFunc>(
          GetProcAddress(lib, "DestroyDisplay"));

  using DrawDisplayFunc = void(__cdecl*)(int handle, HDC dc);
  const DrawDisplayFunc draw_display =
      reinterpret_cast<DrawDisplayFunc>(GetProcAddress(lib, "DrawDisplay"));

  inline static const wchar_t kLibraryPath[] =
      LR"(c:\tc\vidicon\Vidicon\bin\debug\DisplayLib.dll)";
};

class display {
 public:
  explicit display(const display_library& library) : library_{library} {}
  ~display() { library_.destroy_display(handle_); }

  bool is_opened() const { return handle_ >= 0; }

  void open(const std::filesystem::path& path) {
    if (is_opened()) {
      throw std::runtime_error{"The display is already opened"};
    }

    auto handle = library_.create_display(path.wstring().c_str());
    if (handle < 0) {
      throw std::runtime_error{"Cannot open display"};
    }

    handle_ = handle;
  }

  void draw(HDC dc) const {
    check_opened();

    library_.draw_display(handle_, dc);
  }

 private:
  void check_opened() const {
    if (!is_opened()) {
      throw std::runtime_error{"The display is not opened"};
    }
  }

  const display_library& library_;
  int handle_ = -1;
};

}  // namespace vidicon
