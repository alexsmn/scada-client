#pragma once

// Command ids shared by the client's controllers, command registries, menu
// models and action definitions, plus the image ids of the toolbar icons.
//
// This started life as a Visual C++ generated resource header for the client's
// Win32 resource script; the script is gone (see app/client_icon.rc), and with
// it the resource editor that handed out numbers. The ids are now constants
// generated from common_resource_ids.inc, which is also what the uniqueness
// check below reads, so a new id cannot be added without being checked.

#include <cstddef>
#include <iterator>

#define CLIENT_COMMAND_ID(name, value) inline constexpr int name = value;
#define CLIENT_IMAGE_ID(name, value) inline constexpr int name = value;
#include "resources/common_resource_ids.inc"
#undef CLIENT_IMAGE_ID
#undef CLIENT_COMMAND_ID

namespace client_resources_internal {

inline constexpr int kCommandIds[] = {
#define CLIENT_COMMAND_ID(name, value) value,
#define CLIENT_IMAGE_ID(name, value)
#include "resources/common_resource_ids.inc"
#undef CLIENT_IMAGE_ID
#undef CLIENT_COMMAND_ID
};

// Id ranges no static command id may fall in: the first is minted from by
// CreateUniqueCommandId() (controller/command_registry.h) for commands
// registered without an id, the second is ID_NEW's per-node-type range.
struct IdRange {
  int first;
  int last;
};
inline constexpr IdRange kReservedRanges[] = {{10000, 19999}, {40001, 40099}};

// Returns a command id that appears twice in kCommandIds, or 0 when all are
// distinct. Returning the value rather than a bool puts it in the compiler's
// diagnostic when the static_assert below fails.
constexpr int FindDuplicateCommandId() {
  constexpr std::size_t kCount = std::size(kCommandIds);
  for (std::size_t i = 0; i < kCount; ++i) {
    for (std::size_t j = i + 1; j < kCount; ++j) {
      if (kCommandIds[i] == kCommandIds[j]) {
        return kCommandIds[i];
      }
    }
  }
  return 0;
}

// Returns a command id inside one of kReservedRanges, or 0 when there is none.
constexpr int FindReservedCommandId() {
  for (const int id : kCommandIds) {
    for (const IdRange& range : kReservedRanges) {
      if (id >= range.first && id <= range.last) {
        return id;
      }
    }
  }
  return 0;
}

// Window ids and command ids are resolved in one namespace
// (MainWindowCommandRouter), so two sharing a value can dispatch as each
// other. ID_ADMINISTRATION_VIEW and ID_NEW_IEC60870_LINK101 were both 135, and
// ID_WEB_VIEW and ID_MOVE_DOWN both 128, until this check existed.
static_assert(
    FindDuplicateCommandId() == 0,
    "two CLIENT_COMMAND_IDs in common_resource_ids.inc share a value");
static_assert(FindReservedCommandId() == 0,
              "a CLIENT_COMMAND_ID falls in a reserved id range");

}  // namespace client_resources_internal
