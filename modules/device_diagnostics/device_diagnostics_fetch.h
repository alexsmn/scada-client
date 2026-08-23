#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

// Makes the device's own diagnostic children and its parent link readable,
// completing once they are.
//
// The counters survive without this — they resolve through the device's
// children, which the address-space fetcher has usually already browsed by the
// time a device can be selected. The **link** half does not: whether the panel
// draws a link section at all is decided by comparing the parent's type browse
// name against the protocol's, so an unfetched parent reads as "not a link",
// and the section, its rows and its Reconnect action are all omitted — on a
// device that has one. Measured 2026-08-23 by rendering the panel without the
// fetch its capture had been doing on its behalf: the counters came out
// identical and «Переподключить» was gone.
Awaitable<void> FetchDeviceDiagnostics(NodeRef device);
