#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>
#include <span>
#include <vector>

namespace modus {

class ATL_NO_VTABLE NamedPbsStub
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IDispatchImpl<SDECore::INamedPBs> {
 public:
  static HRESULT Create(std::span<const CComPtr<SDECore::INamedPB>> named_pbs,
                        SDECore::INamedPBs** Value) {
    if (!Value) {
      return E_POINTER;
    }

    CComObject<NamedPbsStub>* result = nullptr;
    if (HRESULT hr = CComObject<NamedPbsStub>::CreateInstance(&result);
        FAILED(hr)) {
      return hr;
    }

    result->named_pbs_.assign(named_pbs.begin(), named_pbs.end());

    *Value = CComPtr{result}.Detach();
    return S_OK;
  }

  BEGIN_COM_MAP(NamedPbsStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::INamedPBs)
  END_COM_MAP()

  // SDECore::INamedPBs

  STDMETHODIMP get_Count(
      /*[out,retval]*/ long* Value) override {
    if (!Value) {
      return E_POINTER;
    }

    *Value = named_pbs_.size();
    return S_OK;
  }

  STDMETHODIMP get_Item(
      /*[in]*/ VARIANT Index,
      /*[out,retval]*/ SDECore::INamedPB** Value) override {
    CComVariant v{Index};
    if (HRESULT hr = v.ChangeType(VT_UI8); FAILED(hr)) {
      return hr;
    }
    auto index = v.ullVal;
    if (index < 0 || index >= named_pbs_.size()) {
      return E_BOUNDS;
    }
    return named_pbs_[index].CopyTo(Value);
  }

  STDMETHODIMP get__NewEnum(
      /*[out,retval]*/ IUnknown** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Name(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Name(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP AddPB(
      /*[in]*/ SDECore::INamedPB* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP CreatePB(
      /*[in]*/ BSTR TypeName,
      /*[in]*/ BSTR KeyLink,
      /*[out,retval]*/ SDECore::INamedPB** PB) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_UserProp(
      /*[out,retval]*/ SDECore::IUserProperties** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP RemovePB(
      /*[in]*/ VARIANT Index) override {
    return E_NOTIMPL;
  }

  std::vector<CComPtr<SDECore::INamedPB>> named_pbs_;
};

}  // namespace modus