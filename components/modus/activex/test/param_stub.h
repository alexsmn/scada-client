#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>

namespace modus {

class ATL_NO_VTABLE ParamStub : public CComObjectRootEx<CComMultiThreadModel>,
                                public IDispatchImpl<SDECore::IParam> {
 public:
  static CComPtr<ParamStub> CreateOrDie(std::wstring_view name,
                                        std::wstring_view value) {
    CComObject<ParamStub>* result = nullptr;
    if (FAILED(CComObject<ParamStub>::CreateInstance(&result))) {
      throw std::runtime_error{"Cannot create a parameter"};
    }

    result->name_ = name;
    result->value_ = value;

    return CComPtr<ParamStub>{result}.Detach();
  }

  BEGIN_COM_MAP(ParamStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::IParam)
  END_COM_MAP()

  // SDECore::IParam

  STDMETHODIMP get_Name(
      /*[out,retval]*/ BSTR* Value) override {
    return CreateBstr(name_, Value);
  }

  STDMETHODIMP get_ValuesCount(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_IndexedValue(
      /*[in]*/ long Index,
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_IndexedValue(
      /*[in]*/ long Index,
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Value(
      /*[out,retval]*/ BSTR* Value) override {
    return CreateBstr(value_, Value);
  }

  STDMETHODIMP put_Value(
      /*[in]*/ BSTR Value) override {
    if (!Value) {
      return E_INVALIDARG;
    }

    value_ = Value;
    return S_OK;
  }

  STDMETHODIMP get_Category(
      /*[out,retval]*/ SDECore::TxParamCategory* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Category(
      /*[in]*/ SDECore::TxParamCategory Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Values(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Values(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP Info(
      /*[out,retval]*/ SDECore::IParamInfo** Info) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Mode(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Mode(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Dim(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Dim(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }

  static HRESULT CreateBstr(std::wstring_view str, BSTR* result) {
    if (!result) {
      return E_POINTER;
    }
    *result = CComBSTR{static_cast<int>(str.size()), str.data()}.Detach();
    return S_OK;
  }

  std::wstring name_;
  std::wstring value_;
};

}  // namespace modus