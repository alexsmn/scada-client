#pragma once

#include "base/any_executor.h"

#include "scada/node_id.h"

#include <memory>
#include <string>
#include <vector>

struct DialogSpec;
class NodeService;
class Profile;
class TimedDataService;

// Environment the dialog builders share: an executor to pin
// continuations to, plus whatever services each kind happens to need.
// Owned by `main.cpp`'s test fixture and passed in by reference.
struct DialogEnvironment {
  AnyExecutor executor;
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
  // Analog item node the limits/write dialog screenshots default to. A
  // DialogSpec may name its own node instead ("node" in the fixture), which
  // is how two captures of one kind can show two states of the same dialog.
  scada::NodeId dialog_analog_node_id;
  // Accounts seeded into the login dialog's user combo, first entry
  // pre-selected. Comes from the fixture's `login_user_list`.
  std::vector<std::string> login_user_list;
};

// Builds and shows the dialog identified by `spec.kind`, then grabs a
// QPixmap of it and writes to `OutputPathFor(spec.filename)`.
// Returns true on success, false if the kind is unknown or no visible
// dialog was produced. Uses gtest's ADD_FAILURE on misses.
bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env);
