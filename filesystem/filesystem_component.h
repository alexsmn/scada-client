#pragma once

#include <memory>

class FileSynchronizer;

// TODO: Rename to `FilesystemModule`.
class FileSystemComponent {
 public:
  FileSystemComponent();
  ~FileSystemComponent();

 private:
  std::unique_ptr<FileSynchronizer> file_synchronizer_;
};
