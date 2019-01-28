#pragma once

#include "base/blinker.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "timed_data/timed_data_spec.h"

class Profile;
class TimedDataService;

struct ObjectTreeModelContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  const NodeRef root_;
  TimedDataService& timed_data_service_;
  Profile& profile_;
};

class ObjectTreeModel : private ObjectTreeModelContext,
                        public ConfigurationTreeModel,
                        private Blinker {
 public:
  explicit ObjectTreeModel(ObjectTreeModelContext&& context);

  void SetNodeVisible(ConfigurationTreeNode& node, bool visible);

  const TimedDataSpec* GetTimedData(void* node) const;

  // TreeModel
  virtual int GetColumnCount() const override;
  virtual base::string16 GetColumnText(int column_id) const override;
  virtual int GetColumnPreferredSize(int column_id) const override;
  virtual base::string16 GetText(void* node, int column_id) override;
  virtual SkColor GetTextColor(void* node, int column_id) override;
  virtual SkColor GetBackgroundColor(void* node, int column_id) override;

  // Blinker
  virtual void OnBlink(bool state) override;

  typedef std::map<ConfigurationTreeNode*, TimedDataSpec> NodeDataMap;
  NodeDataMap visible_nodes_data_;
};
