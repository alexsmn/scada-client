#pragma once

#include "base/files/file_path.h"
#include "common/aliases.h"
#include "model/node_id_util.h"
#include "file_cache.h"

#include <memory>
#include <string>

struct FileCacheUpdaterContext {
  const int type_id_;
  const base::FilePath path_;
  const std::wstring title_;
  const AliasResolver alias_resolver_;
  FileCache& cache_;
};

class FileCacheUpdater : private FileCacheUpdaterContext,
                         public std::enable_shared_from_this<FileCacheUpdater> {
 public:
  ~FileCacheUpdater();

  static std::shared_ptr<FileCacheUpdater> Create(
      FileCacheUpdaterContext&& context);

  void Add(base::StringPiece formula, int object_tag);

 private:
  explicit FileCacheUpdater(FileCacheUpdaterContext&& context)
      : FileCacheUpdaterContext{std::move(context)} {}

  BoostLogger logger_{LOG_NAME("FileCacheUpdater")};

  FileCache::ItemMap items_;
};

// static
std::shared_ptr<FileCacheUpdater> FileCacheUpdater::Create(
    FileCacheUpdaterContext&& context) {
  return std::shared_ptr<FileCacheUpdater>(
      new FileCacheUpdater{std::move(context)});
}

inline FileCacheUpdater::~FileCacheUpdater() {
  cache_.Update(type_id_, path_, title_, items_);
}

inline void FileCacheUpdater::Add(base::StringPiece formula, int object_tag) {
  auto self = shared_from_this();
  alias_resolver_(
      formula, [this, self, object_tag](const scada::Status& status,
                                        const scada::NodeId& node_id) {
        if (node_id.is_null())
          return;

        LOG_INFO(logger_) << "Set" << LOG_TAG("Path", path_.value())
                          << LOG_TAG("NodeId", NodeIdToScadaString(node_id))
                          << LOG_TAG("Tag", object_tag);
        items_[node_id] = object_tag;
      });
}
