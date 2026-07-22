#pragma once

#include <string_view>

class NodeRef;

// Why a node does not accept the selection-scoped control/write command
// (ID_WRITE). This is the node-shaped half of the command's gates — the other
// half is the session's Control privilege, which a node cannot answer for.
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
