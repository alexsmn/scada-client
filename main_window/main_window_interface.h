#pragma once

class OpenedView;

class MainWindowInterface {
 public:
  virtual ~MainWindowInterface() = default;

  virtual OpenedView* GetActiveView() = 0;
  virtual OpenedView* GetActiveDataView() = 0;
};
