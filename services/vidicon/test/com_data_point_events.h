#pragma once

#include "vidicon/teleclient.h"

#include <atlbase.h>

#include <atlcom.h>
#include <functional>
#include <wrl/client.h>

namespace vidicon {

struct ComDataPointEventHanders {
  std::function<void(const VARIANT& value, DATE time, UINT quality)>
      data_changed;
};

class ATL_NO_VTABLE ComDataPointEvents
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IDispatchImpl<_IDataPointEvents3,
                           &__uuidof(_IDataPointEvents3),
                           &LIBID_TeleClientLib> {
 public:
  BEGIN_COM_MAP(ComDataPointEvents)
  COM_INTERFACE_ENTRY(_IDataPointEvents3)
  COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  void Init(const ComDataPointEventHanders& handlers);

  void DispatchDataChange(DISPPARAMS& disp_params);

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

inline Microsoft::WRL::ComPtr<_IDataPointEvents3> CreateComDataPointEvents(
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
  if (pDispParams == nullptr || pVarResult == nullptr || puArgErr == 0) {
    return E_POINTER;
  }

  if (dispIdMember == 201) {
    DispatchDataChange(*pDispParams);
  }

  ::VariantInit(pVarResult);
  *pExcepInfo = {};
  *puArgErr = 0;
  return S_OK;
}

void ComDataPointEvents::DispatchDataChange(DISPPARAMS& disp_params) {
  if (disp_params.cArgs != 3) {
    return;
  }

  const auto& value = disp_params.rgvarg[0];
  const auto& time = disp_params.rgvarg[1];
  const auto& quality = disp_params.rgvarg[2];

  if (time.vt != VT_DATE || quality.vt != VT_UI4) {
    return;
  }

  if (handlers_.data_changed) {
    handlers_.data_changed(value, time.date, quality.ulVal);
  }
}

}  // namespace vidicon
