#pragma once

#include "base/any_executor.h"

#include "aui/key_codes.h"
#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <filesystem>
#include <memory>

class DialogService;
class FileManager;
class FileRegistry;
class MainWindowInterface;
class NodeRef;
class TaskManager;

struct OpenFileCommandContext {
  MainWindowInterface* main_window;
  DialogService& dialog_service;
  AnyExecutor executor;
  NodeRef file_node;
  aui::KeyModifiers key_modifiers;
};

using OpenFileCommand =
    std::function<Awaitable<void>(const OpenFileCommandContext& context)>;

class OpenFileCommandImpl {
 public:
  Awaitable<void> Execute(const OpenFileCommandContext& context) const;

  const FileRegistry& file_registry;
  FileManager& file_manager;

 private:
  Awaitable<void> ExecuteAsync(OpenFileCommandContext context) const;
  Awaitable<void> OpenFileAsync(OpenFileCommandContext context) const;
};

Awaitable<void> OpenJsonFileAsync(std::filesystem::path path,
                                  MainWindowInterface* main_window,
                                  DialogService& dialog_service,
                                  aui::KeyModifiers key_modifiers,
                                  AnyExecutor executor);

Awaitable<void> AddFile(NodeRef parent_directory,
                        DialogService& dialog_service,
                        TaskManager& task_manager,
                        AnyExecutor executor);
