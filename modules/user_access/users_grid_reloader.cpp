#include "user_access/users_grid_reloader.h"

#include "model/namespaces.h"
#include "node_service/node_service.h"
#include "scada/event.h"
#include "scada/node_id.h"

#include <utility>

UsersGridReloader::UsersGridReloader(NodeService& node_service,
                                     std::function<void()> reload) {
  connections_.push_back(node_service.SubscribeModelChanged(
      [reload](const scada::ModelChangeEvent& event) {
        if (Affects(event.node_id))
          reload();
      }));
  connections_.push_back(node_service.SubscribeNodeSemanticChanged(
      [reload](const scada::NodeId& node_id) {
        if (Affects(node_id))
          reload();
      }));
}

UsersGridReloader::~UsersGridReloader() = default;

bool UsersGridReloader::Affects(const scada::NodeId& node_id) {
  const auto ns = node_id.namespace_index();
  return ns == scada::NamespaceIndexes::USER ||
         ns == scada::NamespaceIndexes::ROLE ||
         ns == scada::NamespaceIndexes::ROLE_IDENTITY;
}
