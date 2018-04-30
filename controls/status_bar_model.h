#pragma once

#include "base/strings/string16.h"

class StatusBarModelObserver {
 public:
  virtual ~StatusBarModelObserver() {}

  virtual void OnPanesChanged(int index, int count) = 0;
};

class StatusBarModel {
 public:
  ~StatusBarModel() {}

  virtual int GetPaneCount() = 0;
  virtual base::string16 GetPaneText(int index) = 0;
  virtual int GetPaneSize(int index) = 0;

  virtual void AddObserver(StatusBarModelObserver& observer) = 0;
  virtual void RemoveObserver(StatusBarModelObserver& observer) = 0;
};
