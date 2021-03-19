#pragma once

class TaskManager;
struct ImportData;

void ApplyImportData(const ImportData& import_data, TaskManager& task_manager);
