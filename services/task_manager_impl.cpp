#include "services/task_manager_impl.h"

#include "aui/translation.h"
#include "base/awaitable_promise.h"
#include "base/promise.h"
#include "base/u16format.h"
#include "core/progress_host.h"
#include "events/local_events.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "scada/service_context.h"
#include "scada/status_exception.h"
#include "scada/status_or.h"
#include "scada/status_promise.h"

// Windows.h #defines ReportEvent to ReportEventA/W. Undo it.
#ifdef ReportEvent
#undef ReportEvent
#endif

using namespace std::chrono_literals;

namespace {

std::u16string FormatReference(NodeService& node_service,
                               const scada::NodeId& reference_type_id,
                               const scada::NodeId& source_id,
                               const scada::NodeId& target_id,
                               bool add) {
  return u16format(
      L"{} of type {} from {} to {}", add ? L"Adding reference" : L"Deleting reference",
      GetDisplayName(node_service, reference_type_id),
      GetDisplayName(node_service, source_id),
      GetDisplayName(node_service, target_id));
}

scada::StatusOr<std::vector<scada::WriteValue>> PrepareUpdateInputs(
    const NodeRef& node,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  assert(node.fetched());

  std::vector<scada::WriteValue> inputs;
  inputs.reserve(2 + properties.size());

  if (!attributes.browse_name.empty()) {
    inputs.emplace_back(node.node_id(), scada::AttributeId::BrowseName,
                        std::move(attributes.browse_name));
  }

  if (!attributes.display_name.empty()) {
    inputs.emplace_back(node.node_id(), scada::AttributeId::DisplayName,
                        std::move(attributes.display_name));
  }

  for (auto& [prop_decl_id, value] : properties) {
    auto prop_id = node[prop_decl_id].node_id();
    if (prop_id.is_null()) {
      return scada::StatusCode::Bad_WrongPropertyId;
    }

    inputs.emplace_back(std::move(prop_id), scada::AttributeId::Value,
                        std::move(value));
  }

  return inputs;
}

// Maps a write batch result to a Status: first bad code wins, otherwise Good.
scada::Status FirstBadStatus(std::span<const scada::StatusCode> codes) {
  auto it = std::ranges::find_if(codes, &scada::IsBad);
  return it == codes.end() ? scada::Status{scada::StatusCode::Good}
                           : scada::Status{*it};
}

}  // namespace

// TaskManagerImpl

TaskManagerImpl::TaskManagerImpl(TaskManagerImplContext&& context)
    : TaskManagerImplContext{std::move(context)} {
  timer_.StartRepeating(10ms, [this] { Run(); });
}

TaskManagerImpl::~TaskManagerImpl() {
  CancelProgress();
}

void TaskManagerImpl::CancelProgress() {
  // WARNING: Limit callback count.

  running_progress_.reset();
}

promise<void> TaskManagerImpl::PostTask(std::u16string_view description,
                                        const TaskLauncher& launcher) {
  auto self = shared_from_this();
  return PostTaskMethod(
      description,
      [self, launcher]() mutable -> Awaitable<scada::Status> {
        try {
          co_await AwaitPromise(NetExecutorAdapter{self->executor_},
                                launcher());
          co_return scada::Status{scada::StatusCode::Good};
        } catch (const scada::status_exception& e) {
          co_return e.status();
        } catch (...) {
          co_return scada::GetExceptionStatus(std::current_exception());
        }
      });
}

promise<scada::NodeId> TaskManagerImpl::PostInsertTask(
    const scada::NodeState& node_state) {
  promise<scada::NodeId> final_promise;
  auto self = shared_from_this();

  // Cannot use `PostTask` because it only accepts void-returning launchers.
  PostTaskMethod(
      Translate("Insert"),
      [self, node_state,
       final_promise]() mutable -> Awaitable<scada::Status> {
        try {
          NodeRef type_def =
              self->node_service_.GetNode(node_state.type_definition_id);
          co_await AwaitPromise(NetExecutorAdapter{self->executor_},
                                FetchNode(type_def));

          if (type_def.node_class() != scada::NodeClass::ObjectType &&
              type_def.node_class() != scada::NodeClass::VariableType) {
            throw scada::status_exception{scada::StatusCode::Bad_WrongTypeId};
          }

          const scada::NodeClass node_class =
              type_def.node_class() == scada::NodeClass::ObjectType
                  ? scada::NodeClass::Object
                  : scada::NodeClass::Variable;

          auto add_result =
              co_await self->co_node_management_service_.AddNodes(
                  {{.parent_id = node_state.parent_id,
                    .node_class = node_class,
                    .type_definition_id = node_state.type_definition_id,
                    .attributes = node_state.attributes}});
          if (!add_result.ok()) {
            throw scada::status_exception{add_result.status()};
          }
          auto& add_results = *add_result;
          if (add_results.empty()) {
            throw scada::status_exception{scada::StatusCode::Bad};
          }
          if (scada::IsBad(add_results.front().status_code)) {
            throw scada::status_exception{add_results.front().status_code};
          }

          const scada::NodeId added_node_id = add_results.front().added_node_id;

          // References are fire-and-forget — they queue their own tasks.
          for (const auto& reference : node_state.references) {
            if (reference.forward) {
              self->PostAddReference(reference.reference_type_id,
                                     added_node_id, reference.node_id);
            } else {
              self->PostAddReference(reference.reference_type_id,
                                     reference.node_id, added_node_id);
            }
          }

          // Apply the initial property values inline. We cannot recurse into
          // `PostUpdateTask` here: it would queue a second task that cannot
          // start while we are still the running task, deadlocking the queue.
          if (!node_state.properties.empty()) {
            NodeRef node = self->node_service_.GetNode(added_node_id);
            co_await AwaitPromise(NetExecutorAdapter{self->executor_},
                                  FetchNode(node));
            auto inputs = PrepareUpdateInputs(node, /*attributes=*/{},
                                              node_state.properties);
            if (!inputs.ok()) {
              throw scada::status_exception{inputs.status()};
            }
            auto write_result =
                co_await self->co_attribute_service_.Write(
                    scada::ServiceContext{},
                    std::make_shared<const std::vector<scada::WriteValue>>(
                        std::move(*inputs)));
            if (!write_result.ok()) {
              throw scada::status_exception{write_result.status()};
            }
            auto bad = FirstBadStatus(*write_result);
            if (!bad) {
              throw scada::status_exception{std::move(bad)};
            }
          }

          final_promise.resolve(added_node_id);
          co_return scada::Status{scada::StatusCode::Good};
        } catch (const scada::status_exception& e) {
          final_promise.reject(std::current_exception());
          co_return e.status();
        } catch (...) {
          auto status = scada::GetExceptionStatus(std::current_exception());
          final_promise.reject(std::current_exception());
          co_return status;
        }
      });

  return final_promise;
}

promise<void> TaskManagerImpl::PostUpdateTask(
    const scada::NodeId& node_id,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  std::u16string title = GetDisplayName(node_service_, node_id);
  auto self = shared_from_this();
  return PostTaskMethod(
      u16format(L"Modifying {}", title),
      [self, node_id, attributes = std::move(attributes),
       properties = std::move(properties)]() mutable
      -> Awaitable<scada::Status> {
        try {
          NodeRef node = self->node_service_.GetNode(node_id);
          co_await AwaitPromise(NetExecutorAdapter{self->executor_},
                                FetchNode(node));

          auto inputs = PrepareUpdateInputs(node, std::move(attributes),
                                            std::move(properties));
          if (!inputs.ok()) {
            throw scada::status_exception{inputs.status()};
          }

          auto result =
              co_await self->co_attribute_service_.Write(
                  scada::ServiceContext{},
                  std::make_shared<const std::vector<scada::WriteValue>>(
                      std::move(*inputs)));
          if (!result.ok()) {
            throw scada::status_exception{result.status()};
          }
          auto bad = FirstBadStatus(*result);
          if (!bad) {
            throw scada::status_exception{std::move(bad)};
          }
          co_return scada::Status{scada::StatusCode::Good};
        } catch (const scada::status_exception& e) {
          co_return e.status();
        } catch (...) {
          co_return scada::GetExceptionStatus(std::current_exception());
        }
      });
}

promise<void> TaskManagerImpl::PostDeleteTask(const scada::NodeId& node_id) {
  std::u16string title = GetDisplayName(node_service_, node_id);
  auto self = shared_from_this();
  return PostTaskMethod(
      u16format(L"Deleting {}", title),
      [self, node_id]() mutable -> Awaitable<scada::Status> {
        auto result =
            co_await self->co_node_management_service_.DeleteNodes(
                {{.node_id = node_id, .delete_target_references = false}});
        if (!result.ok()) {
          co_return result.status();
        }
        const auto& results = *result;
        if (results.empty()) {
          co_return scada::Status{scada::StatusCode::Bad};
        }
        co_return scada::Status{results.front()};
      });
}

promise<void> TaskManagerImpl::PostAddReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, true);
  auto self = shared_from_this();
  return PostTaskMethod(
      title,
      [self, reference_type_id, source_id,
       target_id]() mutable -> Awaitable<scada::Status> {
        scada::AddReferencesItem input{source_id, reference_type_id, true, {},
                                        target_id};
        auto result =
            co_await self->co_node_management_service_.AddReferences({input});
        if (!result.ok()) {
          co_return result.status();
        }
        const auto& results = *result;
        if (results.empty()) {
          co_return scada::Status{scada::StatusCode::Bad};
        }
        co_return scada::Status{results.front()};
      });
}

promise<void> TaskManagerImpl::PostDeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, false);
  auto self = shared_from_this();
  return PostTaskMethod(
      title,
      [self, reference_type_id, source_id,
       target_id]() mutable -> Awaitable<scada::Status> {
        scada::DeleteReferencesItem input{source_id, reference_type_id, true,
                                          target_id, true};
        auto result =
            co_await self->co_node_management_service_.DeleteReferences(
                {input});
        if (!result.ok()) {
          co_return result.status();
        }
        const auto& results = *result;
        if (results.empty()) {
          co_return scada::Status{scada::StatusCode::Bad};
        }
        co_return scada::Status{results.front()};
      });
}

Awaitable<void> TaskManagerImpl::RunTaskBody(TaskMethod method) {
  scada::Status status{scada::StatusCode::Good};
  try {
    status = co_await method();
  } catch (const scada::status_exception& e) {
    status = e.status();
  } catch (...) {
    status = scada::GetExceptionStatus(std::current_exception());
  }
  ReportRequestCompletion(status, std::u16string{});
  co_return;
}

void TaskManagerImpl::StartTask(Task&& task) {
  assert(!task.IsNull());

  running_task_ = std::move(task);

  if (running_progress_)
    running_progress_->SetStatus((running_task_.title + u"...").c_str());

  // Own `method` here so moving out of `running_task_` inside
  // `ReportRequestCompletion` doesn't dangle the coroutine's captured body.
  auto method = running_task_.method;
  CoSpawn(executor_, [self = shared_from_this(),
                      method = std::move(method)]() mutable {
    return self->RunTaskBody(std::move(method));
  });
}

void TaskManagerImpl::ReportRequestCompletion(
    const scada::Status& status,
    const std::u16string& result_text) {
  auto task = std::move(running_task_);
  running_task_ = Task();

  if (!status || profile_.show_write_ok) {
    std::u16string message =
        u16format(L"{}: {}.", task.title, ToString16(status));
    if (!result_text.empty()) {
      message += u'\n' + result_text;
    }

    auto severity = status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
    local_events_.ReportEvent(severity, message);
  }

  scada::CompleteStatusPromise(task.promise, status);
}

bool TaskManagerImpl::IsRunning() const {
  return !running_task_.IsNull();
}

void TaskManagerImpl::Run() {
  // Handle dialog Cancel button. Allow complete current request.
  if (running_progress_ && running_progress_->IsCanceled()) {
    CancelProgress();

    while (!tasks_.empty()) {
      scada::RejectStatusPromise(tasks_.front().promise,
                                 scada::StatusCode::Bad);
      tasks_.pop();
    }

    running_task_ = Task();
  }

  if (count_ < static_cast<int>(tasks_.size())) {
    count_ = tasks_.size();
  }

  // show or hide dialog
  if (!running_task_.IsNull() || !tasks_.empty()) {
    if (!start_time_.has_value()) {
      start_time_ = std::chrono::steady_clock::now();
    }

    if (!running_progress_ &&
        std::chrono::steady_clock::now() - *start_time_ >= 300ms) {
      running_progress_ = progress_host_.Start();
    }

    if (running_progress_) {
      running_progress_->SetProgress(count_, count_ - tasks_.size());
    }

  } else {
    CancelProgress();
    start_time_.reset();
    count_ = 0;
  }

  // process next task
  if (running_task_.IsNull() && !tasks_.empty()) {
    auto task = std::move(tasks_.front());
    tasks_.pop();
    StartTask(std::move(task));
  }
}

promise<void> TaskManagerImpl::PostTaskMethod(std::u16string_view title,
                                              TaskMethod method) {
  auto promise = tasks_.emplace(std::u16string{title}, std::move(method)).promise;

  Run();

  return promise;
}
