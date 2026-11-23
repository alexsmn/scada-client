#pragma once

#include "base/awaitable.h"
#include "scada/session_service.h"

#include <memory>

namespace scada {
class ViewService;
}

// Finds the node of the account `user_name` signed in as, by browsing the
// Users folder (`scada::security::id::Users`) for a child whose DisplayName
// text or BrowseName equals it. Null when the folder cannot be browsed or
// nothing matches -- which is what a foreign OPC UA server answers.
//
// The match is the one the Server makes at login: `FindByDisplayName` over the
// UserType nodes, with `root` -- a static account that has no node in the
// folder -- answered by `scada::security::id::RootUser`
// (scada-server-framework/modules/security/authentication/
// configuration_authenticator.cpp). `root` is answered only when the folder
// itself could be browsed, so a foreign server never gets a SCADA user id.
// The web client resolves the same way (`resolveUserNodeByName` in
// web/packages/ua-react/src/hooks/use-profile-sync.ts).
Awaitable<scada::NodeId> ResolveUserNode(
    std::shared_ptr<scada::ViewService> view_service,
    scada::LocalizedText user_name);

// Forwards every call to the OPC UA session it wraps, and answers
// `GetUserId()` with the node of the account the session signed in as.
//
// **Why it exists (backlog 1110).** A conformant ActivateSessionResponse
// carries no user id (OPC UA Part 4 §5.6.3 ActivateSession,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.6.3), so the
// opcuapp session answers null. Everything that keys on the account read that
// null as "anonymous": the Client neither loaded nor saved the profile on the
// Server, so an operator's pages stayed on one computer whenever the server
// type was «OPC UA», though they followed the account under «Телеконтроль».
//
// The node is resolved once per sign-in, after a successful connect, and kept
// across `Reconnect()` (the same account re-activates). `Disconnect()` drops
// it. A session that names its user itself is never second-guessed.
class UserNodeSessionService final : public scada::SessionService {
 public:
  UserNodeSessionService(std::shared_ptr<scada::SessionService> inner,
                         std::shared_ptr<scada::ViewService> view_service);

  // scada::SessionService
  Awaitable<void> Connect(scada::SessionConnectParams params) override;
  scada::CoStatus ConnectStatus(scada::SessionConnectParams params) override;
  Awaitable<void> Reconnect() override;
  Awaitable<void> Disconnect() override;
  bool IsConnected(scada::Duration* ping_delay = nullptr) const override;
  scada::NodeId GetUserId() const override;
  std::uint32_t GetAccessRights() const override;
  bool IsAnonymous() const override;
  std::string GetHostName() const override;
  bool IsScada() const override;
  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;
  scada::SessionDebugger* GetSessionDebugger() override;

 private:
  Awaitable<void> ResolveAfterConnect(scada::LocalizedText user_name);

  const std::shared_ptr<scada::SessionService> inner_;
  const std::shared_ptr<scada::ViewService> view_service_;
  scada::NodeId user_id_;
};
