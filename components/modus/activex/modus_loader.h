#pragma once

#include "base/boost_log.h"
#include "common/aliases.h"
#include "components/modus/activex/modus.h"
#include "filesystem/file_cache.h"

#include <string>

class FileCache;
class FileCacheUpdater;
class TimedDataService;

namespace modus {

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

  using ObjectHandler =
      std::function<void(long object_id, std::unique_ptr<ModusObject> object)>;

  void Load(SDECore::ISDEDocument50& sde_document,
            const std::filesystem::path& path,
            const ObjectHandler& object_handler);

 private:
  void LoadElement(std::unique_ptr<ModusObject>& object,
                   ISDEObject& sde_object,
                   ISDEParams& params,
                   const std::wstring& binding,
                   long object_tag,
                   long tech_index);

  void LoadObject(ISDEObject& sde_object);

  void LoadObjects(ISDEObjects& objects);

  BoostLogger logger_{LOG_NAME("ModusLoader")};

  std::wstring title_;

  ObjectHandler object_handler_;

  std::shared_ptr<FileCacheUpdater> cache_updater_;
};

}  // namespace modus
