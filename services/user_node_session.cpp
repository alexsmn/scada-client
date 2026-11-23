#include "services/user_node_session.h"

#include "base/utf_convert.h"
#include "model/security_node_ids.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service.h"

#include <string>
#include <utility>
#include <vector>

namespace {

bool NamesUser(const scada::ReferenceDescription& reference,
               const std::u16string& user_name) {
  // Text only: the Server's DisplayName carries the locale it was authored in,
  // and the login name the operator typed carries none.
  return reference.display_name.text == user_name ||
         UtfConvert<char16_t>(reference.browse_name.name()) == user_name;
}

}  // namespace

Awaitable<scada::NodeId> ResolveUserNode(
    std::shared_ptr<scada::ViewService> view_service,
    scada::LocalizedText user_name) {
  if (!view_service || user_name.text.empty())
    co_return scada::NodeId{};

  // HierarchicalReferences is stated, not left to the default, for the reason
  // the web client gives beside `HIERARCHICAL_REFERENCES`: a Server from before
  // 2026-09-21 answers an unfiltered Browse Good and empty.
  auto results = co_await view_service->Browse(
      scada::ServiceContext{},
      {scada::BrowseDescription{
          .node_id = scada::security::id::Users,
          .direction = scada::BrowseDirection::Forward,
          .reference_type_id = scada::id::HierarchicalReferences,
          .include_subtypes = true}});
  if (!results.ok() || results->empty() ||
      !scada::IsGood((*results)[0].status_code)) {
    co_return scada::NodeId{};
  }

  for (const scada::ReferenceDescription& reference :
       (*results)[0].references) {
    if (NamesUser(reference, user_name.text))
      co_return reference.node_id;
  }
  // A continuation point means the folder holds more users than one answer
  // carried; `scada::ViewService` has no BrowseNext, so such an account stays
  // unresolved and keeps its profile locally, as before 1110.

  if (user_name.text == u"root")
    co_return scada::security::id::RootUser;
  co_return scada::NodeId{};
}

UserNodeSessionService::UserNodeSessionService(
    std::shared_ptr<scada::SessionService> inner,
    std::shared_ptr<scada::ViewService> view_service)
    : inner_{std::move(inner)}, view_service_{std::move(view_service)} {}

Awaitable<void> UserNodeSessionService::Connect(
    scada::SessionConnectParams params) {
  user_id_ = {};
  scada::LocalizedText user_name = params.user_name;
  co_await inner_->Connect(std::move(params));
  if (inner_->IsConnected())
    co_await ResolveAfterConnect(std::move(user_name));
}

scada::CoStatus UserNodeSessionService::ConnectStatus(
    scada::SessionConnectParams params) {
  user_id_ = {};
  scada::LocalizedText user_name = params.user_name;
  const scada::Status status =
      co_await inner_->ConnectStatus(std::move(params));
  if (scada::IsGood(status.code()))
    co_await ResolveAfterConnect(std::move(user_name));
  co_return status;
}

Awaitable<void> UserNodeSessionService::ResolveAfterConnect(
    scada::LocalizedText user_name) {
  if (!inner_->GetUserId().is_null())
    co_return;
  user_id_ = co_await ResolveUserNode(view_service_, std::move(user_name));
}

Awaitable<void> UserNodeSessionService::Reconnect() {
  return inner_->Reconnect();
}

Awaitable<void> UserNodeSessionService::Disconnect() {
  user_id_ = {};
  return inner_->Disconnect();
}

bool UserNodeSessionService::IsConnected(scada::Duration* ping_delay) const {
  return inner_->IsConnected(ping_delay);
}

scada::NodeId UserNodeSessionService::GetUserId() const {
  scada::NodeId inner_id = inner_->GetUserId();
  return inner_id.is_null() ? user_id_ : inner_id;
}

std::uint32_t UserNodeSessionService::GetAccessRights() const {
  return inner_->GetAccessRights();
}

bool UserNodeSessionService::IsAnonymous() const {
  return inner_->IsAnonymous();
}

std::string UserNodeSessionService::GetHostName() const {
  return inner_->GetHostName();
}

bool UserNodeSessionService::IsScada() const {
  return inner_->IsScada();
}

boost::signals2::scoped_connection
UserNodeSessionService::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return inner_->SubscribeSessionStateChanged(callback);
}

scada::SessionDebugger* UserNodeSessionService::GetSessionDebugger() {
  return inner_->GetSessionDebugger();
}
