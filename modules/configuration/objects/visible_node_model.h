#pragma once

#include "aui/color.h"
#include "aui/severity_colors.h"
#include "base/blinker.h"
#include "timed_data/timed_data_spec.h"

#include <boost/signals2/connection.hpp>
#include <map>
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
  // Whether the Value text reads as bad: a data variable's bad quality band,
  // or a group's device not being online.
  virtual bool IsBad() const { return false; }
  // Whether the node has an unacknowledged alarm. Alarm state, not quality:
  // it blinks the Value cell's background and never touches the status dot.
  virtual bool IsAlerting() const { return false; }
  // The quality band of the node's live value (`DataQualityFor`, the mapping
  // the Inspector and the table share), or std::nullopt when the node carries
  // no value quality: a folder or group, whose Value column shows a device
  // state rather than a value, or a data variable that has delivered nothing
  // yet -- "not bad" must never read as "good" when nothing has arrived.
  virtual std::optional<scada::aui::Quality> GetQuality() const {
    return std::nullopt;
  }

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
  virtual std::optional<scada::aui::Quality> GetQuality() const override;

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
  virtual std::optional<scada::aui::Quality> GetQuality() const override;

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
  // No override of GetQuality: a group has no value quality, so it gets no
  // status dot. Its device state still colours the Value text via IsBad.

 private:
  void UpdateDevice();
  // Starts watching `device_`'s runtime state, fetching the node first if it
  // is not resident yet. Separate from UpdateDevice because the fetch can
  // complete later, and this is what runs on both paths.
  void WatchDeviceState();

  void OnModelChanged(const scada::ModelChangeEvent& event);

  TimedDataService& timed_data_service_;
  const NodeRef node_;

  NodeRef device_;
  std::unique_ptr<DeviceStateNotifier> device_state_notifier_;

  boost::signals2::scoped_connection model_changed_connection_;
  boost::signals2::scoped_connection device_fetched_connection_;
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
  // (good/uncertain/bad, from `VisibleNode::GetQuality`), or none when the node
  // carries no value quality — folders/objects and groups, whose Value column
  // shows a device state rather than a value. Never alarm state, which has its
  // own cue in `GetBackgroundColor`.
  std::optional<scada::aui::Color> GetStatusColor(void* tree_node);

 private:
  const VisibleNode* GetNode(void* tree_node) const;

  TimedDataService& timed_data_service_;
  Profile& profile_;
  const NodeChangeHandler node_change_handler_;

  std::map<void*, std::shared_ptr<VisibleNode>> nodes_;
};
