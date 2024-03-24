#pragma once

class DialogService;
class MainWindowInterface;
class SelectionModel;
class OpenedViewInterface;

struct SelectionCommandContext {
  const SelectionModel& selection;
  DialogService& dialog_service;
  MainWindowInterface& main_window;
  OpenedViewInterface& opened_view;
};