#pragma once

class TaskManager;
struct DiffData;
struct ImportData;

void ApplyImportData(const ImportData& import_data, TaskManager& task_manager);
void ApplyDiffData(const DiffData& import_data, TaskManager& task_manager);
