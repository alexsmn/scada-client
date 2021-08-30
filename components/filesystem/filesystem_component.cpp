#include "components/filesystem/filesystem_component.h"

#include "component_api.h"
#include "components/filesystem/file_synchronizer.h"
#include "components/filesystem/filesystem_view.h"
#include "controller_registry.h"

const WindowInfo kWindowInfo = {
    ID_FILE_SYSTEM_VIEW, "FileSystemView", L"Файлы", WIN_SING, 200, 400};

REGISTER_CONTROLLER(FileSystemView, kWindowInfo);

FileSystemComponent::FileSystemComponent(ComponentApi& api) {
  /*base::FilePath public_dir;
  if (base::PathService::Get(client::DIR_PUBLIC, &public_dir)) {
    file_synchronizer_ =
        std::make_unique<FileSynchronizer>(FileSynchronizerContext{
            std::make_shared<NestedLogger>(logger_, "FileSynchronizer"),
            *node_service_,
            public_dir.value(),
        });
  }*/
}

FileSystemComponent::~FileSystemComponent() {}
