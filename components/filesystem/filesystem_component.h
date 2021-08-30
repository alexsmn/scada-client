#pragma once

#include <memory>

class ComponentApi;
class FileSynchronizer;

class FileSystemComponent {
 public:
  FileSystemComponent(ComponentApi& api);
~FileSystemComponent();

private:
  std::unique_ptr<FileSynchronizer> file_synchronizer_;
};
