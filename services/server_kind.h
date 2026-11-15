#pragma once

#include "base/awaitable.h"
#include "model/namespace_uris.h"
#include "scada/session_service.h"

#include <memory>

namespace scada {
class AttributeService;
}

// Decides, after sign-in, whether the Client is talking to a Telecontrol SCADA
// server or to some other OPC UA server, from the namespaces the server
// publishes (`scada::model::ClassifyServer`).
//
// This is the "foreign server" mode of
// docs/product/multi-server-and-distribution.md, item 1: a capability decision
// made once from the server's own description of itself, so a SCADA-only
// surface can be withheld up front instead of failing into an unknown node.
// Every OPC UA server must publish the array: NamespaceArray is a Mandatory
// Property of ServerType, OPC UA Part 5 §6.3.1 ServerType,
// https://reference.opcfoundation.org/Core/Part5/v105/docs/6.3.1 (verified
// 2026-11-15).
//
// Every server of ours publishes the URI, which is what makes this safe: in
// 2.6 the SCADA namespace has no `config_group` in
// common/model/nodesets/namespaces.csv, so no tier drops it from the array it
// publishes (scada-server-framework/base/tier_namespaces.h), and a 2.5.7 Server
// — which still needs the 2.5 fallbacks of services/server_compatibility.h —
// publishes it too (its `standard_io_manager.cpp` at b122fd7ad).
//
// A read that fails, or returns anything but a non-empty array of strings,
// decides `kOurs`: that is how the Client behaved before it asked, and a
// session that cannot answer one Read is not one whose surfaces it should
// start hiding.
Awaitable<scada::model::ServerKind> ReadServerKind(
    std::shared_ptr<scada::AttributeService> attribute_service);

// Forwards every call to the session it wraps, and answers
// `scada::SessionService::IsScada()` from the server kind it is told.
//
// The OPC UA session underneath cannot answer `IsScada()` itself: it is the
// opcuapp client stack, which knows no SCADA vocabulary and says false for
// every server. Wrapping it here lets every surface that already holds the
// session service — the controller factory, the command router — ask the one
// question without new plumbing. Until `set_server_kind` is called it says
// `kOurs`, the behaviour before the question existed.
class ServerKindSessionService final : public scada::SessionService {
 public:
  explicit ServerKindSessionService(
      std::shared_ptr<scada::SessionService> inner);

  void set_server_kind(scada::model::ServerKind kind) { kind_ = kind; }
  scada::model::ServerKind server_kind() const { return kind_; }

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
  const std::shared_ptr<scada::SessionService> inner_;
  scada::model::ServerKind kind_ = scada::model::ServerKind::kOurs;
};
