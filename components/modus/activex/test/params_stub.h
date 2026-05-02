#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>
#include <span>
#include <vector>

namespace modus {

class ATL_NO_VTABLE ParamsStub : public CComObjectRootEx<CComMultiThreadModel>,
                                 public IDispatchImpl<SDECore::IParams52> {
 public:
  template <class T>
  static HRESULT Create(std::span<const CComPtr<SDECore::IParam>> params,
                        T** Value) {
    if (!Value) {
      return E_POINTER;
    }

    CComObject<ParamsStub>* result = nullptr;
    if (HRESULT hr = CComObject<ParamsStub>::CreateInstance(&result);
        FAILED(hr)) {
      return hr;
    }

    result->params_.assign(params.begin(), params.end());

    *Value = CComPtr{result}.Detach();
    return S_OK;
  }

  BEGIN_COM_MAP(ParamsStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::IParams)
  COM_INTERFACE_ENTRY(SDECore::IParams50)
  COM_INTERFACE_ENTRY(SDECore::IParams52)
  END_COM_MAP()

  // SDECore::IParams52

  STDMETHODIMP get_MethodCount(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_MethodInfo(
      /*[in]*/ VARIANT Index,
      /*[out,retval]*/ SDECore::IMethodInfo** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP MethodRun(
      /*[in]*/ VARIANT Index,
      /*[in,out]*/ VARIANT* Params,
      /*[out,retval]*/ VARIANT* Result) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP SParamInfo(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ SDECore::IParamInfo** Info) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SValue(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SValue(
      /*[in]*/ BSTR ParamNm,
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SView(
      /*[out,retval]*/ SDECore::ISDEObject50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SView(
      /*[in]*/ SDECore::ISDEObject50* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP SAddView(
      /*[in]*/ BSTR ParamNm,
      /*[in]*/ SDECore::ISDEObject50* SDEObject) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP SDeleteParam(
      /*[in]*/ BSTR ParamNm) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SSubObject(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ SDECore::IParams50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP SCreateSubObject(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ SDECore::IParams50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_ParamsList(
      /*[in]*/ BSTR Prefix,
      /*[in]*/ long RecursionLevel,
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SDisUsed(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SDisUsed(
      /*[in]*/ BSTR ParamNm,
      /*[in]*/ VARIANT_BOOL Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP FindParam(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ long* Index) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Count(
      /*[out,retval]*/ long* Value) override {
    if (!Value) {
      return E_POINTER;
    }

    *Value = params_.size();
    return S_OK;
  }

  STDMETHODIMP get_Item(
      /*[in]*/ VARIANT Index,
      /*[out,retval]*/ SDECore::IParam** Value) override {
    CComVariant v{Index};

    if (HRESULT hr = v.ChangeType(VT_UI8); SUCCEEDED(hr)) {
      auto index = v.ullVal;
      if (index < 0 || index >= params_.size()) {
        return E_BOUNDS;
      }
      return params_[index].CopyTo(Value);
    }

    if (HRESULT hr = v.ChangeType(VT_BSTR); SUCCEEDED(hr)) {
      if (!v.bstrVal) {
        return E_INVALIDARG;
      }
      auto name = std::wstring_view{v.bstrVal};
      auto i = std::ranges::find(params_, name,
                                 [&](CComPtr<SDECore::IParam>& param) {
                                   return GetParamName(*param);
                                 });
      if (i == params_.end()) {
        return E_INVALIDARG;
      }
      return i->CopyTo(Value);
    }

    return E_INVALIDARG;
  }

  STDMETHODIMP get__NewEnum(
      /*[out,retval]*/ IUnknown** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_AsText(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  static std::wstring GetParamName(SDECore::IParam& param) {
    CComBSTR name;
    param.get_Name(&name);
    return name ? std::wstring{static_cast<const wchar_t*>(name), name.Length()}
                : std::wstring{};
  }

  std::vector<CComPtr<SDECore::IParam>> params_;
};

}  // namespace modus