#pragma once

#include <string>

namespace aui {

class StatusBarModelObserver {
 public:
  virtual ~StatusBarModelObserver() = default;

  virtual void OnPanesChanged(int index, int count) = 0;
};

class StatusBarModel {
 public:
  virtual ~StatusBarModel() = default;

  virtual int GetPaneCount() const = 0;
  virtual std::u16string GetPaneText(int index) const = 0;
  virtual int GetPaneSize(int index) const = 0;

  virtual void AddObserver(StatusBarModelObserver& observer) = 0;
  virtual void RemoveObserver(StatusBarModelObserver& observer) = 0;
};

}  // namespace aui
