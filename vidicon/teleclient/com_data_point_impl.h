#pragma once

#include "base/stop_token.h"
#include "opc/opc_convertions.h"
#include "vidicon/teleclient/com_data_point_events.h"
#include "vidicon/teleclient/data_point_manager.h"

#include <atlbase.h>

#include <TeleClient.h>
#include <atlcom.h>
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
                           /*wMinor =*/0> {
 public:
  using ReleaseHandler = std::function<void()>;

  void Init(DataPointManager& data_point_manager,
            const DataPointAddress& address,
            ReleaseHandler release_handler);

  ULONG InternalRelease();

 private:
  BEGIN_COM_MAP(ComDataPointImpl)
  COM_INTERFACE_ENTRY(IDataPoint)
  COM_INTERFACE_ENTRY_AGGREGATE(IID_IConnectionPointContainer,
                                connection_points_)
  END_COM_MAP()

  // IDataPoint
  STDMETHOD(get_Value)(VARIANT* pVal) override;
  STDMETHOD(put_Value)(VARIANT newVal) override;
  STDMETHOD(get_Time)(DATE* pVal) override;
  STDMETHOD(get_Quality)(ULONG* pVal) override;
  STDMETHOD(get_ValueStr)(BSTR* pVal) override;
  STDMETHOD(get_Status)(ULONG* pVal) override;
  STDMETHOD(get_Attrib)(BSTR Name, VARIANT* pVal) override;
  STDMETHOD(get_Address)(BSTR* pVal) override;
  STDMETHOD(get_Prop)(ULONG ID, VARIANT* pVal) override;
  STDMETHOD(put_Prop)(ULONG ID, VARIANT newVal) override;
  STDMETHOD(get_XmlNode)(IDispatch** pVal) override;
  STDMETHOD(get_ErrorStr)(ULONG Error, BSTR* pVal) override;
  STDMETHOD(get_AccessRights)(ULONG* pVal) override;
  STDMETHOD(Read)
  (ULONG* status, VARIANT* value, ULONG* quality, DATE* timestamp) override;
  STDMETHOD(Write)(VARIANT Value);
  STDMETHOD(WriteAsync)(VARIANT Value, ULONG* TransID) override;
  STDMETHOD(CancelAsync)(ULONG TransID) override;
  STDMETHOD(Ack)(BSTR Acker, BSTR Comment) override;

  // IDataPointServer
  STDMETHOD(get_OPCServer)(IUnknown** pVal);

  // IDataPoint3
  STDMETHOD(Call)
  (BSTR MethodName, ULONG ArgumentCount, VARIANT* Arguments, VARIANT* Result);

  const CComPtr<ComDataPointConnectionPoints> connection_points_ =
      new CComObjectNoLock<ComDataPointConnectionPoints>();

  ReleaseHandler release_handler_;

  mutable std::mutex mutex_;
  opc_client::DataValue data_value_;

  const scoped_stop_source stop_source_;
};

inline void ComDataPointImpl::Init(DataPointManager& data_point_manager,
                                   const DataPointAddress& address,
                                   ReleaseHandler release_handler) {
  release_handler_ = std::move(release_handler);

  data_point_manager.Subscribe(
      address, stop_source_.get_token(),
      [connection_points =
           connection_points_](const opc_client::DataValue& data_value) {
        connection_points->NotifyDataChanged(data_value);
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
  CComVariant value_copy = data_value_.value;
  return value_copy.Detach(pVal);
}

inline STDMETHODIMP ComDataPointImpl::put_Value(VARIANT newVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Time(DATE* pVal) {
  if (!pVal)
    return E_POINTER;

  std::lock_guard lock{mutex_};
  *pVal = opc::ToDATE(data_value_.timestamp);
  return S_OK;
}

inline STDMETHODIMP ComDataPointImpl::get_Quality(ULONG* pVal) {
  if (!pVal)
    return E_POINTER;

  std::lock_guard lock{mutex_};
  *pVal = data_value_.quality.raw();
  return S_OK;
}

inline STDMETHODIMP ComDataPointImpl::get_ValueStr(BSTR* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Status(ULONG* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Attrib(BSTR Name, VARIANT* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Address(BSTR* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_Prop(ULONG ID, VARIANT* pVal) {
  // Display effectively turns an error into an empty value. While it handles
  // errors badly.
  CComVariant value;
  return value.Detach(pVal);
}

inline STDMETHODIMP ComDataPointImpl::put_Prop(ULONG ID, VARIANT newVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_XmlNode(IDispatch** pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_ErrorStr(ULONG Error, BSTR* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::get_AccessRights(ULONG* pVal) {
  return E_NOTIMPL;
}

inline STDMETHODIMP ComDataPointImpl::Read(ULONG* status,
                                           VARIANT* value,
                                           ULONG* quality,
                                           DATE* timestamp) {
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
