#pragma once

#include "base/files/file_path.h"
#include "client/services/page_layout.h"
#include "client/window_definition.h"

namespace xml {
class Node;
}

class Page {
 public:
	Page() : id(0) {}

	base::string16 GetTitle() const;
	PageLayout& layout() { return layout_; }
	const PageLayout& layout() const { return layout_; }

	WindowDefinition& AddWindow(const WindowDefinition& window);
	WindowDefinition* FindWindow(int id);
  const WindowDefinition* FindWindow(int id) const;
  int FindWindow(const WindowDefinition& window) const;
	void DeleteWindow(int index);
	void Clear();

  int GetWindowCount() const { return windows_.size(); }
  WindowDefinition& GetWindow(int index) const { return *windows_[index]; }

	void Load(const xml::Node& node);
	void Save(xml::Node& node, bool current) const;

	int	id;
	base::string16 title;

private:
	int NewWindowId();

	PageLayout layout_;

  typedef std::vector<WindowDefinition*> Windows;
	Windows windows_;
};

base::FilePath GetPagePath(const char* name);
base::FilePath GetPagePath(int id);
