#pragma once

#include <atlcomcli.h>

namespace vidicon {

struct DataPointValue {
  HRESULT status = S_OK;
  CComVariant value;
  DATE time = 0;
  unsigned quality = 0;
};

}  // namespace vidicon