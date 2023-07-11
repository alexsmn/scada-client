#pragma once

#include <atlcomcli.h>
#include <boost/container_hash/hash.hpp>
#include <string>

namespace vidicon {

struct DataPointAddress {
  std::wstring opc_address;
  unsigned vidicon_id = 0;

  bool operator==(const DataPointAddress& other) const = default;
};

struct DataPointValue {
  HRESULT status = S_OK;
  CComVariant value;
  DATE time = 0;
  unsigned quality = 0;
};

}  // namespace vidicon

namespace std {

template <>
struct hash<vidicon::DataPointAddress> {
  std::size_t operator()(
      const vidicon::DataPointAddress& address) const noexcept {
    std::size_t seed = 0;
    boost::hash_combine(seed, address.opc_address);
    boost::hash_combine(seed, address.vidicon_id);
    return seed;
  }
};

}  // namespace std
