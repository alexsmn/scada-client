#include "services/task_manager_impl.h"

#include "commands/views/progress_dialog.h"
#include "core/attribute_service.h"
#include "core/node_management_service.h"
#include "core/status.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/local_events.h"
#include "services/profile.h"

#undef ReportEvent

namespace {

std::wstring FormatReference(NodeService& node_service,
                             const scada::NodeId& reference_type_id,
                             const scada::NodeId& source_id,
                             const scada::NodeId& target_id,
                             bool add) {
  return base::StringPrintf(
      L"%ls типа %ls от %ls к %ls", add ? L"Создание связи" : L"Удаление связи",
      GetDisplayName(node_service, reference_type_id).c_str(),
      GetDisplayName(node_service, source_id).c_str(),
      GetDisplayName(node_service, target_id).c_str());
}

}  // namespace

// TaskManagerImpl

TaskManagerImpl::TaskManagerImpl(TaskManagerImplContext&& context)
    : TaskManagerImplContext{std::move(context)} {
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10), this,
               &TaskManagerImpl::Run);
}

TaskManagerImpl::~TaskManagerImpl() {
  CancelProgressDialog();
}

void TaskManagerImpl::CancelProgressDialog() {
  // WARNING: Limit callback count.

  if (!progress_dialog_)
    return;

  progress_dialog_.reset();

  const TaskManagerObserver::Status status{false};
  for (auto& o : observers_)
    o.OnTaskManagerStatus(status);
}

void TaskManagerImpl::PostInsertTask(const scada::NodeId& requested_id,
                                     const scada::NodeId& parent_id,
                                     const scada::NodeId& type_id,
                                     scada::NodeAttributes attributes,
                                     scada::NodeProperties properties,
                                     InsertCallback callback) {
  auto type = node_service_.GetNode(type_id);
  if (!type) {
    assert(false);
    return;
  }

  if (type.node_class() != scada::NodeClass::ObjectType &&
      type.node_class() != scada::NodeClass::VariableType) {
    assert(false);
    return;
  }

  auto weak_ptr = weak_factory_.GetWeakPtr();
  auto node_class = type.node_class() == scada::NodeClass::ObjectType
                        ? scada::NodeClass::Object
                        : scada::NodeClass::Variable;

  PostTask(L"Вставка", [=,
                        &node_management_service = node_management_service_] {
    scada::AddNode(
        node_management_service,
        {requested_id, parent_id, node_class, type_id, std::move(attributes)},
        [weak_ptr, properties,
         callback](scada::AddNodesResult&& result) mutable {
          auto ptr = weak_ptr.get();
          if (!ptr)
            return;

          ptr->ReportRequestCompletion(result.status_code, {});

          if (properties.empty() || scada::IsBad(result.status_code)) {
            if (callback)
              callback(result.status_code, result.added_node_id);
            return;
          }

          ptr->PostUpdateTask(result.added_node_id, {}, properties,
                              [added_node_id = result.added_node_id,
                               callback](scada::Status&& status) {
                                if (callback)
                                  callback(status, added_node_id);
                              });
        });
  });
}

void TaskManagerImpl::PostUpdateTask(const scada::NodeId& node_id,
                                     scada::NodeAttributes attributes,
                                     scada::NodeProperties properties,
                                     UpdateCallback callback) {
  std::wstring title = GetDisplayName(node_service_, node_id);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::StringPrintf(L"Изменение %ls", title.c_str()), [=] {
    node_service_.GetNode(node_id).Fetch(
        NodeFetchStatus::NodeOnly(),
        [weak_ptr, node_id, &attribute_service = attribute_service_, attributes,
         properties, callback](const NodeRef& node) {
          assert(node.status());
          if (!node.status()) {
            if (callback)
              callback(node.status());
            return;
          }

          auto inputs = std::make_shared<std::vector<scada::WriteValue>>();
          inputs->reserve(1 + properties.size());
          if (!attributes.display_name.empty()) {
            inputs->emplace_back(
                scada::WriteValue{node_id, scada::AttributeId::DisplayName,
                                  std::move(attributes.display_name)});
          }

          for (auto& [prop_decl_id, value] : properties) {
            auto prop_id = node[prop_decl_id].node_id();
            assert(!prop_id.is_null());
            inputs->emplace_back(scada::WriteValue{std::move(prop_id),
                                                   scada::AttributeId::Value,
                                                   std::move(value)});
          }

          attribute_service.Write(
              scada::ServiceContext::default_instance(), inputs,
              [weak_ptr, node_id, callback](
                  scada::Status&& status,
                  std::vector<scada::StatusCode>&& results) {
                if (status) {
                  // Find any failed status.
                  auto i = std::find_if(
                      results.begin(), results.end(),
                      [](const auto& status) { return scada::IsBad(status); });
                  if (i != results.end())
                    status = std::move(*i);
                }

                if (auto* ptr = weak_ptr.get())
                  ptr->ReportRequestCompletion(status, std::wstring());

                // TODO: Handle |results|.
                if (callback)
                  callback(std::move(status));
              });
        });
  });
}

void TaskManagerImpl::PostDeleteTask(const scada::NodeId& node_id) {
  std::wstring title = GetDisplayName(node_service_, node_id);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::StringPrintf(L"Удаление %ls", title.c_str()),
           [=, &node_management_service = node_management_service_] {
             scada::DeleteNode(node_management_service, {node_id},
                               [weak_ptr](scada::Status&& status) {
                                 if (auto ptr = weak_ptr.get())
                                   ptr->ReportRequestCompletion(status, {});
                               });
           });
}

void TaskManagerImpl::PostAddReference(const scada::NodeId& reference_type_id,
                                       const scada::NodeId& source_id,
                                       const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, true);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(title, [=, &node_management_service = node_management_service_] {
    scada::AddReferencesItem input{
        source_id, reference_type_id, true, {}, target_id};
    node_management_service.AddReferences(
        std::vector<scada::AddReferencesItem>(1, input),
        [weak_ptr](const scada::Status& status,
                   const std::vector<scada::StatusCode>& results) {
          auto result = status ? results.front() : status;
          if (auto ptr = weak_ptr.get())
            ptr->ReportRequestCompletion(result, std::wstring());
        });
  });
}

void TaskManagerImpl::PostDeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  auto title = FormatReference(node_service_, reference_type_id, source_id,
                               target_id, false);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(title, [=, &node_management_service = node_management_service_] {
    scada::DeleteReferencesItem input{source_id, reference_type_id, true,
                                      target_id};
    node_management_service.DeleteReferences(
        std::vector<scada::DeleteReferencesItem>(1, input),
        [weak_ptr](const scada::Status& status,
                   const std::vector<scada::StatusCode>& results) {
          auto result = status ? results.front() : status;
          if (auto ptr = weak_ptr.get())
            ptr->ReportRequestCompletion(result, std::wstring());
        });
  });
}

void TaskManagerImpl::StartTask(Task&& task) {
  assert(!task.IsNull());

  running_task_ = std::move(task);

  if (progress_dialog_)
    progress_dialog_->SetStatus((running_task_.title + L"...").c_str());

  running_task_.task();
}

void TaskManagerImpl::ReportRequestCompletion(const scada::Status& status,
                                              const std::wstring& result_text) {
  auto task = std::move(running_task_);
  running_task_ = Task();

  if (status && !profile_.show_write_ok)
    return;

  std::wstring message = base::StringPrintf(L"%ls: %ls.", task.title.c_str(),
                                            ToString16(status).c_str());
  if (!result_text.empty())
    message += L'\n' + result_text;

  LocalEvents::Severity severity =
      status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
  local_events_.ReportEvent(severity, message);
}

void TaskManagerImpl::Run() {
  // Handle dialog Cancel button. Allow complete current request.
  if (progress_dialog_ && progress_dialog_->IsCancelled()) {
    CancelProgressDialog();

    while (!tasks_.empty())
      tasks_.pop();

    running_task_ = Task();
  }

  if (count < (int)tasks_.size())
    count = tasks_.size();

  // show or hide dialog
  if (!running_task_.IsNull() || !tasks_.empty()) {
    if (!start_time)
      start_time = GetTickCount();

    if (!progress_dialog_ && GetTickCount() - start_time >= 300)
      progress_dialog_ = CreateProgressDialog();

    if (progress_dialog_) {
      progress_dialog_->SetProgress(count, count - tasks_.size());

      const TaskManagerObserver::Status status{
          true, count, count - static_cast<int>(tasks_.size())};
      for (auto& o : observers_)
        o.OnTaskManagerStatus(status);
    }

  } else {
    CancelProgressDialog();
    start_time = 0;
    count = 0;
  }

  // process next task
  if (running_task_.IsNull() && !tasks_.empty()) {
    auto task = std::move(tasks_.front());
    tasks_.pop();
    StartTask(std::move(task));
  }
}

void TaskManagerImpl::PostTask(std::wstring_view title, TaskMethod task) {
  tasks_.push({std::wstring{title}, std::move(task)});
}

void TaskManagerImpl::AddObserver(TaskManagerObserver& observer) {
  observers_.AddObserver(&observer);
}

void TaskManagerImpl::RemoveObserver(TaskManagerObserver& observer) {
  observers_.RemoveObserver(&observer);
}
