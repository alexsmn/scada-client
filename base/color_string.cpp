#include "base/color_string.h"

#include "base/format.h"
#include "base/win/scoped_select_object.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"

#include <cassert>
#include <exception>
#include <windows.h>

inline const char* strnchr(const char* str, int len, int ch) {
  while (len--) {
    if (*str == ch)
      return str;
    str++;
  }
  return NULL;
}

void DrawColoredStringHelper(HDC dc,
                             int x,
                             int y,
                             const std::wstring& t,
                             bool measure,
                             int& width,
                             int& height) {
  int len = t.length();
  const wchar_t* text = t.c_str();

  assert(len > 0);

  width = 0;
  height = 0;

  SIZE size;
  int origin = x;
  while (len) {
    const wchar_t* p = wcschr(text, L'&');
    if (!p)
      break;

    // draw block
    int block_len = (int)(p - text);
    if (measure) {
      GetTextExtentPoint32(dc, text, block_len, &size);
      width += size.cx;
      if (height < size.cy)
        height = size.cy;
    } else {
      int cx =
          LOWORD(TabbedTextOut(dc, x, y, text, block_len, 0, NULL, origin));
      x += cx;
    }

    // Parse tag
    const wchar_t* tag = p + 1;
    p = wcschr(tag, L';');
    if (!p)
      throw std::exception("bad colored string", 1);
    int tag_len = (int)(p - tag);
    if (tag_len == 0) {
      // Escape sequence: &;
    } else if (_wcsnicmp(L"color:", tag, std::min(tag_len, 6)) == 0) {
      if (!measure) {
        int color = ParseWithDefault(tag + 6, 0);
        SetTextColor(dc, color);
      }
    } else
      throw std::exception("bad colored string", 1);

    // move to next string
    len -= (int)(p - text) + 1;
    text = p + 1;
  }

  if (len) {
    if (measure) {
      GetTextExtentPoint32(dc, text, len, &size);
      width += size.cx;
      if (height < size.cy)
        height = size.cy;
    } else {
      TabbedTextOut(dc, x, y, text, len, 0, NULL, origin);
    }
  }
}

void MeasureColoredString(gfx::Canvas* canvas,
                          const gfx::Font& font,
                          SkColor color,
                          RECT& rect,
                          const std::wstring& text,
                          int format) {
  if (text.empty()) {
    if (format & DT_RIGHT)
      rect.left = rect.right;
    else
      rect.right = rect.left;
    rect.bottom = rect.top;
    return;
  }

  base::win::ScopedSelectObject select_font(canvas->native_canvas(),
                                            font.GetNativeFont());

  int x = rect.left;
  int y = rect.top;
  int width = 0, height = 0;

  DrawColoredStringHelper(canvas->native_canvas(), x, y, text, true, width,
                          height);

  if (format & DT_CENTER)
    x = (rect.left + rect.right - width) / 2;
  else if (format & DT_RIGHT)
    x = rect.right - width;
  if (format & DT_VCENTER)
    y = (rect.top + rect.bottom - height) / 2;
  else if (format & DT_BOTTOM)
    y = rect.bottom - height;

  rect.left = x;
  rect.top = y;
  rect.right = x + width;
  rect.bottom = y + height;
}

void DrawColoredString(gfx::Canvas* canvas,
                       const gfx::Font& font,
                       SkColor color,
                       const RECT& rect,
                       const std::wstring& text,
                       int format) {
  if (text.empty())
    return;

  base::win::ScopedSelectObject select_font(canvas->native_canvas(),
                                            font.GetNativeFont());
  COLORREF old_color =
      SetTextColor(canvas->native_canvas(), skia::SkColorToCOLORREF(color));
  int old_bk_mode = SetBkMode(canvas->native_canvas(), TRANSPARENT);

  int x = rect.left;
  int y = rect.top;
  int width = 0, height = 0;
  if (format & (DT_CENTER | DT_RIGHT | DT_VCENTER)) {
    DrawColoredStringHelper(canvas->native_canvas(), x, y, text, true, width,
                            height);
    if (format & DT_CENTER)
      x = (rect.left + rect.right - width) / 2;
    else if (format & DT_RIGHT)
      x = rect.right - width;
    if (format & DT_VCENTER)
      y = (rect.top + rect.bottom - height) / 2;
    else if (format & DT_BOTTOM)
      y = rect.bottom - height;
  }

  DrawColoredStringHelper(canvas->native_canvas(), x, y, text, false, width,
                          height);

  SetBkMode(canvas->native_canvas(), old_bk_mode);
  SetTextColor(canvas->native_canvas(), old_color);
}

class ColoredStringPainter {
 public:
  struct format_t {
    bool bold;
    bool italic;
    COLORREF color;
  };

  const char* str_;
  size_t len_;
  format_t format_;
  const char* text_;
  size_t text_len_;

  enum token_t { TEXT, LB, RB, TAG, EOL };

  token_t Parse() {
    if (!len_)
      return EOL;

    else if ((*str_ == '{' || *str_ == '}' || *str_ == '\\') &&
             (len_ == 1 || str_[0] != str_[1])) {
      const char ch = *str_;
      ++str_, --len_;
      if (ch == '{')
        return LB;
      else if (ch == '}')
        return RB;
      else {
        if (len_)
          throw std::exception();
        text_ = str_;
        ++str_, --len_;
        return TAG;
      }

    } else {
      text_ = str_;
      return TEXT;
    }
  }

  void process() {
    for (;;) {
      const char* start = str_;
      token_t token = Parse();
      switch (token) {
        case EOL:
        case RB:
          break;
        case TEXT:
          process_text(start, str_ - start);
        case TAG:
          process_tag(start, str_ - start);
          break;
        case LB: {
          format_t save_format = format_;
          process();
          format_ = save_format;
          break;
        }
        default:
          break;
      }
    }
  }

  void process_text(const char* str, size_t len) {}

  void process_tag(const char* str, size_t len) {}
};
