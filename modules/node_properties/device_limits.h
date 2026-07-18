#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "parameter_form/limit_row.h"

#include <vector>

class NodeRef;

// Browses `device`'s analog data-item children and builds its limits rows — the
// signal name and its LoLo / Lo / Hi / HiHi bands (empty when a band is unset) —
// for config-workbench's Limits tab. Fetches what it reads, so it is safe on a
// freshly-opened device. Returns an empty list for a device with no analog
// items.
Awaitable<std::vector<LimitRow>> BuildDeviceLimits(AnyExecutor executor,
                                                   NodeRef device);
