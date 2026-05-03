#pragma once

#include "modus/activex/modus.h"
#include "modus/activex/test/params_stub.h"

#include <atlbase.h>

#include <atlcom.h>
#include <vector>

namespace modus {

class ATL_NO_VTABLE NamedPbStub : public CComObjectRootEx<CComMultiThreadModel>,
                                  public IDispatchImpl<SDECore::INamedPB> {
 public:
  BEGIN_COM_MAP(NamedPbStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::INamedPB)
  END_COM_MAP()

  // SDECore::INamedPB

  STDMETHODIMP get_TypeName(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_TypeName(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_KeyLink(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_KeyLink(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Params(
      /*[out,retval]*/ SDECore::IParams** Value) override {
    return ParamsStub::Create(params_, Value);
  }

  STDMETHODIMP Clone(
      /*[in]*/ BSTR NamedPBs,
      /*[out,retval]*/ SDECore::INamedPB** Value) override {
    return E_NOTIMPL;
  }

  std::vector<CComPtr<SDECore::IParam>> params_;
};

}  // namespace modus