#include "services/task_manager_impl.h"

#include "commands/views/progress_dialog.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "core/attribute_service.h"
#include "core/node_management_service.h"
#include "core/status.h"
#include "services/local_events.h"
#include "services/profile.h"

#undef ReportEvent

namespace {

base::string16 FormatReference(NodeService& node_service,
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
    node_management_service.CreateNode(
        requested_id, parent_id, node_class, type_id, std::move(attributes),
        [weak_ptr, properties, callback](const scada::Status& status,
                                         const scada::NodeId& node_id) mutable {
          auto ptr = weak_ptr.get();
          if (!ptr)
            return;

          ptr->ReportRequestCompletion(status, base::string16());

          if (properties.empty() || !status) {
            if (callback)
              callback(status, node_id);
            return;
          }

          ptr->PostUpdateTask(node_id, {}, properties,
                              [node_id, callback](const scada::Status& status) {
                                if (callback)
                                  callback(status, node_id);
                              });
        });
  });
}

void TaskManagerImpl::PostUpdateTask(const scada::NodeId& node_id,
                                     scada::NodeAttributes attributes,
                                     scada::NodeProperties properties,
                                     UpdateCallback callback) {
  base::string16 title = GetDisplayName(node_service_, node_id);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::StringPrintf(L"Изменение %ls", title.c_str()), [=] {
    node_service_.GetNode(node_id).Fetch(
        NodeFetchStatus::NodeOnly(),
        [weak_ptr, node_id, &attribute_service = attribute_service_, attributes,
         properties, callback](const NodeRef& node) {
          assert(node.status());
          if (!node.status())
            return;

          auto inputs = std::vector<scada::WriteValueId>();
          inputs.reserve(1 + properties.size());
          if (!attributes.display_name.empty()) {
            inputs.emplace_back(
                scada::WriteValueId{node_id, scada::AttributeId::DisplayName,
                                    std::move(attributes.display_name)});
          }

          for (auto& [prop_decl_id, value] : properties) {
            auto prop_id = node[prop_decl_id].node_id();
            assert(!prop_id.is_null());
            inputs.emplace_back(scada::WriteValueId{std::move(prop_id),
                                                    scada::AttributeId::Value,
                                                    std::move(value)});
          }

          attribute_service.Write(
              inputs, {},
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
                  ptr->ReportRequestCompletion(status, base::string16());

                // TODO: Handle |results|.
                if (callback)
                  callback(std::move(status));
              });
        });
  });
}

void TaskManagerImpl::PostDeleteTask(const scada::NodeId& node_id) {
  base::string16 title = GetDisplayName(node_service_, node_id);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::StringPrintf(L"Удаление %ls", title.c_str()),
           [=, &node_management_service = node_management_service_] {
             node_management_service.DeleteNode(
                 node_id, true,
                 [weak_ptr](scada::Status&& status,
                            std::vector<scada::NodeId>&& dependencies) {
                   if (auto ptr = weak_ptr.get())
                     ptr->OnDeleteRecordComplete(std::move(status),
                                                 std::move(dependencies));
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
            ptr->ReportRequestCompletion(result, base::string16());
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
            ptr->ReportRequestCompletion(result, base::string16());
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

void TaskManagerImpl::OnDeleteRecordComplete(
    scada::Status&& status,
    std::vector<scada::NodeId>&& dependencies) {
  base::string16 dependencies_text;
  for (auto& node_id : dependencies) {
    if (!dependencies_text.empty())
      dependencies_text += L'\n';
    dependencies_text += GetDisplayName(node_service_, node_id);
  }

  ReportRequestCompletion(status, dependencies_text);
}

void TaskManagerImpl::ReportRequestCompletion(
    const scada::Status& status,
    const base::string16& result_text) {
  auto task = std::move(running_task_);
  running_task_ = Task();

  if (status && !profile_.show_write_ok)
    return;

  base::string16 message = base::StringPrintf(L"%ls: %ls.", task.title.c_str(),
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

void TaskManagerImpl::PostTask(base::StringPiece16 title, TaskMethod task) {
  tasks_.push({title.as_string(), std::move(task)});
}

void TaskManagerImpl::AddObserver(TaskManagerObserver& observer) {
  observers_.AddObserver(&observer);
}

void TaskManagerImpl::RemoveObserver(TaskManagerObserver& observer) {
  observers_.RemoveObserver(&observer);
}
