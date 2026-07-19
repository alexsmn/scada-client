#include "modules/node_table/node_table_model.h"

#include "address_space/generic_node_factory.h"
#include "address_space/test/scada_test_address_space.h"
#include "aui/dialog_service_mock.h"
#include "base/any_executor.h"
#include "base/async_completion.h"
#include "base/check.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "events/view_events_subscription.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_fetch_status.h"
#include "node_service/v3/node_fetcher.h"
#include "node_service/v3/node_service_impl.h"
#include "properties/property_context.h"
#include "properties/property_service.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/status.h"
#include "services/task_manager_mock.h"

#include <boost/signals2/connection.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace {

using namespace testing;

constexpr scada::NodeId kGroupId{1, NamespaceIndexes::GROUP};

// Controllable v3::NodeFetcher (mirrors the node_property_model harness): each
// FetchNode suspends until the test calls CompleteFetch, so the test drives the
// exact moment a node's data becomes available and thus when the model's
// SetParentNode coroutine resumes.
class ControllableNodeFetcher : public v3::NodeFetcher {
 public:
  ControllableNodeFetcher(AnyExecutor executor,
                          scada::AddressSpace& address_space)
      : executor_{std::move(executor)}, address_space_{address_space} {}

  Awaitable<scada::StatusOr<scada::NodeState>> FetchNode(
      const scada::NodeId& node_id) override {
    fetch_requests.emplace_back(node_id, NodeFetchStatus::NodeOnly);
    co_await GetGate(node_id).Wait();
    const auto* node = address_space_.GetNode(node_id);
    if (!node)
      co_return scada::StatusCode::Bad_WrongNodeId;
    co_return scada::MakeNodeState(*node);
  }

  Awaitable<scada::StatusOr<scada::ReferenceDescriptions>> FetchChildren(
      const scada::NodeId& node_id) override {
    co_return scada::ReferenceDescriptions{};
  }

  void CompleteFetch(const scada::NodeId& node_id) {
    auto& gate = GetGate(node_id);
    if (!gate.completed())
      gate.Complete();
  }

  std::vector<std::pair<scada::NodeId, NodeFetchStatus>> fetch_requests;

 private:
  scada::base::AsyncCompletion& GetGate(const scada::NodeId& node_id) {
    return gates_.try_emplace(node_id, executor_).first->second;
  }

  AnyExecutor executor_;
  scada::AddressSpace& address_space_;
  std::map<scada::NodeId, scada::base::AsyncCompletion> gates_;
};

class NodeTableModelTest : public Test {
 protected:
  NodeTableModelTest() {
    GenericNodeFactory factory{address_space_};
    auto [status, node] = factory.CreateNode(
        scada::NodeState{}
            .set_node_id(kGroupId)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(data_items::id::DataGroupType)
            .set_parent(scada::id::Organizes, data_items::id::DataItems)
            .set_attributes(
                scada::NodeAttributes{}.set_display_name(u"Group")));
    base::Check(status);
    base::Check(node);

    node_service_->OnChannelOpened();
  }

  ViewEventsProvider MakeViewEventsProvider() {
    return [this](scada::ViewEvents& events)
               -> std::unique_ptr<IViewEventsSubscription> {
      view_events_ = &events;
      return std::make_unique<IViewEventsSubscription>();
    };
  }

  std::shared_ptr<NodeTableModel> CreateModel(const scada::NodeId& parent) {
    auto model = std::make_shared<NodeTableModel>(
        executor_, property_service_,
        PropertyContext{executor_, *node_service_, task_manager_,
                        dialog_service_});
    model->SetParentNode(node_service_->GetNode(parent));
    return model;
  }

  // Drives the SetParentNode coroutine to completion by releasing each fetch it
  // requests until no new request appears (the model chases the type chain).
  void CompleteAllFetches() {
    Drain(executor_);
    for (size_t i = 0; i < fetcher_->fetch_requests.size(); ++i) {
      fetcher_->CompleteFetch(fetcher_->fetch_requests[i].first);
      Drain(executor_);
    }
  }

  scada_test::ScadaTestAddressSpace address_space_;
  TestExecutor executor_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  scada::ViewEvents* view_events_ = nullptr;
  std::shared_ptr<ControllableNodeFetcher> fetcher_ =
      std::make_shared<ControllableNodeFetcher>(executor_, address_space_);
  std::shared_ptr<v3::NodeServiceImpl> node_service_ =
      std::make_shared<v3::NodeServiceImpl>(v3::NodeServiceImplContext{
          .executor_ = executor_,
          .monitored_item_service_ = monitored_item_service_,
          .node_fetcher_ = fetcher_,
          .view_events_provider_ = MakeViewEventsProvider(),
          .keep_alive_capacity_ = 1024});
  StrictMock<MockTaskManager> task_manager_;
  StrictMock<MockDialogService> dialog_service_;
  PropertyService property_service_;
};

// Baseline: the async browse resolves and the model fires a change
// notification, proving the harness drives the SetParentNode coroutine.
TEST_F(NodeTableModelTest, NotifiesAfterAsyncBrowseCompletes) {
  auto model = CreateModel(kGroupId);
  int model_changed = 0;
  boost::signals2::scoped_connection conn =
      model->SubscribeModelChanged([&](aui::GridModel&) { ++model_changed; });

  CompleteAllFetches();

  EXPECT_GE(model_changed, 1);
}

// Regression for the on-close SIGSEGV: a fetch completing during teardown
// resumes the SetParentNode coroutine and fires model_changed_signal_. A
// handler — in the app, the grid adapter's endResetModel driving the
// view/controller teardown — drops the model's last external reference *inside*
// that dispatch. The coroutine must keep the model alive so the signal (a member
// of the model) is not freed while boost::signals2::signal::operator() is still
// iterating its slots. Pre-fix this is a use-after-free; with the coroutine
// holding a shared_from_this keep-alive the model outlives the notification and
// is destroyed only when the coroutine frame unwinds.
TEST_F(NodeTableModelTest, SurvivesReentrantRefDropDuringAsyncNotify) {
  auto model = CreateModel(kGroupId);
  std::weak_ptr<NodeTableModel> weak = model;

  bool notified = false;
  bool alive_after_reentrant_drop = false;
  boost::signals2::scoped_connection conn =
      model->SubscribeModelChanged([&](aui::GridModel&) {
        if (notified)
          return;
        notified = true;
        model.reset();  // release the last external ref mid-notification
        // The coroutine's keep-alive must still hold the model here; pre-fix the
        // model (and this very signal) is already freed.
        alive_after_reentrant_drop = !weak.expired();
      });

  CompleteAllFetches();

  ASSERT_TRUE(notified);
  EXPECT_TRUE(alive_after_reentrant_drop);
  // Once the notification returns and the coroutine unwinds, the keep-alive is
  // released and the model is gone — no leak.
  EXPECT_TRUE(weak.expired());
}

}  // namespace
