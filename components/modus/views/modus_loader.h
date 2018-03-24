#pragma once

#include "base/strings/string16.h"
#include "common/aliases.h"
#include "services/file_cache.h"

class FileCache;
class FileCacheUpdater;
class ModusView;
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

class ModusObject;

struct ModusLoaderContext {
  const AliasResolver alias_resolver_;
  TimedDataService& timed_data_service_;
  FileCache& file_cache_;
};

class ModusLoader : private ModusLoaderContext {
 public:
  explicit ModusLoader(ModusLoaderContext&& context);

  const base::string16& title() const { return title_; }

  void Load(SDECore::ISDEDocument50& document,
            const base::FilePath& path,
            ModusView* view);

 private:
  void AddElement(ModusObject*& object,
                  SDECore::ISDEObject50& sde_object,
                  SDECore::IParams& params,
                  const base::string16& binding,
                  long object_tag);

  void AddObject(SDECore::ISDEObject50& sde_object);

  void LoadObjects(SDECore::ISDEObjects2& objects);

  ModusView* view_;

  base::string16 title_;

  std::shared_ptr<FileCacheUpdater> cache_updater_;
};

}  // namespace modus
