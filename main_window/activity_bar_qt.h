#pragma once

#include <QWidget>

#include <functional>
#include <string>
#include <vector>

class QToolButton;
class QButtonGroup;
class QIcon;

// Left activity rail — opt-in reshell chrome (backlog 1.1). A charcoal column
// of section buttons that activate the operator's primary surfaces (Overview,
// Alarms, Trends, Substations, Tables; Administration and Settings pinned at
// the bottom). The active section carries an accent marker; the Alarms section
// carries an unacknowledged-alarm count badge. Sections whose backing view does
// not exist yet are shown disabled with a tooltip, so the rail reads as the
// full navigation model without pretending every surface is ready.
class ActivityBar : public QWidget {
  Q_OBJECT

 public:
  // Dedicated rail glyph drawn for a section. Kept independent of the view
  // command icons (which are toolbar-shaped) so the rail reads as a coherent,
  // workbench-style icon set.
  enum class Icon {
    kNone,
    kOverview,
    kAlarms,
    kTrends,
    kSubstations,
    kTables,
    kAdministration,
    kSettings,
  };

  // One rail entry. `window_info_name` is the view type the section activates
  // (empty / unknown => the section is disabled). `icon_kind` selects the
  // dedicated glyph (falls back to the label's first letter when kNone).
  // `is_alarms` marks the single section that shows the unread badge;
  // `pinned_bottom` sinks the entry to the bottom group.
  struct Section {
    std::string window_info_name;
    std::u16string label;
    Icon icon_kind = Icon::kNone;
    bool enabled = true;
    bool is_alarms = false;
    bool pinned_bottom = false;
  };

  // Invoked when the user activates a section, with its `window_info_name`.
  using ActivateCallback = std::function<void(const std::string&)>;

  ActivityBar(QWidget* parent,
              std::vector<Section> sections,
              ActivateCallback on_activate);
  ~ActivityBar() override;

  // Sets the Alarms unread badge; 0 hides it.
  void SetAlarmCount(int count);

  // Marks the section backing `window_info_name` as active (accent marker).
  void SetActiveSection(const std::string& window_info_name);

 private:
  void RefreshAlarmsButton();

  struct Item {
    Section section;
    QToolButton* button = nullptr;
  };

  std::vector<Item> items_;
  ActivateCallback on_activate_;
  QButtonGroup* group_ = nullptr;
  int alarm_count_ = 0;
};
