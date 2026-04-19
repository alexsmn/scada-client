#pragma once

#include "aui/key_codes.h"
#include "base/awaitable.h"
#include "base/promise.h"
#include "node_service/node_ref.h"

#include <filesystem>
#include <memory>

class DialogService;
class Executor;
class FileManager;
class FileRegistry;
class MainWindowInterface;
class NodeRef;
class TaskManager;

struct OpenFileCommandContext {
  MainWindowInterface* main_window;
  DialogService& dialog_service;
  std::shared_ptr<Executor> executor;
  NodeRef file_node;
  aui::KeyModifiers key_modifiers;
};

using OpenFileCommand =
    std::function<void(const OpenFileCommandContext& context)>;

class OpenFileCommandImpl {
 public:
  promise<void> Execute(const OpenFileCommandContext& context) const;

  const FileRegistry& file_registry;
  FileManager& file_manager;

 private:
  // Coroutine internals: the public `Execute` spawns `ExecuteAsync` on
  // `context.executor` and hands back a `promise<void>` so callers don't
  // change. `OpenFileAsync` branches on file type and dispatches to the
  // right open flow. `OpenJsonFileAsync` loads a `.workplace` JSON window
  // definition from disk and opens it.
  Awaitable<void> ExecuteAsync(OpenFileCommandContext context) const;
  Awaitable<void> OpenFileAsync(OpenFileCommandContext context) const;
};

Awaitable<void> OpenJsonFileAsync(std::filesystem::path path,
                                  MainWindowInterface* main_window,
                                  DialogService& dialog_service,
                                  aui::KeyModifiers key_modifiers,
                                  std::shared_ptr<Executor> executor);

promise<> AddFile(NodeRef parent_directory,
                  DialogService& dialog_service,
                  TaskManager& task_manager,
                  std::shared_ptr<Executor> executor);
