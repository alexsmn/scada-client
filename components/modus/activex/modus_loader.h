#pragma once

#include "base/boost_log.h"
#include "common/aliases.h"
#include "services/file_cache.h"

#include <string>

class FileCache;
class FileCacheUpdater;
class TimedDataService;

namespace base {
class FilePath;
}

namespace SDECore {
struct IParams;
struct ISDEDocument50;
struct ISDEObject50;
struct ISDEObjects2;
}  // namespace SDECore

namespace modus {

class ModusDocument;
class ModusObject;

struct ModusLoaderContext {
  const AliasResolver alias_resolver_;
  TimedDataService& timed_data_service_;
  FileCache& file_cache_;
};

class ModusLoader : private ModusLoaderContext {
 public:
  explicit ModusLoader(ModusLoaderContext&& context);

  const std::wstring& title() const { return title_; }

  void Load(SDECore::ISDEDocument50& sde_document,
            const base::FilePath& path,
            ModusDocument* document);

 private:
  void LoadElement(std::unique_ptr<ModusObject>& object,
                   SDECore::ISDEObject50& sde_object,
                   SDECore::IParams& params,
                   const std::wstring& binding,
                   long object_tag,
                   long tech_index);

  void LoadObject(SDECore::ISDEObject50& sde_object);

  void LoadObjects(SDECore::ISDEObjects2& objects);

  BoostLogger logger_{LOG_NAME("ModusLoader")};

  ModusDocument* document_ = nullptr;

  std::wstring title_;

  std::shared_ptr<FileCacheUpdater> cache_updater_;
};

}  // namespace modus
