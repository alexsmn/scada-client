#pragma once

#include "components/vidicon_display/data_point.h"
#include "components/vidicon_display/teleclient.h"

#include <atlbase.h>

#include <atlcom.h>
#include <functional>
#include <memory>

class ComDataPointConnectionPoints
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IConnectionPointContainerImpl<ComDataPointConnectionPoints>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(TeleClientLib::_IDataPointEvents)>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(TeleClientLib::_IDataPointEvents3)>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(
                                      TeleClientLib::_IDataPointEventsEx)> {
 public:
  void NotifyDataChanged() {}

  BEGIN_COM_MAP(ComDataPointConnectionPoints)
  COM_INTERFACE_ENTRY(IConnectionPointContainer)
  END_COM_MAP()

  BEGIN_CONNECTION_POINT_MAP(ComDataPointConnectionPoints)
  CONNECTION_POINT_ENTRY(__uuidof(TeleClientLib::_IDataPointEvents))
  CONNECTION_POINT_ENTRY(__uuidof(TeleClientLib::_IDataPointEventsEx))
  CONNECTION_POINT_ENTRY(__uuidof(TeleClientLib::_IDataPointEvents3))
  END_CONNECTION_POINT_MAP()
};

class ComDataPointImpl
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IDispatchImpl<TeleClientLib::IDataPoint,
                           &__uuidof(TeleClientLib::IDataPoint),
                           &__uuidof(TeleClientLib::__TeleClientLib),
                           /*wMajor =*/1,
                           /*wMinor =*/0>,
      public IDispatchImpl<TeleClientLib::IDataPointServer,
                           &__uuidof(TeleClientLib::IDataPointServer),
                           &__uuidof(TeleClientLib::__TeleClientLib),
                           /*wMajor =*/1,
                           /*wMinor =*/0>,
      public IDispatchImpl<TeleClientLib::IDataPoint3,
                           &__uuidof(TeleClientLib::IDataPoint3),
                           &__uuidof(TeleClientLib::__TeleClientLib),
                           /*wMajor =*/1,
                           /*wMinor =*/0> {
 public:
  using ReleaseHandler = std::function<void()>;

  void Init(std::shared_ptr<DataPoint> data_point,
            ReleaseHandler release_handler);

  ULONG InternalRelease();

 private:
  BEGIN_COM_MAP(ComDataPointImpl)
  COM_INTERFACE_ENTRY(TeleClientLib::IDataPoint)
  COM_INTERFACE_ENTRY(TeleClientLib::IDataPointServer)
  COM_INTERFACE_ENTRY(TeleClientLib::IDataPoint3)
  COM_INTERFACE_ENTRY2(IDispatch, TeleClientLib::IDataPoint3)
  COM_INTERFACE_ENTRY_AGGREGATE(IID_IConnectionPointContainer,
                                connection_points_)
  END_COM_MAP()

  // IDataPoint
  STDMETHOD(get_Value)(VARIANT* pVal);
  STDMETHOD(put_Value)(VARIANT newVal);
  STDMETHOD(get_Time)(DATE* pVal);
  STDMETHOD(get_Quality)(ULONG* pVal);
  STDMETHOD(get_ValueStr)(BSTR* pVal);
  STDMETHOD(get_Attrib)(BSTR Name, VARIANT* pVal);
  STDMETHOD(get_Address)(BSTR* pVal);
  STDMETHOD(get_Prop)(ULONG ID, VARIANT* pVal);
  STDMETHOD(put_Prop)(ULONG ID, VARIANT newVal);
  STDMETHOD(get_XmlNode)(IDispatch** pVal);
  STDMETHOD(get_Connected)(ULONG* pVal);
  STDMETHOD(get_ErrorStr)(ULONG Error, BSTR* pVal);
  STDMETHOD(get_AccessRights)(ULONG* pVal);
  STDMETHOD(Write)(VARIANT Value);
  STDMETHOD(WriteAsync)(VARIANT Value, ULONG* TransID);
  STDMETHOD(CancelAsync)(ULONG TransID);
  STDMETHOD(Ack)(BSTR Acker, BSTR Comment);

  // IDataPointServer
  STDMETHOD(get_OPCServer)(IUnknown** pVal);

  // IDataPoint3
  STDMETHOD(Call)
  (BSTR MethodName, ULONG ArgumentCount, VARIANT* Arguments, VARIANT* Result);

  const CComPtr<ComDataPointConnectionPoints> connection_points_ =
      new CComObjectNoLock<ComDataPointConnectionPoints>();

  std::shared_ptr<DataPoint> data_point_;
  ReleaseHandler release_handler_;
  boost::signals2::scoped_connection data_changed_connection_;
};

inline void ComDataPointImpl::Init(std::shared_ptr<DataPoint> data_point,
                                   ReleaseHandler release_handler) {
  assert(data_point);

  data_point_ = std::move(data_point);
  release_handler_ = std::move(release_handler);

  data_changed_connection_ = data_point_->data_changed_signal.connect(
      [connection_points = connection_points_] {
        connection_points->NotifyDataChanged();
      });
}

ULONG ComDataPointImpl::InternalRelease() {
  auto count = CComObjectRootEx<CComMultiThreadModelNoCS>::InternalRelease();
  if (count == 1)
    release_handler_();
  return count;
}

inline STDMETHODIMP ComDataPointImpl::get_OPCServer(IUnknown** pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Value(VARIANT* pVal) {
  auto value = data_point_->GetValue();
  return value.Detach(pVal);
}

inline STDMETHODIMP ComDataPointImpl::put_Value(VARIANT newVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Time(DATE* pVal) {
  if (!pVal)
    return E_POINTER;

  *pVal = data_point_->GetTime();
  return S_OK;
}

inline STDMETHODIMP ComDataPointImpl::get_Quality(ULONG* pVal) {
  if (!pVal)
    return E_POINTER;

  *pVal = data_point_->GetQuality();
  return S_OK;
}

inline STDMETHODIMP ComDataPointImpl::get_ValueStr(BSTR* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Attrib(BSTR Name, VARIANT* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Address(BSTR* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Prop(ULONG ID, VARIANT* pVal) {
  CComVariant value;
  return value.Detach(pVal);
}

inline STDMETHODIMP ComDataPointImpl::put_Prop(ULONG ID, VARIANT newVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_XmlNode(IDispatch** pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Connected(ULONG* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_ErrorStr(ULONG Error, BSTR* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_AccessRights(ULONG* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::Write(VARIANT Value) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::WriteAsync(VARIANT Value,
                                                 ULONG* TransID) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::CancelAsync(ULONG TransID) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::Ack(BSTR Acker, BSTR Comment) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::Call(BSTR MethodName,
                                           ULONG ArgumentCount,
                                           VARIANT* Arguments,
                                           VARIANT* Result) {
  return E_NOTIMPL;
}
