#pragma once

#include "base/boost_log.h"
#include "common/aliases.h"
#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>
#include <map>
#include <unordered_map>

class NodeRef;
class NodeService;

struct AliasServiceContext {
  const std::shared_ptr<BoostLogger> logger_;
  NodeService& node_service_;
};

class AliasService final : private AliasServiceContext {
 public:
  explicit AliasService(AliasServiceContext&& context);
  ~AliasService();

  void Resolve(std::string_view alias, const AliasResolveCallback& callback);

 private:
  void OnFetchCompleted();

  void OnNodeFetched(const NodeFetchedEvent& event);

  scada::NodeId ResolveNow(const std::string& alias) const;

  NodeRef aliases_;

  bool fetched_ = false;
  std::unordered_map<std::string, std::vector<AliasResolveCallback>>
      pending_aliases_;

  boost::signals2::scoped_connection node_fetched_connection_;
};
