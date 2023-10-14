#pragma once

#include "vidicon/teleclient.h"

#include <atlbase.h>

#include <atlcom.h>
#include <functional>
#include <wrl/client.h>

namespace vidicon {

struct ComDataPointEventHanders {
  std::function<
      void(HRESULT status, const VARIANT& value, DATE time, UINT quality)>
      data_changed;
};

class ATL_NO_VTABLE ComDataPointEvents
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IDispatchImpl<_IDataPointEvents,
                           &__uuidof(_IDataPointEvents),
                           &LIBID_TeleClientLib> {
 public:
  BEGIN_COM_MAP(ComDataPointEvents)
  COM_INTERFACE_ENTRY(_IDataPointEvents)
  COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  void Init(const ComDataPointEventHanders& handlers);

  HRESULT DispatchDataChange(DISPPARAMS& disp_params);

  // IDispatch
  STDMETHOD(Invoke)
  (DISPID dispIdMember,
   REFIID riid,
   LCID lcid,
   WORD wFlags,
   DISPPARAMS* pDispParams,
   VARIANT* pVarResult,
   EXCEPINFO* pExcepInfo,
   UINT* puArgErr);

  ComDataPointEventHanders handlers_;
};

inline Microsoft::WRL::ComPtr<_IDataPointEvents> CreateComDataPointEvents(
    const ComDataPointEventHanders& handlers) {
  auto* com_data_point_events = new CComObjectNoLock<ComDataPointEvents>();
  com_data_point_events->Init(handlers);
  return com_data_point_events;
}

inline void ComDataPointEvents::Init(const ComDataPointEventHanders& handlers) {
  handlers_ = handlers;
}

inline STDMETHODIMP ComDataPointEvents::Invoke(DISPID dispIdMember,
                                               REFIID riid,
                                               LCID lcid,
                                               WORD wFlags,
                                               DISPPARAMS* pDispParams,
                                               VARIANT* pVarResult,
                                               EXCEPINFO* pExcepInfo,
                                               UINT* puArgErr) {
  if (pDispParams == nullptr) {
    return E_POINTER;
  }

  if (pVarResult) {
    ::VariantInit(pVarResult);
  }

  if (pExcepInfo) {
    *pExcepInfo = {};
  }

  if (puArgErr) {
    *puArgErr = 0;
  }

  switch (dispIdMember) {
    case 201:
      return DispatchDataChange(*pDispParams);
    default:
      return DISP_E_MEMBERNOTFOUND;
  }
}

HRESULT ComDataPointEvents::DispatchDataChange(DISPPARAMS& disp_params) {
  if (disp_params.cArgs != 4) {
    return DISP_E_BADPARAMCOUNT;
  }

  const auto& status = disp_params.rgvarg[3];
  const auto& value = disp_params.rgvarg[2];
  const auto& time = disp_params.rgvarg[1];
  const auto& quality = disp_params.rgvarg[0];

  if (status.vt != VT_UI4 || time.vt != VT_DATE || quality.vt != VT_UI4) {
    // TODO: Set `puArgErr` per
    // https://learn.microsoft.com/en-us/windows/win32/api/oaidl/nf-oaidl-idispatch-invoke.
    return DISP_E_BADVARTYPE;
  }

  if (handlers_.data_changed) {
    handlers_.data_changed(status.ulVal, value, time.date, quality.ulVal);
  }

  return S_OK;
}

}  // namespace vidicon
