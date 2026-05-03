#pragma once

#include "base/blinker.h"
#include "aui/color.h"
#include "node_service/node_observer.h"
#include "timed_data/timed_data_spec.h"

class BlinkerManager;
class ConfigurationTreeNode;
class DeviceStateNotifier;
class Profile;
class TimedDataService;
class VisibleNodeModel;

class VisibleNode {
 public:
  virtual ~VisibleNode();

  using ChangeHandler = std::function<void()>;
  void SetChangeHandler(ChangeHandler change_handler);

  virtual std::u16string GetText() const = 0;
  virtual bool IsBad() const { return false; }
  virtual bool IsAlerting() const { return false; }

 protected:
  void NotifyChanged();

 private:
  ChangeHandler change_handler_;

  friend class VisibleNodeModel;
};

class ProxyVisibleNode final
    : public VisibleNode,
      public std::enable_shared_from_this<ProxyVisibleNode> {
 public:
  ~ProxyVisibleNode();

  void SetUnderlyingNode(std::shared_ptr<VisibleNode> node);

  // VisibleNode
  virtual std::u16string GetText() const override;
  virtual bool IsBad() const override;
  virtual bool IsAlerting() const override;

 private:
  std::shared_ptr<VisibleNode> underlying_node_;
};

class DataItemVisibleNode final : private Blinker, public VisibleNode {
 public:
  DataItemVisibleNode(TimedDataService& timed_data_service,
                      BlinkerManager& blinker_manager,
                      NodeRef node);

  // VisibleNode
  virtual std::u16string GetText() const override;
  virtual bool IsBad() const override;
  virtual bool IsAlerting() const override;

 private:
  void SetAlerting(bool alerting);

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataSpec spec_;

  bool alerting_ = false;
};

class DataGroupVisibleNode final : public VisibleNode, private NodeRefObserver {
 public:
  DataGroupVisibleNode(TimedDataService& timed_data_service, NodeRef node);
  ~DataGroupVisibleNode();

  // VisibleNode
  virtual std::u16string GetText() const override;
  virtual bool IsBad() const override;

 private:
  void UpdateDevice();

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  TimedDataService& timed_data_service_;
  const NodeRef node_;

  NodeRef device_;
  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;
};

class VisibleNodeModel {
 public:
  using NodeChangeHandler = std::function<void(void* tree_node)>;

  VisibleNodeModel(TimedDataService& timed_data_service,
                   Profile& profile,
                   NodeChangeHandler node_change_handler);
  ~VisibleNodeModel();

  void SetNode(void* tree_node, std::shared_ptr<VisibleNode> node);
  bool HasNode(void* tree_node, const std::shared_ptr<VisibleNode>& node) const;

  std::u16string GetText(void* tree_node);
  aui::Color GetTextColor(void* tree_node);
  aui::Color GetBackgroundColor(void* tree_node);

 private:
  const VisibleNode* GetNode(void* tree_node) const;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  std::map<void*, std::shared_ptr<VisibleNode>> nodes_;
};
