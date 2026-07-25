#pragma once

#include "aui/color.h"
#include "base/blinker.h"
#include "timed_data/timed_data_spec.h"

#include <boost/signals2/connection.hpp>
#include <optional>

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
  // Whether this node speaks for a real live value yet. A node that is still
  // resolving reports no quality at all rather than borrowing the good band —
  // "not bad" must never be allowed to read as "good" when nothing has been
  // delivered.
  virtual bool IsResolved() const { return true; }
  // Whether this node carries a data quality of its own — i.e. it stands for a
  // data variable's live value. Only such nodes get a status dot. A group
  // (folder) shows its device's connection state as *text* in the Value
  // column; that is not a value quality, and painting a good/bad dot on the
  // folder icon would read as a quality claim about the folder itself.
  virtual bool HasQuality() const { return false; }

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
  virtual bool IsResolved() const override;
  virtual bool HasQuality() const override;

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
  virtual bool IsResolved() const override;
  virtual bool HasQuality() const override { return true; }

 private:
  void SetAlerting(bool alerting);

  // Blinker
  virtual void OnBlink(bool state) override;

  TimedDataSpec spec_;

  bool alerting_ = false;
};

class DataGroupVisibleNode final : public VisibleNode {
 public:
  DataGroupVisibleNode(TimedDataService& timed_data_service, NodeRef node);
  ~DataGroupVisibleNode();

  // VisibleNode
  virtual std::u16string GetText() const override;
  virtual bool IsBad() const override;
  // No override of HasQuality: a group has no value quality, so it gets no
  // status dot. Its device state still colours the Value text via IsBad.

 private:
  void UpdateDevice();

  void OnModelChanged(const scada::ModelChangeEvent& event);

  TimedDataService& timed_data_service_;
  const NodeRef node_;

  NodeRef device_;
  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;

  boost::signals2::scoped_connection model_changed_connection_;
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
  scada::aui::Color GetTextColor(void* tree_node);
  scada::aui::Color GetBackgroundColor(void* tree_node);

  // Quality status-dot colour for a data variable's live value
  // (good/uncertain/bad), or none when the node carries no value quality —
  // folders/objects and groups, whose Value column shows a device state rather
  // than a value.
  std::optional<scada::aui::Color> GetStatusColor(void* tree_node);

 private:
  const VisibleNode* GetNode(void* tree_node) const;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  std::map<void*, std::shared_ptr<VisibleNode>> nodes_;
};
