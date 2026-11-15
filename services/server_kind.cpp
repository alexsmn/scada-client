#include "services/server_kind.h"

#include "scada/attribute_service.h"
#include "scada/standard_node_ids.h"

#include <string>
#include <utility>
#include <vector>

Awaitable<scada::model::ServerKind> ReadServerKind(
    std::shared_ptr<scada::AttributeService> attribute_service) {
  if (!attribute_service)
    co_return scada::model::ServerKind::kOurs;

  const scada::DataValue value = co_await scada::Read(
      *attribute_service, scada::ServiceContext{},
      scada::ReadValueId{
          .node_id = scada::NodeId{scada::id::Server_NamespaceArray},
          .attribute_id = scada::AttributeId::Value});
  if (!scada::IsGood(value.status_code))
    co_return scada::model::ServerKind::kOurs;

  // An empty array is not an answer either: a real server's always starts
  // with the OPC UA namespace at index 0. The Client's in-process address
  // spaces (the screenshot generator's, the tests') can hold the node with no
  // value, and they are ours.
  const auto* uris = value.value.get_if<std::vector<scada::String>>();
  if (!uris || uris->empty())
    co_return scada::model::ServerKind::kOurs;

  co_return scada::model::ClassifyServer(*uris);
}

ServerKindSessionService::ServerKindSessionService(
    std::shared_ptr<scada::SessionService> inner)
    : inner_{std::move(inner)} {}

Awaitable<void> ServerKindSessionService::Connect(
    scada::SessionConnectParams params) {
  return inner_->Connect(std::move(params));
}

scada::CoStatus ServerKindSessionService::ConnectStatus(
    scada::SessionConnectParams params) {
  return inner_->ConnectStatus(std::move(params));
}

Awaitable<void> ServerKindSessionService::Reconnect() {
  return inner_->Reconnect();
}

Awaitable<void> ServerKindSessionService::Disconnect() {
  return inner_->Disconnect();
}

bool ServerKindSessionService::IsConnected(scada::Duration* ping_delay) const {
  return inner_->IsConnected(ping_delay);
}

scada::NodeId ServerKindSessionService::GetUserId() const {
  return inner_->GetUserId();
}

std::uint32_t ServerKindSessionService::GetAccessRights() const {
  return inner_->GetAccessRights();
}

bool ServerKindSessionService::IsAnonymous() const {
  return inner_->IsAnonymous();
}

std::string ServerKindSessionService::GetHostName() const {
  return inner_->GetHostName();
}

bool ServerKindSessionService::IsScada() const {
  return kind_ == scada::model::ServerKind::kOurs;
}

boost::signals2::scoped_connection
ServerKindSessionService::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return inner_->SubscribeSessionStateChanged(callback);
}

scada::SessionDebugger* ServerKindSessionService::GetSessionDebugger() {
  return inner_->GetSessionDebugger();
}
