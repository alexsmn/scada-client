#pragma once

#include "base/files/file_path.h"
#include "services/page_layout.h"
#include "window_definition.h"

namespace xml {
class Node;
}

class Page {
 public:
  Page() : id(0) {}

  Page(const Page& source);
  Page& operator=(const Page& source);

  base::string16 GetTitle() const;

  WindowDefinition& AddWindow(const WindowDefinition& window);
  WindowDefinition* FindWindowDef(int id);
  const WindowDefinition* FindWindowDef(int id) const;
  int FindWindowDef(const WindowDefinition& window) const;
  void DeleteWindow(int index);
  void Clear();

  int GetWindowCount() const { return windows_.size(); }
  WindowDefinition& GetWindow(int index) const { return *windows_[index]; }

  void Load(const xml::Node& node);
  void Save(xml::Node& node, bool current) const;

  base::DictionaryValue LoadJson();
  void SaveJson(base::DictionaryValue& json) const;

  int id;
  base::string16 title;

  PageLayout layout;

 private:
  int NewWindowId();

  typedef std::vector<std::unique_ptr<WindowDefinition>> Windows;
  Windows windows_;
};

base::FilePath GetPagePath(const char* name);
base::FilePath GetPagePath(int id);
