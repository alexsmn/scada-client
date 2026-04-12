#pragma once

#include <memory>

struct DialogSpec;
class Executor;
class LocalNodeService;

// Environment the dialog builders share: an executor to pin
// continuations to, plus whatever services each kind happens to need.
// Owned by `main.cpp`'s test fixture and passed in by reference.
struct DialogEnvironment {
  std::shared_ptr<Executor> executor;
  // LocalNodeService from the fixture — dialogs that operate on a node
  // (limits, write, …) pull NodeRefs from here. Null is fine for kinds
  // that don't need it.
  LocalNodeService* node_service = nullptr;
};

// Builds and shows the dialog identified by `spec.kind`, then grabs a
// QPixmap of it and writes to `GetOutputDir() / spec.filename`.
// Returns true on success, false if the kind is unknown or no visible
// dialog was produced. Uses gtest's ADD_FAILURE on misses.
bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env);
