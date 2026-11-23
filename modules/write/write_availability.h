#pragma once

#include <string_view>

class NodeRef;

// Why a node does not accept the selection-scoped control/write command
// (ID_WRITE). This is the node-shaped half of the command's gates — the other
// half is the session's Write permission, which a node cannot answer for.
// Shared by the command's own handlers and by the Inspector's disabled-control
// explanation, so the rule and the reason the operator reads never drift.
enum class WriteBlock {
  // The node itself accepts control.
  kNone,
  // Not a Variable — a folder, an object, or a computed expression row.
  kNotCommandable,
  // A data item with no output channel configured: nothing to write to.
  kNoOutputChannel,
};

// Classifies `node`. A null node reports kNotCommandable.
WriteBlock GetWriteBlock(const NodeRef& node);

// A short operator-facing explanation of `block`, as an untranslated English
// key for Translate(). Empty for kNone.
std::string_view WriteBlockText(WriteBlock block);

// Whether the Control dialog sends `node`'s command in two stages -- a select,
// then an operate -- rather than as one write. This is the rule `WriteModel`
// applies, shared so the Inspector's hint and the dialog's behaviour never
// drift (backlog 1074).
//
// A data item's "Two-stage control" (`DataItemType_OutputTwoStaged`) decides,
// and an absent property is the nodeset default, Yes. A foreign OPC UA server
// (`scada_server` false, `scada::SessionService::IsScada()`) has no Control
// object to select, so its Variables are always written in one stage.
bool IsTwoStagedControl(const NodeRef& node, bool scada_server);

// What pressing the Inspector's «Control…» will lead to, as an untranslated
// English key for Translate(): whether the command goes in two stages
// (`IsTwoStagedControl`) and whether the operator is asked to confirm it
// (`IsControlConfirmationRequired`: the profile switch, honoured only for an
// administrator).
std::string_view ControlFlowText(bool two_staged, bool confirmation);
