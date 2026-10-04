#pragma once

#include <functional>
#include <string_view>

class ActivityBar;
class CommandHandler;

// The pinned utilities at the foot of the activity rail — Settings and Users —
// and their marker.
//
// A utility is not a third kind of destination: it opens its surface in the
// current page the way any view does, so its marker is independent of the
// mode and page markers. Split out of MainWindow on 2026-10-04 (backlog 722).
class RailUtilitiesController {
 public:
  // Runs a registered command, doing nothing when it does not resolve or is
  // disabled.
  using ExecuteCommand = std::function<void(unsigned command_id)>;
  // The handler the window would run `command_id` through, or null.
  using ResolveCommand = std::function<CommandHandler*(unsigned command_id)>;

  // Adds the utilities to `activity_bar`, which must outlive this object.
  RailUtilitiesController(ActivityBar& activity_bar,
                          ExecuteCommand execute_command,
                          ResolveCommand resolve_command);

  RailUtilitiesController(const RailUtilitiesController&) = delete;
  RailUtilitiesController& operator=(const RailUtilitiesController&) = delete;

  // Re-derives which utilities are offered and which one is marked, given the
  // `WindowInfo::name` of the active view (empty when there is none).
  //
  // Call it on every active-view change and once the command registry is
  // populated: the rail is built before the modules finish registering their
  // views, so a one-shot check at construction would hide the admin-only
  // Users button for everyone.
  void Refresh(std::string_view active_view_type);

 private:
  ActivityBar& activity_bar_;
  const ExecuteCommand execute_command_;
  const ResolveCommand resolve_command_;
};
