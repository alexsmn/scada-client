#pragma once

#include "base/files/file_path.h"
#include "services/page_layout.h"
#include "window_definition.h"

class Page {
 public:
  Page() : id(0) {}

  Page(const Page& source);
  Page& operator=(const Page& source);

  std::u16string GetTitle() const;

  WindowDefinition& AddWindow(const WindowDefinition& window);
  WindowDefinition* FindWindowDef(int id);
  const WindowDefinition* FindWindowDef(int id) const;
  int FindWindowDef(const WindowDefinition& window) const;
  void DeleteWindow(int index);
  void Clear();

  int GetWindowCount() const { return windows_.size(); }
  WindowDefinition& GetWindow(int index) const { return *windows_[index]; }

  void Load(const base::Value& value);
  base::Value Save(bool current) const;

  int id;
  std::u16string title;

  PageLayout layout;

 private:
  int NewWindowId();

  typedef std::vector<std::unique_ptr<WindowDefinition>> Windows;
  Windows windows_;
};
