#pragma once

#include <boost/signals2/connection.hpp>

#include <functional>
#include <vector>

class NodeService;

namespace scada {
class NodeId;
}

// Calls `reload` whenever the node service reports a change that can alter
// what the Users view shows — an account, a Role, or a role rule. The view
// read its rows once and never again, so an account created, renamed,
// deleted, disabled or given a Role from that very view stayed as it was until
// the view was reopened (found with backlog 149).
//
// `reload` is expected to coalesce: a burst of changes calls it once per
// change.
class UsersGridReloader {
 public:
  UsersGridReloader(NodeService& node_service, std::function<void()> reload);
  ~UsersGridReloader();

  UsersGridReloader(const UsersGridReloader&) = delete;
  UsersGridReloader& operator=(const UsersGridReloader&) = delete;

  // Whether a change to `node_id` can change what the Users view shows.
  static bool Affects(const scada::NodeId& node_id);

 private:
  std::vector<boost::signals2::scoped_connection> connections_;
};
