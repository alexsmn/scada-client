#pragma once

#include "scada/node.h"
#include "scada/status_promise.h"

#include <filesystem>

struct FileManagerContext {
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;
};

class FileManager : private FileManagerContext {
 public:
  explicit FileManager(FileManagerContext&& context)
      : FileManagerContext{std::move(context)} {}

  // Downloads file from server and saves it to public path. May use cached file
  // if it's already downloaded.
  scada::status_promise<void> DownloadFileFromServer(
      const std::filesystem::path& path) const;

 private:
  // `path` is a relative path from public path.
  scada::status_promise<scada::NodeId> GetFileNode(
      const std::filesystem::path& path) const;
};
