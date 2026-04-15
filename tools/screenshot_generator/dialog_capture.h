#pragma once

#include "scada/node_id.h"

#include <memory>

struct DialogSpec;
class Executor;
class NodeService;
class Profile;
class TimedDataService;

// Environment the dialog builders share: an executor to pin
// continuations to, plus whatever services each kind happens to need.
// Owned by `main.cpp`'s test fixture and passed in by reference.
struct DialogEnvironment {
  std::shared_ptr<Executor> executor;
  // NodeService from the running application — dialogs that operate on
  // a node (limits, write, …) pull NodeRefs from here. Null is fine for
  // kinds that don't need it.
  NodeService* node_service = nullptr;
  // TimedDataService instance the WriteDialog family connects specs to.
  // Null is fine for kinds that don't need it.
  TimedDataService* timed_data_service = nullptr;
  // Profile — WriteModel consults `profile.control_confirmation` on the
  // write path (never taken in capture mode), but still needs a valid
  // reference at construction time.
  Profile* profile = nullptr;
  // Analog item node used by the limits/write dialog screenshots.
  scada::NodeId dialog_analog_node_id;
};

// Builds and shows the dialog identified by `spec.kind`, then grabs a
// QPixmap of it and writes to `GetOutputDir() / spec.filename`.
// Returns true on success, false if the kind is unknown or no visible
// dialog was produced. Uses gtest's ADD_FAILURE on misses.
bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env);
