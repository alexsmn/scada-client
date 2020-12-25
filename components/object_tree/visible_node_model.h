#pragma once

#include "base/blinker.h"
#include "controls/color.h"
#include "timed_data/timed_data_spec.h"

class BlinkerManager;
class ConfigurationTreeNode;
class Profile;
class TimedDataService;
class VisibleNodeModel;

class VisibleNode {
 public:
  virtual ~VisibleNode();

  using ChangeHandler = std::function<void()>;
  void SetChangeHandler(ChangeHandler change_handler);

  virtual std::wstring GetText() const = 0;
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
  virtual std::wstring GetText() const override;
  virtual bool IsBad() const override;
  virtual bool IsAlerting() const override;

 private:
  std::shared_ptr<VisibleNode> underlying_node_;
};

class ObjectVisibleNode final : private Blinker, public VisibleNode {
 public:
  ObjectVisibleNode(TimedDataService& timed_data_service,
                    BlinkerManager& blinker_manager,
                    NodeRef node);

  // VisibleNode
  virtual std::wstring GetText() const override;
  virtual bool IsBad() const override;
  virtual bool IsAlerting() const override;

 private:
  void SetAlerting(bool alerting);

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataSpec spec_;

  bool alerting_ = false;
};

class VisibleNodeModel {
 public:
  using NodeChangeHandler = std::function<void(void* tree_node)>;

  VisibleNodeModel(TimedDataService& timed_data_service,
                   Profile& profile,
                   NodeChangeHandler node_change_handler);
  ~VisibleNodeModel();

  void SetNode(void* tree_node, std::shared_ptr<VisibleNode> node);

  std::wstring GetText(void* tree_node);
  SkColor GetTextColor(void* tree_node);
  SkColor GetBackgroundColor(void* tree_node);

 private:
  const VisibleNode* GetNode(void* tree_node) const;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  std::map<void*, std::shared_ptr<VisibleNode>> nodes_;
};
