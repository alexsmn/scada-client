#include "filesystem/filesystem_component.h"

#include "component_api.h"
#include "controller/controller_registry.h"
#include "filesystem/file_synchronizer.h"
#include "filesystem/filesystem_view.h"

const WindowInfo kWindowInfo = {
    ID_FILE_SYSTEM_VIEW, "FileSystemView", u"Файлы", WIN_SING, 200, 400};

REGISTER_CONTROLLER(FileSystemView, kWindowInfo);

FileSystemComponent::FileSystemComponent(ComponentApi& api) {
  /*std::filesystem::path public_dir;
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
