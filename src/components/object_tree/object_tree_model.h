#pragma once

#include "client/base/blinker.h"
#include "client/components/configuration_tree/configuration_tree_model.h"
#include "common/timed_data/timed_data_spec.h"

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

  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties) override;
  virtual void OnEventsChanged(rt::TimedDataSpec& spec,
                               const events::EventSet& events) override;

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataService& timed_data_service_;
  Profile& profile_;

  typedef std::map<ConfigurationTreeNode*, rt::TimedDataSpec> NodeDataMap;
  NodeDataMap visible_nodes_data_;
};
