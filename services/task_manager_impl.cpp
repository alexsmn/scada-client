#include "services/task_manager_impl.h"

#include "base/executor.h"
#include "base/strings/stringprintf.h"
#include "core/attribute_service.h"
#include "core/node_management_service.h"
#include "core/status.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/local_events.h"
#include "services/profile.h"
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

void TaskManagerImpl::PostInsertTask(const scada::NodeId& requested_id,
                                     const scada::NodeId& parent_id,
                                     const scada::NodeId& type_id,
                                     scada::NodeAttributes attributes,
                                     scada::NodeProperties properties,
                                     InsertCallback callback) {
  node_service_.GetNode(type_id).Fetch(
      NodeFetchStatus::NodeOnly(),
      [=, ref = shared_from_this()](const NodeRef& type) {
        if (!type) {
          assert(false);
          if (callback)
            callback(scada::StatusCode::Bad, {});
          return;
        }

        if (type.node_class() != scada::NodeClass::ObjectType &&
            type.node_class() != scada::NodeClass::VariableType) {
          assert(false);
          if (callback)
            callback(scada::StatusCode::Bad, {});
          return;
        }

        auto node_class = type.node_class() == scada::NodeClass::ObjectType
                              ? scada::NodeClass::Object
                              : scada::NodeClass::Variable;

        PostTask(u"Вставка", [=, ref = shared_from_this()] {
          scada::AddNode(
              node_management_service_,
              {requested_id, parent_id, node_class, type_id, attributes},
              BindExecutor(
                  executor_, weak_from_this(),
                  [this, properties, callback](scada::AddNodesResult result) {
                    ReportRequestCompletion(result.status_code, {});

                    if (properties.empty() ||
                        scada::IsBad(result.status_code)) {
                      if (callback)
                        callback(result.status_code, result.added_node_id);
                      return;
                    }

                    PostUpdateTask(result.added_node_id, {}, properties,
                                   [added_node_id = result.added_node_id,
                                    callback](scada::Status status) {
                                     if (callback)
                                       callback(std::move(status),
                                                added_node_id);
                                   });
                  }));
        });
      });
}

void TaskManagerImpl::PostUpdateTask(const scada::NodeId& node_id,
                                     scada::NodeAttributes attributes,
                                     scada::NodeProperties properties,
                                     UpdateCallback callback) {
  std::u16string title = GetDisplayName(node_service_, node_id);

  PostTask(base::StringPrintf(u"Изменение %ls", title.c_str()), [=] {
    node_service_.GetNode(node_id).Fetch(
        NodeFetchStatus::NodeOnly(),
        BindExecutor(
            executor_, weak_from_this(),
            [this, node_id, attributes, properties,
             callback](const NodeRef& node) {
              assert(node.status());
              if (!node.status()) {
                if (callback)
                  callback(node.status());
                return;
              }

              auto inputs = std::make_shared<std::vector<scada::WriteValue>>();
              inputs->reserve(2 + properties.size());
              if (!attributes.browse_name.empty()) {
                inputs->emplace_back(
                    scada::WriteValue{node_id, scada::AttributeId::BrowseName,
                                      std::move(attributes.browse_name)});
              }
              if (!attributes.display_name.empty()) {
                inputs->emplace_back(
                    scada::WriteValue{node_id, scada::AttributeId::DisplayName,
                                      std::move(attributes.display_name)});
              }

              for (auto& [prop_decl_id, value] : properties) {
                auto prop_id = node[prop_decl_id].node_id();
                assert(!prop_id.is_null());
                inputs->emplace_back(scada::WriteValue{
                    std::move(prop_id), scada::AttributeId::Value,
                    std::move(value)});
              }

              attribute_service_.Write(
                  scada::ServiceContext::default_instance(), inputs,
                  BindExecutor(
                      executor_, weak_from_this(),
                      [this, node_id, callback](
                          scada::Status status,
                          std::vector<scada::StatusCode> results) {
                        if (status) {
                          // Find any failed status.
                          auto i = std::find_if(results.begin(), results.end(),
                                                [](const auto& status) {
                                                  return scada::IsBad(status);
                                                });
                          if (i != results.end())
                            status = std::move(*i);
                        }

                        ReportRequestCompletion(status, std::u16string());

                        // TODO: Handle |results|.
                        if (callback)
                          callback(std::move(status));
                      }));
            }));
  });
}

void TaskManagerImpl::PostDeleteTask(const scada::NodeId& node_id) {
  std::u16string title = GetDisplayName(node_service_, node_id);
  PostTask(base::StringPrintf(u"Удаление %ls", title.c_str()),
           [=, &node_management_service = node_management_service_] {
             scada::DeleteNode(node_management_service, {node_id},
                               BindExecutor(executor_, weak_from_this(),
                                            [this](scada::Status status) {
                                              ReportRequestCompletion(
                                                  std::move(status), {});
                                            }));
           });
}

void TaskManagerImpl::PostAddReference(const scada::NodeId& reference_type_id,
                                       const scada::NodeId& source_id,
                                       const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, true);
  PostTask(title, [=, &node_management_service = node_management_service_] {
    scada::AddReferencesItem input{
        source_id, reference_type_id, true, {}, target_id};
    node_management_service.AddReferences(
        std::vector<scada::AddReferencesItem>(1, input),
        BindExecutor(executor_, weak_from_this(),
                     [this](scada::Status status,
                            std::vector<scada::StatusCode> results) {
                       auto result =
                           status ? results.front() : std::move(status);
                       ReportRequestCompletion(result, std::u16string());
                     }));
  });
}

void TaskManagerImpl::PostDeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, false);
  PostTask(title, [=, &node_management_service = node_management_service_] {
    scada::DeleteReferencesItem input{source_id, reference_type_id, true,
                                      target_id, true};
    node_management_service.DeleteReferences(
        std::vector<scada::DeleteReferencesItem>(1, input),
        BindExecutor(executor_, weak_from_this(),
                     [this](scada::Status status,
                            std::vector<scada::StatusCode> results) {
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

  running_task_.task();
}

void TaskManagerImpl::ReportRequestCompletion(
    const scada::Status& status,
    const std::u16string& result_text) {
  auto task = std::move(running_task_);
  running_task_ = Task();

  if (status && !profile_.show_write_ok)
    return;

  std::u16string message = base::StringPrintf(u"%ls: %ls.", task.title.c_str(),
                                              ToString16(status).c_str());
  if (!result_text.empty())
    message += u'\n' + result_text;

  auto severity = status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
  local_events_.ReportEvent(severity, message);
}

void TaskManagerImpl::Run() {
  // Handle dialog Cancel button. Allow complete current request.
  if (running_progress_ && running_progress_->IsCanceled()) {
    CancelProgress();

    while (!tasks_.empty())
      tasks_.pop();

    running_task_ = Task();
  }

  if (count_ < static_cast<int>(tasks_.size()))
    count_ = tasks_.size();

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

void TaskManagerImpl::PostTask(std::u16string_view title, TaskMethod task) {
  tasks_.push({std::u16string{title}, std::move(task)});
}
