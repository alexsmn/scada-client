#pragma once

#include "components/vidicon_display/teleclient/com_data_point_manager.h"
#include "components/vidicon_display/teleclient/teleclient.h"

#include <atlbase.h>

#include <atlcom.h>

class ComTeleclientImpl
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IDispatchImpl<TeleClientLib::IClient,
                           &__uuidof(TeleClientLib::IClient),
                           &__uuidof(TeleClientLib::__TeleClientLib),
                           /*wMajor =*/1,
                           /*wMinor =*/0> {
 public:
  void Init(ComDataPointManager& com_data_point_manager);

 private:
  BEGIN_COM_MAP(ComTeleclientImpl)
  COM_INTERFACE_ENTRY(TeleClientLib::IClient)
  COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  // IClient
  STDMETHOD(RequestPoint)(BSTR Name, TeleClientLib::IDataPoint** Point);
  STDMETHOD(get_GlobAttrib)(BSTR Name, VARIANT* pVal);
  STDMETHOD(get_XmlConfig)(BSTR Name, IDispatch** pVal);
  STDMETHOD(get_XmlNode)(BSTR Name, ULONG ID, IDispatch** pVal);
  STDMETHOD(get_PointProp)(BSTR Name, ULONG ID, VARIANT* pVal);
  STDMETHOD(Evalute)(BSTR Text, BSTR* pVal);

  ComDataPointManager* com_data_point_manager_ = nullptr;
};

void ComTeleclientImpl::Init(ComDataPointManager& com_data_point_manager) {
  com_data_point_manager_ = &com_data_point_manager;
}

STDMETHODIMP ComTeleclientImpl::RequestPoint(
    BSTR Name,
    TeleClientLib::IDataPoint** Point) {
  if (!Point)
    return E_POINTER;

  auto com_data_point = com_data_point_manager_->GetComDataPoint(Name);
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
  return E_NOTIMPL;
}

STDMETHODIMP ComTeleclientImpl::Evalute(BSTR Text, BSTR* pVal) {
  CComBSTR value{Text};
  *pVal = value.Detach();
  return S_OK;
}