#pragma once

class TaskManager;
struct DiffData;

void ApplyDiffData(const DiffData& import_data, TaskManager& task_manager);
