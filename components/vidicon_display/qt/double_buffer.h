#pragma once

class DoubleBuffer {
 public:
  DoubleBuffer(HDC dc, const RECT& rect) : dc{dc}, rect{rect} {
    mem_dc = ::CreateCompatibleDC(dc);
    bitmap = ::CreateCompatibleBitmap(dc, rect.right - rect.left,
                                      rect.bottom - rect.top);
    select_bitmap = ::SelectObject(mem_dc, bitmap);
  }

  ~DoubleBuffer() {
    /*::BitBlt(dc, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                  mem_dc,
             0, 0, SRCCOPY);*/
    ::SelectObject(mem_dc, select_bitmap);
    ::DeleteObject(bitmap);
    ::DeleteDC(mem_dc);
  }

  HBITMAP buffer_bitmap() const { return bitmap; }

  HDC buffer_dc() const { return mem_dc; }

 private:
  HDC dc;
  HDC mem_dc;
  HBITMAP bitmap;
  HGDIOBJ select_bitmap;
  RECT rect;
};
