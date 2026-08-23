#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

// Makes everything a transmission rule renders from readable, completing once
// it is.
//
// Kept apart from `transmission_rule.h`, whose helpers are deliberately
// node-service-free: this one is nothing but node traffic.
//
// A rule reads in two hops, and a selection makes neither of them resident —
// selecting a node connects live data and fetches the node alone. The first hop
// is the rule's own property children (`Address`, and `SourceNode`, which holds
// a NodeId rather than a reference), which need the item's children resident,
// its type chain resident for the subscript to resolve the declaration against,
// and each property's own value. The second hop is the node that NodeId names —
// a peer elsewhere in the address space, not a child — plus the parent
// endpoint. Without all of it the inspector renders a rule with no source name,
// no signal tag, no endpoint and IOA 0: a configured rule that looks
// unconfigured.
Awaitable<void> FetchTransmissionRule(NodeRef transmission);
