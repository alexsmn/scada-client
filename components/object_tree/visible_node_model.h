#pragma once

#include "base/blinker.h"
#include "controls/color.h"
#include "timed_data/timed_data_spec.h"

class ConfigurationTreeNode;
class Profile;
class TimedDataService;

class VisibleNodeModel : private Blinker {
 public:
  using NodeChangeHandler = std::function<void(void* tree_node)>;

  VisibleNodeModel(TimedDataService& timed_data_service,
                   Profile& profile,
                   NodeChangeHandler node_change_handler);

  void SetNodeVisible(ConfigurationTreeNode& tree_node, bool visible);

  std::wstring GetText(void* tree_node);
  SkColor GetTextColor(void* tree_node);
  SkColor GetBackgroundColor(void* tree_node);

 private:
  TimedDataSpec MakeTimedDataSpec(ConfigurationTreeNode& tree_node);

  const TimedDataSpec* GetTimedData(void* tree_node) const;

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  typedef std::map<ConfigurationTreeNode*, TimedDataSpec> NodeDataMap;
  NodeDataMap visible_nodes_data_;
};
