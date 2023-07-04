#pragma once

#include "services/vidicon/vidicon_types.h"

#include <atlbase.h>

#include <array>
#include <atlcom.h>

namespace vidicon {

class ATL_NO_VTABLE ComDataPointConnectionPoints
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IConnectionPointContainerImpl<ComDataPointConnectionPoints>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(_IDataPointEvents)>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(_IDataPointEvents3)>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(_IDataPointEventsEx)> {
 public:
  void NotifyDataChanged(const DataPointValue& value) {
    NotifyDataChanged(__uuidof(_IDataPointEvents),
                      IConnectionPointImpl<ComDataPointConnectionPoints,
                                           &__uuidof(_IDataPointEvents)>::m_vec,
                      value);

    NotifyDataChanged(
        __uuidof(_IDataPointEvents3),
        IConnectionPointImpl<ComDataPointConnectionPoints,
                             &__uuidof(_IDataPointEvents3)>::m_vec,
        value);

    NotifyDataChanged(
        __uuidof(_IDataPointEventsEx),
        IConnectionPointImpl<ComDataPointConnectionPoints,
                             &__uuidof(_IDataPointEventsEx)>::m_vec,
        value);
  }

  void NotifyDataChanged(const IID& iid,
                         CComDynamicUnkArray& vec,
                         const DataPointValue& value) {
    if (vec.GetSize() == 0) {
      return;
    }

    std::array<VARIANTARG, 3> args;
    args[0] = value.value;
    args[1].vt = VT_DATE;
    args[1].date = value.time;
    args[2].vt = VT_UI4;
    args[2].ulVal = value.quality;

    DISPPARAMS params{.rgvarg = args.data(), .cArgs = std::size(args)};

    for (auto* unk : vec) {
      if (CComQIPtr<IDispatch> disp{unk}) {
        CComVariant result;
        EXCEPINFO excep_info = {};
        UINT arg_err = 0;
        disp->Invoke(201, iid, 0, DISPATCH_METHOD, &params, &result,
                     &excep_info, &arg_err);
      }
    }
  }

  BEGIN_COM_MAP(ComDataPointConnectionPoints)
  COM_INTERFACE_ENTRY(IConnectionPointContainer)
  END_COM_MAP()

  BEGIN_CONNECTION_POINT_MAP(ComDataPointConnectionPoints)
  CONNECTION_POINT_ENTRY(__uuidof(_IDataPointEvents))
  CONNECTION_POINT_ENTRY(__uuidof(_IDataPointEventsEx))
  CONNECTION_POINT_ENTRY(__uuidof(_IDataPointEvents3))
  END_CONNECTION_POINT_MAP()
};

}  // namespace vidicon