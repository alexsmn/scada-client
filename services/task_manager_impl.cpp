#include "services/task_manager_impl.h"

#include "aui/translation.h"
#include "base/check.h"
#include "base/u16format.h"
#include "core/progress_host.h"
#include "events/local_events.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "scada/co_result.h"
#include "scada/service_context.h"
#include "scada/status_or.h"

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
  return u16format(L"{} of type {} from {} to {}",
                   add ? L"Adding reference" : L"Deleting reference",
                   GetDisplayName(node_service, reference_type_id).text,
                   GetDisplayName(node_service, source_id).text,
                   GetDisplayName(node_service, target_id).text);
}

scada::StatusOr<std::vector<scada::WriteValue>> PrepareUpdateInputs(
    const NodeRef& node,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  scada::base::Check(node.fetched());

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

void CompleteTaskCompletion(scada::base::AsyncCompletion& completion,
                            const scada::Status& status) {
  completion.Complete();
}

scada::CoStatus RunTaskLauncher(AnyExecutor executor,
                                TaskManager::TaskLauncher launcher) {
  co_return co_await launcher();
}

template <class T>
struct TaskResultState {
  std::optional<T> value;
  scada::Status status{scada::StatusCode::Good};
};

template <class T>
scada::CoStatus RunTypedTaskMethod(std::function<scada::CoStatusOr<T>()> method,
                                   std::shared_ptr<TaskResultState<T>> result) {
  auto value = co_await method();
  if (!value.ok()) {
    result->status = value.status();
    co_return result->status;
  }
  result->value.emplace(std::move(*value));
  result->status = scada::StatusCode::Good;
  co_return result->status;
}

template <class T>
scada::CoStatusOr<T> WaitTypedTaskResult(
    std::shared_ptr<TaskResultState<T>> result,
    Awaitable<void> waiter) {
  co_await std::move(waiter);
  if (!result->status) {
    co_return result->status;
  }
  co_return std::move(*result->value);
}

scada::CoStatus WaitTaskResult(
    std::shared_ptr<TaskResultState<scada::Status>> result,
    Awaitable<void> waiter) {
  co_await std::move(waiter);
  co_return result->status;
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

scada::CoStatus TaskManagerImpl::PostTask(std::u16string_view description,
                                          const TaskLauncher& launcher) {
  auto self = shared_from_this();
  return PostTaskMethod(std::u16string{description},
                        [self, launcher]() mutable -> scada::CoStatus {
                          return RunTaskLauncher(self->executor_,
                                                 std::move(launcher));
                        });
}

scada::CoStatusOr<scada::NodeId> TaskManagerImpl::PostInsertTask(
    const scada::NodeState& node_state) {
  auto self = shared_from_this();

  return PostTypedTaskMethod<scada::NodeId>(
      Translate("Insert"), [self, node_state]() mutable {
        return RunInsertTask(std::move(self), std::move(node_state));
      });
}

scada::CoStatusOr<scada::NodeId> TaskManagerImpl::RunInsertTask(
    std::shared_ptr<TaskManagerImpl> self,
    scada::NodeState node_state) {
  NodeRef type_def = self->node_service_.GetNode(node_state.type_definition_id);
  auto fetch_status = co_await FetchNodeStatus(type_def);
  if (!fetch_status) {
    co_return fetch_status;
  }

  if (type_def.node_class() != scada::NodeClass::ObjectType &&
      type_def.node_class() != scada::NodeClass::VariableType) {
    co_return scada::StatusCode::Bad_WrongTypeId;
  }

  const scada::NodeClass node_class =
      type_def.node_class() == scada::NodeClass::ObjectType
          ? scada::NodeClass::Object
          : scada::NodeClass::Variable;

  auto add_result = co_await self->node_management_service_.AddNodes(
      scada::ServiceContext{},
      {{.parent_id = node_state.parent_id,
        .node_class = node_class,
        .type_definition_id = node_state.type_definition_id,
        .attributes = node_state.attributes}});
  if (!add_result.ok()) {
    co_return add_result.status();
  }
  auto& add_results = *add_result;
  if (add_results.empty()) {
    co_return scada::StatusCode::Bad;
  }
  if (scada::IsBad(add_results.front().status_code)) {
    co_return add_results.front().status_code;
  }

  const scada::NodeId added_node_id = add_results.front().added_node_id;

  // Add the references through the service directly, as part of this task.
  // `PostAddReference` cannot be used here: it returns a lazy awaitable that
  // enqueues a task only once awaited, and awaiting a queued task from inside
  // this coroutine would deadlock — this coroutine itself runs as the queue's
  // current task, and tasks are serialized.
  if (!node_state.references.empty()) {
    std::vector<scada::AddReferencesItem> reference_inputs;
    reference_inputs.reserve(node_state.references.size());
    for (const auto& reference : node_state.references) {
      if (reference.forward) {
        reference_inputs.push_back(
            {.source_node_id = added_node_id,
             .reference_type_id = reference.reference_type_id,
             .target_node_id = reference.node_id});
      } else {
        reference_inputs.push_back(
            {.source_node_id = reference.node_id,
             .reference_type_id = reference.reference_type_id,
             .target_node_id = added_node_id});
      }
    }

    auto add_references_result =
        co_await self->node_management_service_.AddReferences(
            scada::ServiceContext{}, std::move(reference_inputs));
    if (!add_references_result.ok()) {
      co_return add_references_result.status();
    }
    auto bad_reference = FirstBadStatus(*add_references_result);
    if (!bad_reference) {
      co_return bad_reference;
    }
  }

  if (!node_state.properties.empty()) {
    NodeRef node = self->node_service_.GetNode(added_node_id);
    fetch_status = co_await FetchNodeStatus(node);
    if (!fetch_status) {
      co_return fetch_status;
    }
    auto inputs =
        PrepareUpdateInputs(node, /*attributes=*/{}, node_state.properties);
    if (!inputs.ok()) {
      co_return inputs.status();
    }
    auto write_result = co_await self->attribute_service_.Write(
        scada::ServiceContext{}, std::move(*inputs));
    if (!write_result.ok()) {
      co_return write_result.status();
    }
    auto bad = FirstBadStatus(*write_result);
    if (!bad) {
      co_return bad;
    }
  }

  co_return added_node_id;
}

scada::CoStatus TaskManagerImpl::PostUpdateTask(
    const scada::NodeId& node_id,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  std::u16string title = GetDisplayName(node_service_, node_id).text;
  auto self = shared_from_this();
  return PostTaskMethod(u16format(L"Modifying {}", title),
                        [self, node_id, attributes = std::move(attributes),
                         properties = std::move(properties)]() mutable {
                          return RunUpdateTask(std::move(self), node_id,
                                               std::move(attributes),
                                               std::move(properties));
                        });
}

scada::CoStatus TaskManagerImpl::RunUpdateTask(
    std::shared_ptr<TaskManagerImpl> self,
    scada::NodeId node_id,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  NodeRef node = self->node_service_.GetNode(node_id);
  auto fetch_status = co_await FetchNodeStatus(node);
  if (!fetch_status) {
    co_return fetch_status;
  }

  auto inputs =
      PrepareUpdateInputs(node, std::move(attributes), std::move(properties));
  if (!inputs.ok()) {
    co_return inputs.status();
  }

  auto result = co_await self->attribute_service_.Write(scada::ServiceContext{},
                                                        std::move(*inputs));
  if (!result.ok()) {
    co_return result.status();
  }
  auto bad = FirstBadStatus(*result);
  if (!bad) {
    co_return bad;
  }
  co_return scada::Status{scada::StatusCode::Good};
}

scada::CoStatus TaskManagerImpl::PostDeleteTask(const scada::NodeId& node_id) {
  std::u16string title = GetDisplayName(node_service_, node_id).text;
  auto self = shared_from_this();
  return PostTaskMethod(u16format(L"Deleting {}", title),
                        [self, node_id]() mutable {
                          return RunDeleteTask(std::move(self), node_id);
                        });
}

scada::CoStatus TaskManagerImpl::RunDeleteTask(
    std::shared_ptr<TaskManagerImpl> self,
    scada::NodeId node_id) {
  auto result = co_await self->node_management_service_.DeleteNodes(
      scada::ServiceContext{},
      {{.node_id = node_id, .delete_target_references = false}});
  if (!result.ok()) {
    co_return result.status();
  }
  const auto& results = *result;
  if (results.empty()) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  co_return scada::Status{results.front()};
}

scada::CoStatus TaskManagerImpl::PostAddReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, true);
  auto self = shared_from_this();
  return PostTaskMethod(std::move(title), [self, reference_type_id, source_id,
                                           target_id]() mutable {
    return RunAddReferenceTask(std::move(self), reference_type_id, source_id,
                               target_id);
  });
}

scada::CoStatus TaskManagerImpl::RunAddReferenceTask(
    std::shared_ptr<TaskManagerImpl> self,
    scada::NodeId reference_type_id,
    scada::NodeId source_id,
    scada::NodeId target_id) {
  scada::AddReferencesItem input{
      source_id, reference_type_id, true, {}, target_id};
  auto result = co_await self->node_management_service_.AddReferences(
      scada::ServiceContext{}, {input});
  if (!result.ok()) {
    co_return result.status();
  }
  const auto& results = *result;
  if (results.empty()) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  co_return scada::Status{results.front()};
}

scada::CoStatus TaskManagerImpl::PostDeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, false);
  auto self = shared_from_this();
  return PostTaskMethod(std::move(title), [self, reference_type_id, source_id,
                                           target_id]() mutable {
    return RunDeleteReferenceTask(std::move(self), reference_type_id, source_id,
                                  target_id);
  });
}

scada::CoStatus TaskManagerImpl::RunDeleteReferenceTask(
    std::shared_ptr<TaskManagerImpl> self,
    scada::NodeId reference_type_id,
    scada::NodeId source_id,
    scada::NodeId target_id) {
  scada::DeleteReferencesItem input{source_id, reference_type_id, true,
                                    target_id, true};
  auto result = co_await self->node_management_service_.DeleteReferences(
      scada::ServiceContext{}, {input});
  if (!result.ok()) {
    co_return result.status();
  }
  const auto& results = *result;
  if (results.empty()) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  co_return scada::Status{results.front()};
}

Awaitable<void> TaskManagerImpl::RunTaskBody(TaskMethod method) {
  auto status = co_await method();
  ReportRequestCompletion(status, std::u16string{});
  co_return;
}

void TaskManagerImpl::StartTask(Task&& task) {
  scada::base::Check(!task.IsNull());

  running_task_ = std::move(task);

  if (running_progress_)
    running_progress_->SetStatus((running_task_.title + u"...").c_str());

  // Own `method` here so moving out of `running_task_` inside
  // `ReportRequestCompletion` doesn't dangle the coroutine's captured body.
  auto method = running_task_.method;
  CoSpawn(executor_,
          [self = shared_from_this(), method = std::move(method)]() mutable {
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

  if (task.completion) {
    CompleteTaskCompletion(*task.completion, status);
  }
}

bool TaskManagerImpl::IsRunning() const {
  return !running_task_.IsNull();
}

void TaskManagerImpl::Run() {
  // Handle dialog Cancel button. Allow complete current request.
  if (running_progress_ && running_progress_->IsCanceled()) {
    CancelProgress();

    while (!tasks_.empty()) {
      auto bad_status = scada::Status{scada::StatusCode::Bad};
      if (tasks_.front().cancel) {
        tasks_.front().cancel(bad_status);
      }
      if (tasks_.front().completion) {
        CompleteTaskCompletion(*tasks_.front().completion, bad_status);
      }
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

scada::CoStatus TaskManagerImpl::PostTaskMethod(std::u16string title,
                                                TaskMethod method) {
  auto result = std::make_shared<TaskResultState<scada::Status>>();
  auto completion = scada::base::AsyncCompletion{executor_};
  auto waiter = completion.Wait();
  tasks_.push(Task{.title = std::move(title),
                   .method = [method = std::move(method),
                              result]() mutable -> scada::CoStatus {
                     result->status = co_await method();
                     co_return result->status;
                   },
                   .completion = std::move(completion),
                   .cancel =
                       [result](const scada::Status& status) mutable {
                         result->status = status;
                       }});

  PostDelayedTask(executor_, 1ms, [self = shared_from_this()] { self->Run(); });

  return WaitTaskResult(std::move(result), std::move(waiter));
}

template <class T>
scada::CoStatusOr<T> TaskManagerImpl::PostTypedTaskMethod(
    std::u16string title,
    std::function<scada::CoStatusOr<T>()> method) {
  auto result = std::make_shared<TaskResultState<T>>();
  auto task_completion = scada::base::AsyncCompletion{executor_};
  auto waiter = task_completion.Wait();

  Task task{.title = std::move(title),
            .method =
                [method = std::move(method), result]() mutable {
                  return RunTypedTaskMethod(std::move(method),
                                            std::move(result));
                },
            .completion = std::move(task_completion),
            .cancel =
                [result](const scada::Status& status) mutable {
                  result->status = status;
                }};

  tasks_.push(std::move(task));
  PostDelayedTask(executor_, 1ms, [self = shared_from_this()] { self->Run(); });

  return WaitTypedTaskResult(std::move(result), std::move(waiter));
}
