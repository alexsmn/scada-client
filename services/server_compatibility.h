#pragma once

#include "aui/translation.h"
#include "scada/status.h"

#include <string>

// What a Server from before version 2.6 answers when the 2.6 Client asks for
// something it does not have.
//
// The 2.6 Client and the 2.5.7 Server speak the same connection protocol
// (20.0), so the Server accepts the session and the Client has no version
// number to go on. What differs is the address space: 2.6 added methods and
// objects -- the per-item `Control` object, Roles, account enable/disable,
// profile methods, frame capture -- that a 2.5.7 Server has never heard of.
// It refuses them before they reach any device or table: its
// `RootNodeManager::Call` answers Bad_WrongNodeId when no service owns the
// node, and a service that owns the node but not the method answers
// Bad_WrongMethodId (read from the 2.5.7 source, scada-server-framework at
// b122fd7ad, `node_manager/router/root_node_manager.cpp`).
//
// So this is a capability test made on first use, not a version check: a
// caller that gets one of these answers for a 2.6-only method falls back to
// the 2.5 way of doing the same thing, or tells the operator the Server is too
// old for it, instead of passing a bare status code on.
//
// Never use it for a status a device returned: Bad_NotSupported, for one, is
// the 2.6 Server refusing on behalf of a device that cannot do something, and
// is deliberately not in this set.
inline bool IsMissingOnServer(const scada::Status& status) {
  return status.code() == scada::StatusCode::Bad_WrongNodeId ||
         status.code() == scada::StatusCode::Bad_WrongMethodId;
}

// What the operator is told when a 2.6-only service is missing on the Server.
// Says what is needed rather than naming a status code, since "Wrong node
// identifier" would send them looking for a misconfigured object.
inline std::u16string ServerTooOldText() {
  return Translate(
      "The Server does not support this. It requires Server version 2.6 or "
      "later.");
}
