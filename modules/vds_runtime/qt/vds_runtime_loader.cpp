#include "vds_runtime/qt/vds_runtime_loader.h"

#include "base/client_paths.h"
#include "base/path_service.h"

#include <QDir>

#include <filesystem>

namespace {

QString ToQString(const std::filesystem::path& path) {
  return QString::fromStdWString(path.wstring());
}

QString RuntimeFileName() {
#ifdef _WIN32
  return QStringLiteral("tc_vds_runtime.dll");
#elif defined(__APPLE__)
  return QStringLiteral("libtc_vds_runtime.dylib");
#else
  return QStringLiteral("libtc_vds_runtime.so");
#endif
}

}  // namespace

VdsRuntimeLoader::VdsRuntimeLoader() {
  std::filesystem::path install_dir;
  if (!base::PathService::Get(client::DIR_INSTALL, &install_dir) ||
      install_dir.empty()) {
    error_message_ = QStringLiteral("Cannot resolve client install directory.");
    return;
  }

  const auto install = ToQString(install_dir);
  const QStringList candidates = {
      QDir{install}.filePath(RuntimeFileName()),
      QDir{QDir{install}.filePath(QStringLiteral("vds"))}.filePath(
          RuntimeFileName()),
  };

  for (const auto& candidate : candidates) {
    if (TryLoad(candidate))
      return;
  }

  if (error_message_.isEmpty()) {
    error_message_ = QStringLiteral("Cannot find tc_vds_runtime in the "
                                    "client install directory.");
  }
}

bool VdsRuntimeLoader::TryLoad(const QString& path) {
  auto library = std::make_unique<QLibrary>(path);
  if (!library->load()) {
    error_message_ = library->errorString();
    return false;
  }

  auto* get_api = reinterpret_cast<GetApiFn>(
      library->resolve("TcVdsRuntimeGetApi"));
  if (!get_api) {
    error_message_ = QStringLiteral("tc_vds_runtime does not export "
                                    "TcVdsRuntimeGetApi.");
    return false;
  }

  api_ = get_api(TC_VDS_RUNTIME_ABI_VERSION);
  if (!api_ || api_->struct_size < sizeof(TcVdsRuntimeApi)) {
    error_message_ = QStringLiteral("tc_vds_runtime has an incompatible ABI.");
    api_ = nullptr;
    return false;
  }

  library_ = std::move(library);
  error_message_.clear();
  return true;
}
