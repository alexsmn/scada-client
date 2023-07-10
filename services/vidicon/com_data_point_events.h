#pragma once

#include "services/vidicon/teleclient.h"
#include "services/vidicon/vidicon_types.h"

#include <atlbase.h>

#include <algorithm>
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

    // The arguments are in the inverse order.
    std::array<VARIANTARG, 4> args;
    args[3].vt = VT_UI4;
    args[3].ulVal = value.status;
    // No copy for performance sake.
    args[2] = value.value;
    args[1].vt = VT_DATE;
    args[1].date = value.time;
    args[0].vt = VT_UI4;
    args[0].ulVal = value.quality;

    DISPPARAMS params{.rgvarg = args.data(), .cArgs = std::size(args)};

    // The approach is copied from MSDN:
    // https://learn.microsoft.com/en-us/cpp/atl/adding-an-event-atl-tutorial-part-5?view=msvc-170
    int connection_count = m_vec.GetSize();
    for (int i = 0; i < connection_count; ++i) {
      Lock();
      CComPtr<IUnknown> sp = m_vec.GetAt(i);
      Unlock();

      if (IDispatch* disp = reinterpret_cast<IDispatch*>(sp.p)) {
        disp->Invoke(201, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
                     &params, nullptr, nullptr, nullptr);
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