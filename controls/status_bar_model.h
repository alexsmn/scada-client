#pragma once

#include "base/strings/string16.h"

class StatusBarModelObserver {
 public:
  virtual ~StatusBarModelObserver() {}

  virtual void OnPanesChanged(int index, int count) = 0;
  virtual void OnProgressChanged() = 0;
};

class StatusBarModel {
 public:
  ~StatusBarModel() {}

  virtual int GetPaneCount() = 0;
  virtual base::string16 GetPaneText(int index) = 0;
  virtual int GetPaneSize(int index) = 0;

  struct Progress {
    bool active = false;
    int range = 0;
    int current = 0;
  };

  virtual Progress GetProgress() const = 0;

  virtual void AddObserver(StatusBarModelObserver& observer) = 0;
  virtual void RemoveObserver(StatusBarModelObserver& observer) = 0;
};
