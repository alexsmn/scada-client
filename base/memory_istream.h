#pragma once

#include <Windows.h>
#include <cassert>
th class MemoryIStream : public IStream {
 public:
  MemoryIStream(BYTE* data, DWORD size, DWORD capacity = 0) noexcept {
    assert(data);
    data_ = data;
    pos_ = 0;
    size_ = size;
    capacity_ = capacity ? capacity : size;
  }

  ~MemoryIStream() {
#ifndef NDEBUG
    assert(ref_count_ == 0);
#endif
  }

  DWORD size() const { return size_; }

  STDMETHOD(Read)(void* pv, ULONG cb, ULONG* pcbRead) noexcept {
    if (!pv)
      return E_INVALIDARG;

    if (cb > size_ - pos_)
      cb = size_ - pos_;

    if (cb) {
      if (!data_)
        return E_UNEXPECTED;
      BYTE* pCurr = data_;
      pCurr += pos_;
      memcpy(pv, pCurr, cb);
      pos_ += cb;
    }

    if (pcbRead)
      *pcbRead = cb;
    return S_OK;
  }

  STDMETHOD(Write)(const void* pv, ULONG cb, ULONG* pcbWrite) noexcept {
    if (!pv)
      return E_INVALIDARG;

    if (cb > capacity_ - pos_)
      cb = capacity_ - pos_;

    if (cb) {
      if (!data_)
        return E_UNEXPECTED;
      BYTE* pCurr = data_;
      pCurr += pos_;
      memcpy(pCurr, pv, cb);
      pos_ += cb;
      if (size_ <= pos_)
        size_ = pos_ + 1;
    }

    if (pcbWrite)
      *pcbWrite = cb;
    return S_OK;
  }

  STDMETHOD(Seek)
  (LARGE_INTEGER dlibMove,
   DWORD dwOrigin,
   ULARGE_INTEGER* pLibNewPosition) noexcept {
    switch (dwOrigin) {
      case STREAM_SEEK_CUR:
        pos_ += (DWORD)dlibMove.QuadPart;
        break;
      case STREAM_SEEK_END:
        pos_ = size_ - (DWORD)dlibMove.QuadPart;
        break;
      case STREAM_SEEK_SET:
        pos_ = (DWORD)dlibMove.QuadPart;
        break;
      default:
        return E_INVALIDARG;
    }

    if (pLibNewPosition)
      pLibNewPosition->QuadPart = pos_;
    return S_OK;
  }

  STDMETHOD(SetSize)(ULARGE_INTEGER) noexcept { return E_NOTIMPL; }

  STDMETHOD(CopyTo)
  (IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) noexcept {
    return E_NOTIMPL;
  }

  STDMETHOD(Commit)(DWORD) noexcept { return E_NOTIMPL; }

  STDMETHOD(Revert)(void) noexcept { return E_NOTIMPL; }

  STDMETHOD(LockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) noexcept {
    return E_NOTIMPL;
  }

  STDMETHOD(UnlockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) noexcept {
    return E_NOTIMPL;
  }

  STDMETHOD(Stat)(STATSTG*, DWORD) noexcept { return E_NOTIMPL; }

  STDMETHOD(Clone)(IStream**) noexcept { return E_NOTIMPL; }

  STDMETHOD(QueryInterface)(REFIID iid, void** ppUnk) noexcept {
    *ppUnk = nullptr;
    if (::InlineIsEqualGUID(iid, IID_IUnknown) ||
        ::InlineIsEqualGUID(iid, IID_ISequentialStream) ||
        ::InlineIsEqualGUID(iid, IID_IStream)) {
      *ppUnk = (void*)(IStream*)this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef(void) noexcept {
#ifndef NDEBUG
    ++ref_count_;
#endif

    return 1;
  }

  ULONG STDMETHODCALLTYPE Release(void) noexcept {
#ifndef NDEBUG
    assert(ref_count_ > 0);
    --ref_count_;
#endif

    return 1;
  }

 private:
  BYTE* data_;
  DWORD pos_;
  DWORD size_;
  DWORD capacity_;

#ifndef NDEBUG
  size_t ref_count_ = 0;
#endif
};
