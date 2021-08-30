#pragma once

#include "base/files/file_util.h"
#include "model/filesystem_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"

#include <filesystem>

inline std::filesystem::path GetFilePath(const NodeRef& file_node) {
  // TODO: Fetch all parents.

  std::wstring path = file_node.display_name();

  for (auto directory_node = file_node;
       IsSubtypeOf(directory_node, filesystem::id::FileDirectoryType);
       directory_node = directory_node.parent()) {
    path.insert(0, directory_node.display_name());
  }

  return std::filesystem::path{std::move(path)};
}
