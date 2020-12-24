#pragma once

#include "base/blinker.h"
#include "controls/color.h"
#include "timed_data/timed_data_spec.h"

class ConfigurationTreeNode;
class Profile;
class TimedDataService;
class VisibleNodeModel;

class VisibleNode {
 public:
  virtual ~VisibleNode() = default;

  virtual std::wstring GetText() const = 0;
  virtual bool IsBad() const { return false; }
  virtual bool IsAlerting() const { return false; }

 protected:
  void NotifyChanged();

 private:
  using ChangeHandler = std::function<void()>;
  ChangeHandler change_handler_;

  friend class VisibleNodeModel;
};

class ObjectVisibleNode final : public VisibleNode {
 public:
  ObjectVisibleNode(TimedDataService& timed_data_service, const NodeRef& node);

  // VisibleNode
  virtual std::wstring GetText() const override;
  virtual bool IsBad() const override;
  virtual bool IsAlerting() const override;

 private:
  TimedDataSpec spec_;
};

class VisibleNodeModel : private Blinker {
 public:
  using NodeChangeHandler = std::function<void(void* tree_node)>;

  VisibleNodeModel(TimedDataService& timed_data_service,
                   Profile& profile,
                   BlinkerManager& blinker_manager,
                   NodeChangeHandler node_change_handler);

  void SetNode(void* tree_node, std::unique_ptr<VisibleNode> node);

  std::wstring GetText(void* tree_node);
  SkColor GetTextColor(void* tree_node);
  SkColor GetBackgroundColor(void* tree_node);

 private:
  const VisibleNode* GetNode(void* tree_node) const;

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  std::map<void*, std::unique_ptr<VisibleNode>> nodes_;
};
