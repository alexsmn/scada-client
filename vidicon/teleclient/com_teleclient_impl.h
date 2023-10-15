#pragma once

#include "vidicon/teleclient/com_data_point_manager.h"

#include <atlbase.h>

#include <TeleClient.h>
#include <atlcom.h>
#include <cassert>
#include <wrl/client.h>

namespace vidicon {

class ATL_NO_VTABLE ComTeleclientImpl
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IDispatchImpl<IClient,
                           &__uuidof(IClient),
                           &LIBID_TeleClientLib,
                           /*wMajor =*/1,
                           /*wMinor =*/0> {
 public:
  void Init(ComDataPointManager& com_data_point_manager);

 private:
  BEGIN_COM_MAP(ComTeleclientImpl)
  COM_INTERFACE_ENTRY(IClient)
  COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  // IClient
  STDMETHOD(RequestPoint)(BSTR Name, IDataPoint** Point);
  STDMETHOD(get_GlobAttrib)(BSTR Name, VARIANT* pVal);
  STDMETHOD(get_XmlConfig)(BSTR Name, IDispatch** pVal);
  STDMETHOD(get_XmlNode)(BSTR Name, ULONG ID, IDispatch** pVal);
  STDMETHOD(get_PointProp)(BSTR Name, ULONG ID, VARIANT* pVal);
  STDMETHOD(Evalute)(BSTR Text, BSTR* pVal);

  ComDataPointManager* com_data_point_manager_ = nullptr;
};

inline Microsoft::WRL::ComPtr<IClient> CreateComTeleClient(
    ComDataPointManager& com_data_point_manager) {
  auto* com_teleclient = new CComObjectNoLock<ComTeleclientImpl>();
  com_teleclient->Init(com_data_point_manager);
  return com_teleclient;
}

void ComTeleclientImpl::Init(ComDataPointManager& com_data_point_manager) {
  com_data_point_manager_ = &com_data_point_manager;
}

STDMETHODIMP ComTeleclientImpl::RequestPoint(BSTR Name, IDataPoint** Point) {
  if (!Point)
    return E_POINTER;

  auto com_data_point = com_data_point_manager_->GetComDataPoint(Name);
  if (!com_data_point) {
    return E_FAIL;
  }

  *Point = com_data_point.Detach();
  return S_OK;
}

STDMETHODIMP ComTeleclientImpl::get_GlobAttrib(BSTR Name, VARIANT* pVal) {
  return E_NOTIMPL;
}

STDMETHODIMP ComTeleclientImpl::get_XmlConfig(BSTR Name, IDispatch** pVal) {
  return E_NOTIMPL;
}

STDMETHODIMP ComTeleclientImpl::get_XmlNode(BSTR Name,
                                            ULONG ID,
                                            IDispatch** pVal) {
  return E_NOTIMPL;
}

STDMETHODIMP ComTeleclientImpl::get_PointProp(BSTR Name,
                                              ULONG ID,
                                              VARIANT* pVal) {
  // Display effectively turns an error into an empty value. While it handles
  // errors badly.
  CComVariant value;
  return value.Detach(pVal);
}

STDMETHODIMP ComTeleclientImpl::Evalute(BSTR Text, BSTR* pVal) {
  CComBSTR value{Text};
  *pVal = value.Detach();
  return S_OK;
}

}  // namespace vidicon
