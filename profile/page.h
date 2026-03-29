#pragma once

#include <boost/json.hpp>
#include "profile/page_layout.h"
#include "profile/window_definition.h"

class Page {
 public:
  Page() = default;

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

  void Load(const boost::json::value& value);
  boost::json::value Save(bool current) const;

  int id = 0;
  std::u16string title;

  PageLayout layout;

 private:
  int NewWindowId();

  using Windows = std::vector<std::unique_ptr<WindowDefinition>>;
  Windows windows_;
};
