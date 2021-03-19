#include "components/configuration_export/importer.h"

#include "components/configuration_export/import_data.h"
#include "services/task_manager.h"

void ApplyImportData(const ImportData& import_data, TaskManager& task_manager) {
  for (auto& p : import_data.create_nodes) {
    task_manager.PostInsertTask(p.id, p.parent_id, p.type_id, p.attrs, p.props);
    for (auto& ref : p.refs) {
      assert(ref.delete_target_id.is_null());
      assert(!ref.add_target_id.is_null());
      task_manager.PostAddReference(ref.reference_type_id, p.id,
                                    ref.add_target_id);
    }
  }

  for (auto& p : import_data.modify_nodes) {
    if (!p.attrs.empty() || !p.props.empty())
      task_manager.PostUpdateTask(p.id, p.attrs, p.props);
    for (auto& ref : p.refs) {
      if (!ref.delete_target_id.is_null())
        task_manager.PostDeleteReference(ref.reference_type_id, p.id,
                                         ref.add_target_id);
      if (!ref.add_target_id.is_null())
        task_manager.PostAddReference(ref.reference_type_id, p.id,
                                      ref.add_target_id);
    }
  }

  for (auto& p : import_data.delete_nodes)
    task_manager.PostDeleteTask(p);
}
