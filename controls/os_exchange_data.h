#pragma once

#include "base/memory/ref_counted.h"

#include <objidl.h>
#include <vector>

namespace base {
class Pickle;
}

namespace aui {

class DataObjectImpl : public IDataObject,
                       public base::RefCountedThreadSafe<DataObjectImpl> {
 public:
  DataObjectImpl();
  virtual ~DataObjectImpl();

  // IDataObject
  virtual HRESULT __stdcall GetData(FORMATETC* format_etc, STGMEDIUM* medium);
  virtual HRESULT __stdcall GetDataHere(FORMATETC* format_etc,
                                        STGMEDIUM* medium);
  virtual HRESULT __stdcall QueryGetData(FORMATETC* format_etc);
  virtual HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC* format_etc,
                                                  FORMATETC* result);
  virtual HRESULT __stdcall SetData(FORMATETC* format_etc,
                                    STGMEDIUM* medium,
                                    BOOL should_release);
  virtual HRESULT __stdcall EnumFormatEtc(DWORD direction,
                                          IEnumFORMATETC** enumerator);
  virtual HRESULT __stdcall DAdvise(FORMATETC* format_etc,
                                    DWORD advf,
                                    IAdviseSink* sink,
                                    DWORD* connection);
  virtual HRESULT __stdcall DUnadvise(DWORD connection);
  virtual HRESULT __stdcall EnumDAdvise(IEnumSTATDATA** enumerator);

  // IUnknown
  virtual HRESULT __stdcall QueryInterface(const IID& iid, void** object);
  virtual ULONG __stdcall AddRef();
  virtual ULONG __stdcall Release();

 private:
  friend class FormatEtcEnumerator;
  friend class OSExchangeData;

  // Removes from contents_ the first data that matches |format|.
  void RemoveData(const FORMATETC& format);

  // Our internal representation of stored data & type info.
  struct StoredDataInfo {
    FORMATETC format_etc;
    STGMEDIUM* medium;
    bool owns_medium;

    StoredDataInfo(CLIPFORMAT cf, STGMEDIUM* medium)
        : medium(medium), owns_medium(true) {
      format_etc.cfFormat = cf;
      format_etc.dwAspect = DVASPECT_CONTENT;
      format_etc.lindex = -1;
      format_etc.ptd = NULL;
      format_etc.tymed = medium ? medium->tymed : TYMED_HGLOBAL;
    }

    StoredDataInfo(FORMATETC* format_etc, STGMEDIUM* medium)
        : format_etc(*format_etc), medium(medium), owns_medium(true) {}

    ~StoredDataInfo() {
      if (owns_medium) {
        ReleaseStgMedium(medium);
        delete medium;
      }
    }
  };

  typedef std::vector<StoredDataInfo*> StoredData;
  StoredData contents_;
};

class OSExchangeData {
 public:
  OSExchangeData();
  explicit OSExchangeData(IDataObject* data_object);
  ~OSExchangeData();

  typedef CLIPFORMAT CustomFormat;

  bool HasCustomFormat(CustomFormat format) const;

  bool GetPickledData(CustomFormat format, base::Pickle& data) const;
  void SetPickledData(CustomFormat format, base::Pickle& data);

  IDataObject* GetIDataObject() const { return data_object_.get(); }

  static CustomFormat RegisterCustomFormat(const char* name);

 private:
  void SetData(CustomFormat format, const void* data, size_t size);

  scoped_refptr<DataObjectImpl> data_object_impl_;
  scoped_refptr<IDataObject> data_object_;

  DISALLOW_COPY_AND_ASSIGN(OSExchangeData);
};

}  // namespace aui
