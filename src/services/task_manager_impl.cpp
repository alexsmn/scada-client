#include "services/task_manager_impl.h"

#include "commands/views/progress_dialog.h"
#include "common/node_service.h"
#include "core/node_management_service.h"
#include "core/status.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "translation.h"

namespace {

std::string FormatReference(NodeService& node_service,
                            const scada::NodeId& reference_type_id,
                            const scada::NodeId& source_id,
                            const scada::NodeId& target_id) {
  return base::StringPrintf(
      "%s {%s} %s",
      node_service.GetNode(source_id).browse_name().name().c_str(),
      node_service.GetNode(reference_type_id).browse_name().name().c_str(),
      node_service.GetNode(target_id).browse_name().name().c_str());
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
  auto weak_ptr = weak_factory_.GetWeakPtr();
  node_service_.GetNode(type_id).Fetch([=](NodeRef type_definition) {
    if (!weak_ptr.get())
      return;

    if (!type_definition.status()) {
      // TODO: Report error.
      assert(false);
      return;
    }

    if (type_definition.node_class() != scada::NodeClass::ObjectType &&
        type_definition.node_class() != scada::NodeClass::VariableType) {
      // TODO: Report error.
      assert(false);
      return;
    }

    auto node_class =
        type_definition.node_class() == scada::NodeClass::ObjectType
            ? scada::NodeClass::Object
            : scada::NodeClass::Variable;

    PostTask(L"Вставка", [=] {
      node_management_service_.CreateNode(
          requested_id, parent_id, node_class, type_id, std::move(attributes),
          [=](const scada::Status& status, const scada::NodeId& node_id) {
            if (!weak_ptr.get())
              return;

            // Attributes have already been applied.

            if (properties.empty()) {
              ReportRequestCompletion(status, base::string16());
              return;
            }

            PostUpdateTask(node_id, {}, std::move(properties),
                           [=](const scada::Status& status) {
                             if (!weak_ptr.get())
                               return;
                             ReportRequestCompletion(status, base::string16());
                             callback(status, node_id);
                           });
          });
    });
  });
}

void TaskManagerImpl::PostUpdateTask(const scada::NodeId& node_id,
                                     scada::NodeAttributes attributes,
                                     scada::NodeProperties properties,
                                     UpdateCallback callback) {
  assert(!attributes.empty() || !properties.empty());

  auto weak_ptr = weak_factory_.GetWeakPtr();
  node_service_.GetNode(node_id).Fetch([=](const NodeRef& node) {
    if (!weak_ptr.get())
      return;

    if (!node.status()) {
      // TODO: Report error.
      assert(false);
      return;
    }

    base::string16 title = ToString16(node.display_name());

    std::vector<std::pair<scada::NodeId, scada::NodeAttributes>> nodes;
    nodes.reserve(1 + properties.size());

    if (!attributes.empty())
      nodes.emplace_back(node_id, std::move(attributes));

    for (auto& p : properties) {
      auto id = node[p.first].id();
      // TODO: !id.is_null()
      scada::NodeAttributes attributes;
      attributes.set_value(p.second);
      nodes.emplace_back(std::move(id), std::move(attributes));
    }

    PostTask(base::StringPrintf(L"Изменение %ls", title.c_str()), [=] {
      node_management_service_.ModifyNodes(
          nodes, [=](const scada::Status& status,
                     const std::vector<scada::Status>& statuses) {
            if (!weak_ptr.get())
              return;
            ReportRequestCompletion(statuses.front(), base::string16());
            if (callback)
              callback(status);
          });
    });
  });
}

void TaskManagerImpl::PostDeleteTask(const scada::NodeId& node_id) {
  base::string16 title =
      ToString16(node_service_.GetNode(node_id).display_name());
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::StringPrintf(L"Удаление %ls", title.c_str()), [=] {
    node_management_service_.DeleteNode(
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
  auto title =
      FormatReference(node_service_, reference_type_id, source_id, target_id);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::SysNativeMBToWide(title), [=] {
    node_management_service_.AddReference(
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
  auto title =
      FormatReference(node_service_, reference_type_id, source_id, target_id);
  auto weak_ptr = weak_factory_.GetWeakPtr();
  PostTask(base::SysNativeMBToWide(title), [=] {
    node_management_service_.DeleteReference(
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
    relations_text += ToString16(node_service_.GetNode(node_id).display_name());
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
