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
                   BlinkerManager& blinker_manager,
                   NodeChangeHandler node_change_handler);

  void SetNodeVisible(ConfigurationTreeNode& tree_node, bool visible);

  std::wstring GetText(void* tree_node);
  SkColor GetTextColor(void* tree_node);
  SkColor GetBackgroundColor(void* tree_node);

 private:
  class NodeData {
   public:
    using ChangeHandler = std::function<void()>;

    virtual ~NodeData() = default;

    virtual std::wstring GetText() const = 0;
    virtual bool IsBad() const { return false; }
    virtual bool IsAlerting() const { return false; }
  };

  class NodeDataImpl final : public NodeData {
   public:
    using ChangeHandler = std::function<void()>;

    NodeDataImpl(TimedDataService& timed_data_service,
                 const NodeRef& node,
                 ChangeHandler change_handler);

    // NodeData
    virtual std::wstring GetText() const override;
    virtual bool IsBad() const override;
    virtual bool IsAlerting() const override;

   private:
    TimedDataSpec spec_;
  };

  std::unique_ptr<NodeData> CreateNodeData(ConfigurationTreeNode& tree_node);

  const NodeData* GetNodeData(void* tree_node) const;

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  using NodeDataMap =
      std::map<ConfigurationTreeNode*, std::unique_ptr<NodeData>>;
  NodeDataMap visible_nodes_data_;
};
