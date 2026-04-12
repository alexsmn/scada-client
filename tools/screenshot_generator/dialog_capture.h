#pragma once

#include <memory>

struct DialogSpec;
class Executor;

// Environment the dialog builders share: an executor to pin
// continuations to, plus whatever services each kind happens to need.
// Owned by `main.cpp`'s test fixture and passed in by reference.
struct DialogEnvironment {
  std::shared_ptr<Executor> executor;
};

// Builds and shows the dialog identified by `spec.kind`, then grabs a
// QPixmap of it and writes to `GetOutputDir() / spec.filename`.
// Returns true on success, false if the kind is unknown or no visible
// dialog was produced. Uses gtest's ADD_FAILURE on misses.
bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env);
