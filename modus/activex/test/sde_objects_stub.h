#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>
#include <span>
#include <vector>

namespace modus {

class ATL_NO_VTABLE SdeObjectsStub
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IDispatchImpl<SDECore::ISDEObjects2> {
 public:
  static HRESULT Create(std::span<const CComPtr<SDECore::ISDEObject50>> objects,
                        SDECore::ISDEObjects2** Value) {
    if (!Value) {
      return E_POINTER;
    }

    CComObject<SdeObjectsStub>* sde_objects = nullptr;
    if (HRESULT hr = CComObject<SdeObjectsStub>::CreateInstance(&sde_objects);
        FAILED(hr)) {
      return hr;
    }

    sde_objects->objects_.assign(objects.begin(), objects.end());

    *Value = CComPtr{sde_objects}.Detach();
    return S_OK;
  }

  BEGIN_COM_MAP(SdeObjectsStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::ISDEObjects2)
  END_COM_MAP()

  // SDECore::ISDEObjects2

  STDMETHODIMP get_Count(
      /*[out,retval]*/ long* Value) override {
    if (!Value) {
      return E_POINTER;
    }

    *Value = objects_.size();
    return S_OK;
  }

  STDMETHODIMP get__NewEnum(
      /*[out,retval]*/ IUnknown** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Name(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Item(
      /*[in]*/ VARIANT Index,
      /*[out,retval]*/ SDECore::ISDEObject50** Value) override {
    CComVariant v{Index};
    if (HRESULT hr = v.ChangeType(VT_UI8); FAILED(hr)) {
      return hr;
    }
    auto index = v.ullVal;
    if (index < 0 || index >= objects_.size()) {
      return E_BOUNDS;
    }
    return objects_[index].CopyTo(Value);
  }

  STDMETHODIMP GetList(
      /*[in]*/ SDECore::TxStatQuery QueryType,
      /*[in]*/ BSTR ElemType,
      /*[in]*/ BSTR Param,
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP CreateView(
      /*[in]*/ BSTR TypeName,
      /*[in]*/ BSTR Params,
      /*[out,retval]*/ SDECore::ISDEObject50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP RemoveView(
      /*[in]*/ SDECore::ISDEObject50* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP Clear() override { return E_NOTIMPL; }
  STDMETHODIMP AddView(
      /*[in]*/ SDECore::ISDEObject50* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP IsPresent(
      /*[in]*/ SDECore::ISDEObject50* Value,
      /*[out,retval]*/ VARIANT_BOOL* Result) override {
    return E_NOTIMPL;
  }

  std::vector<CComPtr<SDECore::ISDEObject50>> objects_;
};

}  // namespace modus