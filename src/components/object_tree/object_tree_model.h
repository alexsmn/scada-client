#pragma once

#include "base/blinker.h"
#include "components/configuration_tree/configuration_tree_model.h"
#include "timed_data/timed_data_spec.h"

class Profile;

class ObjectTreeModel : public ConfigurationTreeModel,
                        private rt::TimedDataDelegate,
                        private Blinker {
 public:
  ObjectTreeModel(NodeRefService& node_service, scada::NodeId root_id,
                  TimedDataService& timed_data_service, Profile& profile);

  void SetNodeVisible(ConfigurationTreeNode& node, bool visible);

  const rt::TimedDataSpec* GetTimedData(void* node) const;

  // ConfigurationTreeModel
  virtual bool AreChecksVisible() const override { return true; }

  // TreeModel
  virtual int GetColumnCount() const override;
  virtual base::string16 GetColumnText(int column_id) const override;
  virtual int GetColumnPreferredSize(int column_id) const override;
  virtual base::string16 GetText(void* node, int column_id) override;
  virtual SkColor GetTextColor(void* node, int column_id) override;
  virtual SkColor GetBackgroundColor(void* node, int column_id) override;

 private:
  struct VisibleNode {
    bool fetched = false;
    rt::TimedDataSpec spec;
  };

  void ConnectVisibleNode(VisibleNode& visible_node, ConfigurationTreeNode& tree_node);

  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties) override;
  virtual void OnEventsChanged(rt::TimedDataSpec& spec,
                               const events::EventSet& events) override;

  // Blinker
  virtual void OnBlink(bool state) override;

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  TimedDataService& timed_data_service_;
  Profile& profile_;

  std::map<ConfigurationTreeNode*, VisibleNode> visible_nodes_;
};
