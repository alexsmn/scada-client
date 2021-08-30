#pragma once

#include <memory>

class Executor;
class FileRegistry;
class MainWindow;
class NodeRef;

bool ExecuteFileCommand(MainWindow* main_window,
                        const std::shared_ptr<Executor>& executor,
                        const FileRegistry& file_registry,
                        const NodeRef& file_node,
                        unsigned shift);
