#include "services/task_manager_impl.h"

#include "commands/views/progress_dialog.h"
#include "common/node_service.h"
#include "common/node_util.h"
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
  progress_dialog_.reset();
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
        [weak_ptr, node_id, &node_management_service = node_management_service_,
         attributes, properties, callback](const NodeRef& node) {
          assert(node.status());
          if (!node.status())
            return;

          std::vector<std::pair<scada::NodeId, scada::NodeAttributes>> nodes;
          nodes.reserve(1 + properties.size());
          if (!attributes.empty())
            nodes.emplace_back(node_id, std::move(attributes));

          for (auto& [prop_decl_id, value] : properties) {
            auto prop_id = node[prop_decl_id].node_id();
            assert(!prop_id.is_null());
            scada::NodeAttributes attributes;
            attributes.value = std::move(value);
            nodes.emplace_back(std::move(prop_id), std::move(attributes));
          }

          node_management_service.ModifyNodes(
              nodes, [weak_ptr, node_id, callback](
                         const scada::Status& status,
                         const std::vector<scada::Status>& results) {
                if (auto* ptr = weak_ptr.get())
                  ptr->ReportRequestCompletion(status, base::string16());

                if (callback)
                  callback(status);
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
                 [weak_ptr](const scada::Status& status,
                            const scada::NodeIdSet* relations) {
                   if (auto ptr = weak_ptr.get())
                     ptr->OnDeleteRecordComplete(status, relations);
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
    node_management_service.AddReference(
        reference_type_id, source_id, target_id,
        [weak_ptr](const scada::Status& status) {
          if (auto ptr = weak_ptr.get())
            ptr->ReportRequestCompletion(status, base::string16());
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
    node_management_service.DeleteReference(
        reference_type_id, source_id, target_id,
        [weak_ptr](const scada::Status& status) {
          if (auto ptr = weak_ptr.get())
            ptr->ReportRequestCompletion(status, base::string16());
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
    const scada::Status& status,
    const std::set<scada::NodeId>* relations) {
  base::string16 relations_text;
  for (auto& node_id : *relations) {
    if (!relations_text.empty())
      relations_text += L'\n';
    relations_text += GetDisplayName(node_service_, node_id);
  }

  ReportRequestCompletion(status, relations_text);
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

    if (progress_dialog_)
      progress_dialog_->SetProgress(count, count - tasks_.size());

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
