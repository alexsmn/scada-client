#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "scada/node_id.h"

class DialogService;
class LocalEvents;
class Profile;
class TimedDataService;

struct WriteContext {
  const AnyExecutor executor_;
  TimedDataService& timed_data_service_;
  const scada::NodeId node_id_;
  Profile& profile_;
  const bool manual_ = false;
  // Where a successful command is reported when the profile's
  // `show_write_ok` asks for it. Optional: without it success stays silent.
  LocalEvents* const local_events_ = nullptr;
  // False when the session is connected to an OPC UA server that is not ours
  // (`scada::SessionService::IsScada()`). Such a server has no per-item
  // Control object, so the select stage of two-stage control is skipped and
  // the value is written directly — the only command any OPC UA server
  // understands (docs/product/multi-server-and-distribution.md, "foreign
  // server" item 1).
  const bool scada_server_ = true;
  // Whether the signed-in account may switch control confirmation off
  // (`CanSwitchOffControlConfirmation`). False — the safe default — makes
  // every command go through the review whatever the profile says.
  const bool may_skip_confirmation_ = false;
  // The signed-in account's user node, which the review names as the operator
  // the command is logged against. Null for an anonymous session.
  const scada::NodeId user_id_;
};

// The context is taken **by value**: this returns a lazy awaitable, and a
// coroutine does not copy reference parameters into its frame, so an rvalue
// reference here would dangle the moment the callee became a coroutine.
Awaitable<void> ExecuteWriteDialog(DialogService& dialog_service,
                                   WriteContext context);
