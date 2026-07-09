#pragma once

#include "base/lifetime.h"
#include "common/vds_runtime_api.h"

#include <QLibrary>
#include <QString>

#include <memory>

class VdsRuntimeLoader {
 public:
  VdsRuntimeLoader();

  bool is_loaded() const { return api_ != nullptr; }
  const QString& error_message() const SCADA_LIFETIME_BOUND {
    return error_message_;
  }
  const TcVdsRuntimeApi& api() const SCADA_LIFETIME_BOUND { return *api_; }

 private:
  using GetApiFn = const TcVdsRuntimeApi*(
      TC_VDS_RUNTIME_CALL*)(uint32_t requested_abi_version);

  bool TryLoad(const QString& path);

  std::unique_ptr<QLibrary> library_;
  const TcVdsRuntimeApi* api_ = nullptr;
  QString error_message_;
};
