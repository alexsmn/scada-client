#pragma once

#include "services/vidicon/teleclient.h"
#include "services/vidicon/vidicon_types.h"

#include <atlbase.h>

#include <array>
#include <atlcom.h>

namespace vidicon {

class ATL_NO_VTABLE ComDataPointConnectionPoints
    : public CComObjectRootEx<CComMultiThreadModelNoCS>,
      public IConnectionPointContainerImpl<ComDataPointConnectionPoints>,
      public IConnectionPointImpl<ComDataPointConnectionPoints,
                                  &__uuidof(_IDataPointEvents)> {
 public:
  void NotifyDataChanged(const DataPointValue& value) {
    if (m_vec.GetSize() == 0) {
      return;
    }

    std::array<VARIANTARG, 3> args;
    args[0] = value.value;
    args[1].vt = VT_DATE;
    args[1].date = value.time;
    args[2].vt = VT_UI4;
    args[2].ulVal = value.quality;

    DISPPARAMS params{.rgvarg = args.data(), .cArgs = std::size(args)};

    for (auto* unk : m_vec) {
      if (CComQIPtr<IDispatch> disp{unk}) {
        CComVariant result;
        EXCEPINFO excep_info = {};
        UINT arg_err = 0;
        disp->Invoke(201, __uuidof(_IDataPointEvents), 0, DISPATCH_METHOD,
                     &params, &result, &excep_info, &arg_err);
      }
    }
  }

  BEGIN_COM_MAP(ComDataPointConnectionPoints)
  COM_INTERFACE_ENTRY(IConnectionPointContainer)
  END_COM_MAP()

  BEGIN_CONNECTION_POINT_MAP(ComDataPointConnectionPoints)
  CONNECTION_POINT_ENTRY(__uuidof(_IDataPointEvents))
  END_CONNECTION_POINT_MAP()
};

}  // namespace vidicon