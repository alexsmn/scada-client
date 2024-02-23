#include "services/task_manager_impl.h"

#include "base/promise_executor.h"
#include "base/strings/stringprintf.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "scada/attribute_service_promises.h"
#include "scada/node_management_service_promises.h"
#include "scada/status_or.h"
#include "scada/status_promise.h"
#include "services/local_events.h"
#include "services/progress_host.h"

using namespace std::chrono_literals;

namespace {

std::u16string FormatReference(NodeService& node_service,
                               const scada::NodeId& reference_type_id,
                               const scada::NodeId& source_id,
                               const scada::NodeId& target_id,
                               bool add) {
  return base::StringPrintf(
      u"%ls типа %ls от %ls к %ls", add ? u"Создание связи" : u"Удаление связи",
      GetDisplayName(node_service, reference_type_id).c_str(),
      GetDisplayName(node_service, source_id).c_str(),
      GetDisplayName(node_service, target_id).c_str());
}

scada::StatusCodeOr<std::vector<scada::WriteValue>> PrepareUpdateInputs(
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

scada::status_promise<void> TaskManagerImpl::PostTask(
    std::u16string_view description,
    const TaskLauncher& launcher) {
  return PostTaskMethod(description, [this, launcher] {
    scada::ToExplicitStatusCodePromise(launcher())
        .then(BindPromiseExecutor(
            executor_, weak_from_this(), [this](scada::StatusCode status_code) {
              ReportRequestCompletion(status_code, /*result_text=*/{});
            }));
  });
}

scada::status_promise<scada::NodeId> TaskManagerImpl::PostInsertTask(
    const scada::NodeId& requested_id,
    const scada::NodeId& parent_id,
    const scada::NodeId& type_def_id,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties,
    std::vector<scada::ReferenceDescription> references) {
  scada::status_promise<scada::NodeId> final_promise;

  auto ref = shared_from_this();

  auto node_state = scada::NodeState{.node_id = requested_id,
                                     .type_definition_id = type_def_id,
                                     .parent_id = parent_id,
                                     .attributes = std::move(attributes),
                                     .properties = std::move(properties),
                                     .references = std::move(references)};

  // Cannot use `PostTask` because it only accepts void promises.
  PostTaskMethod(u"Вставка", [this, ref, node_state = std::move(node_state),
                              final_promise] {
    auto type_def = node_service_.GetNode(node_state.type_definition_id);

    auto add_node_promise =
        FetchNode(type_def).then([this, ref, type_def, node_state] {
          if (type_def.node_class() != scada::NodeClass::ObjectType &&
              type_def.node_class() != scada::NodeClass::VariableType) {
            throw scada::status_exception{scada::StatusCode::Bad_WrongTypeId};
          }

          auto node_class =
              type_def.node_class() == scada::NodeClass::ObjectType
                  ? scada::NodeClass::Object
                  : scada::NodeClass::Variable;

          return scada::AddNode(
              node_management_service_,
              {node_state.node_id, node_state.parent_id, node_class,
               node_state.type_definition_id, node_state.attributes});
        });

    scada::ToExplicitStatusCodePromise(add_node_promise)
        .then(BindExecutor(executor_, weak_from_this(),
                           [this](scada::StatusCode status_code) {
                             ReportRequestCompletion(status_code,
                                                     /*result_text=*/{});
                           }));

    auto add_props_and_refs_promise = add_node_promise.then(BindPromiseExecutor(
        executor_, weak_from_this(),
        [this, node_state](const scada::NodeId& added_node_id) {
          // TODO: Combine with properties.

          for (const auto& reference : node_state.references) {
            if (reference.forward)
              PostAddReference(reference.reference_type_id, added_node_id,
                               reference.node_id);
            else
              PostAddReference(reference.reference_type_id, reference.node_id,
                               added_node_id);
          }

          if (node_state.properties.empty()) {
            return scada::MakeResolvedStatusPromise(added_node_id);
          }

          return ToValuePromise(PostUpdateTask(added_node_id, /*attributes=*/{},
                                               node_state.properties),
                                added_node_id);
        }));

    ForwardPromise(add_props_and_refs_promise, final_promise);
  });

  return final_promise;
}

scada::status_promise<void> TaskManagerImpl::PostUpdateTask(
    const scada::NodeId& node_id,
    scada::NodeAttributes attributes,
    scada::NodeProperties properties) {
  std::u16string title = GetDisplayName(node_service_, node_id);
  return PostTask(
      base::StringPrintf(u"Изменение %ls", title.c_str()), [=]() mutable {
        auto node = node_service_.GetNode(node_id);
        return FetchNode(node)
            .then([&attribute_service = attribute_service_, node, attributes,
                   properties]() mutable {
              assert(node.fetched());

              auto inputs = PrepareUpdateInputs(node, std::move(attributes),
                                                std::move(properties));
              if (!inputs.ok()) {
                throw scada::status_exception{inputs.status_code()};
              }

              return scada::Write(attribute_service,
                                  scada::ServiceContext::default_instance(),
                                  std::move(*inputs));
            })
            .then([](const std::vector<scada::StatusCode>& results) {
              // Find any failed status.
              auto i = std::ranges::find_if(results, &scada::IsBad);
              if (i != results.end()) {
                throw scada::status_exception{*i};
              }
            });
      });
}

scada::status_promise<void> TaskManagerImpl::PostDeleteTask(
    const scada::NodeId& node_id) {
  std::u16string title = GetDisplayName(node_service_, node_id);
  return PostTask(
      base::StringPrintf(u"Удаление %ls", title.c_str()),
      [this, node_id,
       &node_management_service = node_management_service_]() mutable {
        return scada::DeleteNode(node_management_service, {node_id});
      });
}

scada::status_promise<void> TaskManagerImpl::PostAddReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, true);
  return PostTaskMethod(title, [=, &node_management_service =
                                       node_management_service_]() mutable {
    scada::AddReferencesItem input{
        source_id, reference_type_id, true, {}, target_id};
    node_management_service.AddReferences(
        std::vector<scada::AddReferencesItem>(1, input),
        BindExecutor(executor_, weak_from_this(),
                     [this](scada::Status status,
                            std::vector<scada::StatusCode> results) mutable {
                       auto result =
                           status ? results.front() : std::move(status);
                       ReportRequestCompletion(result, std::u16string());
                     }));
  });
}

scada::status_promise<void> TaskManagerImpl::PostDeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, false);
  return PostTaskMethod(title, [=, &node_management_service =
                                       node_management_service_]() mutable {
    scada::DeleteReferencesItem input{source_id, reference_type_id, true,
                                      target_id, true};
    node_management_service.DeleteReferences(
        std::vector<scada::DeleteReferencesItem>(1, input),
        BindExecutor(executor_, weak_from_this(),
                     [this](scada::Status status,
                            std::vector<scada::StatusCode> results) mutable {
                       auto result =
                           status ? results.front() : std::move(status);
                       ReportRequestCompletion(result, std::u16string());
                     }));
  });
}

void TaskManagerImpl::StartTask(Task&& task) {
  assert(!task.IsNull());

  running_task_ = std::move(task);

  if (running_progress_)
    running_progress_->SetStatus((running_task_.title + u"...").c_str());

  // The running task keeps captured variables and must be alive until it's
  // entirely completed. It cannot be deleted immediately from
  // `ReportRequestCompletion`, since `ReportRequestCompletion` might be called
  // from within the task.
  auto captured_task = running_task_;

  captured_task.task();
}

void TaskManagerImpl::ReportRequestCompletion(
    const scada::Status& status,
    const std::u16string& result_text) {
  auto task = std::move(running_task_);
  running_task_ = Task();

  if (!status || profile_.show_write_ok) {
    std::u16string message = base::StringPrintf(
        u"%ls: %ls.", task.title.c_str(), ToString16(status).c_str());
    if (!result_text.empty())
      message += u'\n' + result_text;

    auto severity = status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
    local_events_.ReportEvent(severity, message);
  }

  scada::ResolveStatusPromise(task.promise, status);
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
    if (!start_time_.has_value())
      start_time_ = std::chrono::steady_clock::now();

    if (!running_progress_ &&
        std::chrono::steady_clock::now() - *start_time_ >= 300ms) {
      running_progress_ = progress_host_.Start();
    }

    if (running_progress_)
      running_progress_->SetProgress(count_, count_ - tasks_.size());

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

scada::status_promise<void> TaskManagerImpl::PostTaskMethod(
    std::u16string_view title,
    TaskMethod task) {
  auto promise = tasks_.emplace(std::u16string{title}, std::move(task)).promise;

  Run();

  return promise;
}
