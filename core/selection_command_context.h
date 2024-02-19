#pragma once

class DialogService;
class SelectionModel;

struct SelectionCommandContext {
  const SelectionModel& selection;
  DialogService& dialog_service;
};