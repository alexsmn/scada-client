#pragma once

#include "base/stop_token.h"
#include "services/vidicon/com_data_point_events.h"
#include "services/vidicon/data_point_manager.h"
#include "services/vidicon/teleclient.h"

#include <atlbase.h>

#include <atlcom.h>
#include <boost/locale/encoding_utf.hpp>
#include <functional>
#include <memory>
#include <mutex>

namespace vidicon {

// WARNING: The object is accessed from multiple threads.
class ATL_NO_VTABLE ComDataPointImpl
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IDispatchImpl<IDataPoint,
                           &__uuidof(IDataPoint),
                           &LIBID_TeleClientLib,
                           /*wMajor =*/1,
                           /*wMinor =*/0>,
      public IDispatchImpl<IDataPointServer,
                           &__uuidof(IDataPointServer),
                           &LIBID_TeleClientLib,
                           /*wMajor =*/1,
                           /*wMinor =*/0>,
      public IDispatchImpl<IDataPoint3,
                           &__uuidof(IDataPoint3),
                           &LIBID_TeleClientLib,
                           /*wMajor =*/1,
                           /*wMinor =*/0> {
 public:
  using ReleaseHandler = std::function<void()>;

  void Init(DataPointManager& data_point_manager,
            const std::wstring& formula,
            ReleaseHandler release_handler);

  ULONG InternalRelease();

 private:
  BEGIN_COM_MAP(ComDataPointImpl)
  COM_INTERFACE_ENTRY(IDataPoint)
  COM_INTERFACE_ENTRY(IDataPointServer)
  COM_INTERFACE_ENTRY(IDataPoint3)
  COM_INTERFACE_ENTRY2(IDispatch, IDataPoint3)
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

  ReleaseHandler release_handler_;

  mutable std::mutex mutex_;
  DataPointValue data_value_;

  const scoped_stop_source stop_source_;
};

inline void ComDataPointImpl::Init(DataPointManager& data_point_manager,
                                   const std::wstring& formula,
                                   ReleaseHandler release_handler) {
  release_handler_ = std::move(release_handler);

  data_point_manager.Subscribe(
      boost::locale::conv::utf_to_utf<char>(formula), stop_source_.get_token(),
      [connection_points = connection_points_](const DataPointValue& value) {
        connection_points->NotifyDataChanged(value);
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
  if (!pVal)
    return E_POINTER;

  std::lock_guard lock{mutex_};
  CComVariant value_copy{data_value_.value};
  return value_copy.Detach(pVal);
}

inline STDMETHODIMP ComDataPointImpl::put_Value(VARIANT newVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Time(DATE* pVal) {
  if (!pVal)
    return E_POINTER;

  std::lock_guard lock{mutex_};
  *pVal = data_value_.time;
  return S_OK;
}

inline STDMETHODIMP ComDataPointImpl::get_Quality(ULONG* pVal) {
  if (!pVal)
    return E_POINTER;

  std::lock_guard lock{mutex_};
  *pVal = data_value_.quality;
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

}  // namespace vidicon
