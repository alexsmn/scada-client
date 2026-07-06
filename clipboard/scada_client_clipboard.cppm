// scada.client.clipboard — named C++20 module facade over the
// client_clipboard headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): headers stay the source of truth, the global
// module fragment includes them, the purview re-exports names with
// `export using`. `export import scada.node_service;` mirrors
// client_clipboard's PUBLIC link on node_service (the other PUBLIC link,
// transport, has no facade — its headers stay textual for consumers;
// scada_remote_protocol_conversions is PRIVATE and not surfaced here).
//
// Notes:
//  - The ::Convert overloads (node_serialization.h) take a forward-declared
//    protocol::Node; only these two overloads are captured below. The
//    protobuf-backed Convert sets in core/remote stay include-only there
//    (see scada.remote), so no cross-module collision arises. The protocol
//    namespace itself is not exported.
//  - Names merely forward-declared by these headers (scada::NodeState,
//    NodeService, TaskManager, CreateTree, ...) are owned by other
//    libraries/facades and are not exported here.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "clipboard/clipboard_util.h"
#include "clipboard/node_serialization.h"

export module scada.client.clipboard;

export import scada.node_service;

export {
  // clipboard_util.h
  using ::CopyNodesToClipboard;
  using ::GetPasteParentNode;
  using ::PasteNodesFromClipboard;
  using ::PasteNodesFromNodeStateRecursive;

  // node_serialization.h
  using ::Convert;
  using ::NodeToData;
}  // export
