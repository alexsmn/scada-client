#include "export/configuration/importer.h"

#include "export/configuration/diff_data.h"
#include "export/configuration/import_data.h"
#include "services/task_manager.h"

template <class T>
void ApplyImportDataT(const T& import_data, TaskManager& task_manager) {
  for (const auto& p : import_data.create_nodes) {
    task_manager.PostInsertTask(p.id, p.parent_id, p.type_id, p.attrs, p.props,
                                /*references=*/{});

    for (const auto& ref : p.refs) {
      assert(ref.delete_target_id.is_null());
      assert(!ref.add_target_id.is_null());
      task_manager.PostAddReference(ref.reference_type_id, p.id,
                                    ref.add_target_id);
    }
  }

  for (const auto& p : import_data.modify_nodes) {
    if (!p.attrs.empty() || !p.props.empty()) {
      task_manager.PostUpdateTask(p.id, p.attrs, p.props);
    }

    for (auto& ref : p.refs) {
      if (!ref.delete_target_id.is_null()) {
        task_manager.PostDeleteReference(ref.reference_type_id, p.id,
                                         ref.add_target_id);
      }

      if (!ref.add_target_id.is_null()) {
        task_manager.PostAddReference(ref.reference_type_id, p.id,
                                      ref.add_target_id);
      }
    }
  }

  for (auto& p : import_data.delete_nodes) {
    task_manager.PostDeleteTask(p);
  }
}

void ApplyImportData(const ImportData& import_data, TaskManager& task_manager) {
  ApplyImportDataT(import_data, task_manager);
}

void ApplyDiffData(const DiffData& diff, TaskManager& task_manager) {
  ApplyImportDataT(diff, task_manager);
}
