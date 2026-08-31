#include "device_diagnostics/qt/device_diagnostics_panel.h"

#include "base/format_time.h"

#include "base/time_utils.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "scada/data_value.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"
#include "timed_data/timed_data_spec.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <array>
#include <cmath>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace {

// The design tokens for the active theme. The panel is only built under
// a token theme (the factory gates on it), so the legacy fallback is harmless.
const scada::aui::ThemeTokens& PanelTokens() {
  return scada::aui::ActiveThemeTokens();
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// True when a diagnostic boolean reads as set. The live server sends a proper
// Boolean Variant; the headless fixture stores the flag as a number, so accept
// a non-zero integer/double too.
bool VariantIsTruthy(const scada::Variant& value) {
  if (bool flag; value.get(flag))
    return flag;
  if (scada::Int32 i; value.get(i))
    return i != 0;
  if (double d; value.get(d))
    return d != 0.0;
  return false;
}

// The reading's current value: the live monitored value when it has been
// delivered, otherwise the node's Value-attribute snapshot. The offscreen
// capture (and the moment right after selection, before the first monitored
// update) has no delivered value, so the snapshot keeps the hero + counters
// from reading empty/"Disabled".
scada::Variant CurrentValue(const TimedDataSpec* spec, const NodeRef& node) {
  if (spec) {
    scada::DataValue current = spec->current();
    if (!current.value.is_null())
      return current.value;
  }
  return node ? node.value() : scada::Variant{};
}

// Formats an integral diagnostic counter with the mockup's space grouping
// (128 402). Empty for a missing value.
QString FormatCount(const scada::Variant& value) {
  if (value.is_null())
    return {};
  const long long n = std::llround(value.get_or<double>(0.0));
  QString digits = QString::number(n < 0 ? -n : n);
  for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
    digits.insert(i, QChar(' '));
  return n < 0 ? QStringLiteral("-") + digits : digits;
}

// The hero band's {status text, colour} for the active theme.
std::pair<QString, QColor> BandStatus(DeviceLinkBand band,
                                      const scada::aui::ThemeTokens& tokens) {
  switch (band) {
    case DeviceLinkBand::kUp:
      return {Tr("Link up"), tokens.good};
    case DeviceLinkBand::kDown:
      return {Tr("Link down"), tokens.bad};
    case DeviceLinkBand::kDisabled:
      return {Tr("Disabled"), tokens.fg_subtle};
  }
  return {Tr("Disabled"), tokens.fg_subtle};
}

QLabel* SectionHeader(const QString& text,
                      const scada::aui::ThemeTokens& tokens) {
  auto* label = new QLabel{text};
  label->setStyleSheet(
      QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                     "text-transform:uppercase;letter-spacing:.5px;")
          .arg(tokens.fg_subtle.name()));
  return label;
}

// A device diagnostic variable to surface as a live counter row.
struct DiagnosticDescriptor {
  scada::NodeId declaration_id;  // resolves the instance child in the live app.
  std::string_view browse_name;  // fallback match (headless fixture children).
  std::string_view label;        // English label, translated at render time.
};

const std::array<DiagnosticDescriptor, 6>& CounterDescriptors() {
  static const std::array<DiagnosticDescriptor, 6> descriptors{{
      {scada::devices::id::DeviceType_MessagesIn, "MessagesIn", "Messages RX"},
      {scada::devices::id::DeviceType_MessagesOut, "MessagesOut",
       "Messages TX"},
      {scada::devices::id::DeviceType_BytesIn, "BytesIn", "Bytes RX"},
      {scada::devices::id::DeviceType_BytesOut, "BytesOut", "Bytes TX"},
      {scada::devices::id::DeviceType_InterrogateCount, "InterrogateCount",
       "Interrogations"},
      {scada::devices::id::DeviceType_SyncClockCount, "SyncClockCount",
       "Clock syncs"},
  }};
  return descriptors;
}

// Renders a reading by its declared shape. Falls through to FormatCount --
// including its em-dash for an absent value -- so an unmeasured round trip and
// a link that has never connected read as "no reading" rather than 0 ms and
// 1970.
QString FormatShaped(const scada::Variant& value, ProtocolValueShape shape) {
  switch (shape) {
    case ProtocolValueShape::kState:
      if (scada::Int32 state; value.get(state))
        return Tr(Iec60870LinkStateLabel(state));
      return {};
    case ProtocolValueShape::kFlag:
      // A t1 flag reads as a condition. "true" makes an operator remember which
      // boolean means trouble.
      if (bool flag; value.get(flag))
        return flag ? Tr("expired") : Tr("ok");
      return {};
    case ProtocolValueShape::kDuration:
      if (double ms; value.get(ms))
        return QStringLiteral("%1 ms").arg(std::llround(ms));
      return {};
    case ProtocolValueShape::kTime:
      // The zero instant is not a reading: a link that has never connected has
      // no last-connected time, and rendering the epoch would show 1970.
      if (scada::Time stamp; value.get(stamp) && stamp != scada::Time{}) {
        // The client's own format, not Qt::TextDate. Qt's renders the C locale
        // ("Thu Apr 16 14:48:12 2026") in the middle of an otherwise Russian
        // panel, and disagrees with every other timestamp the operator sees —
        // the event journal, the Inspector's alarm card and this panel's own
        // device log all go through FormatTime.
        return QString::fromStdString(
            FormatTime(stamp, TIME_FORMAT_DATE | TIME_FORMAT_TIME));
      }
      return {};
    case ProtocolValueShape::kCount:
      break;
  }
  return FormatCount(value);
}

// Resolves a device's diagnostic child. The live app models these as aggregate
// (HasComponent) variables, resolved by the type declaration; the headless
// fixture attaches them as plain children, matched by browse name.
NodeRef ResolveDiagnosticChild(const NodeRef& device,
                               const scada::NodeId& declaration_id,
                               std::string_view browse_name) {
  if (NodeRef child = device[declaration_id])
    return child;
  for (const NodeRef& child : device.targets()) {
    if (child.browse_name().name() == browse_name)
      return child;
  }
  return {};
}

// Removes and deletes every widget currently in `layout`.
void ClearLayout(QVBoxLayout* layout) {
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (QWidget* widget = item->widget())
      widget->deleteLater();
    delete item;
  }
}

}  // namespace

DeviceDiagnosticsPanel::DeviceDiagnosticsPanel(
    DeviceDiagnosticsPanelContext context,
    QWidget* parent)
    : QWidget{parent}, context_{std::move(context)} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("deviceDiagnosticsPanel"));
  setStyleSheet(QStringLiteral("#deviceDiagnosticsPanel{background:%1;}")
                    .arg(tokens.bg_elevated.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);

  stack_ = new QStackedWidget{this};
  stack_->addWidget(BuildEmptyState());  // index 0
  stack_->addWidget(BuildContent());     // index 1
  root->addWidget(stack_);

  Clear();
}

DeviceDiagnosticsPanel::~DeviceDiagnosticsPanel() = default;

QWidget* DeviceDiagnosticsPanel::BuildEmptyState() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* empty = new QWidget;
  auto* layout = new QVBoxLayout{empty};
  layout->setAlignment(Qt::AlignCenter);
  auto* label = new QLabel{Tr("Select a device to see its diagnostics")};
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet(
      QStringLiteral("color:%1;padding:24px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(label);
  return empty;
}

QWidget* DeviceDiagnosticsPanel::BuildContent() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* view = new QWidget;
  auto* layout = new QVBoxLayout{view};
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(12);

  // Header: device name + type subtitle.
  name_ = new QLabel;
  name_->setWordWrap(true);
  name_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  type_ = new QLabel;
  type_->setWordWrap(true);
  type_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(name_);
  layout->addWidget(type_);

  // Link-status hero.
  hero_ = new QFrame;
  hero_->setObjectName(QStringLiteral("diagnosticsHero"));
  auto* hero_layout = new QVBoxLayout{hero_};
  hero_layout->setContentsMargins(14, 11, 14, 11);
  hero_layout->setSpacing(2);
  hero_status_ = new QLabel;
  hero_status_->setObjectName(QStringLiteral("diagnosticsHeroStatus"));
  hero_detail_ = new QLabel;
  hero_detail_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));
  hero_layout->addWidget(hero_status_);
  hero_layout->addWidget(hero_detail_);
  layout->addWidget(hero_);

  // Counters section (its rows are (re)built per device).
  layout->addWidget(SectionHeader(Tr("Counters"), tokens));
  auto* rows_host = new QWidget;
  rows_layout_ = new QVBoxLayout{rows_host};
  rows_layout_->setContentsMargins(0, 0, 0, 0);
  rows_layout_->setSpacing(0);
  layout->addWidget(rows_host);

  // Actions section. Its buttons are (re)built per selection, because the
  // protocol's link action exists only for some devices.
  layout->addWidget(SectionHeader(Tr("Actions"), tokens));
  auto* actions_host = new QWidget;
  actions_layout_ = new QVBoxLayout{actions_host};
  actions_layout_->setContentsMargins(0, 0, 0, 0);
  actions_layout_->setSpacing(6);
  layout->addWidget(actions_host);

  layout->addStretch(1);
  return view;
}

void DeviceDiagnosticsPanel::RebuildActionButtons() {
  if (!actions_layout_)
    return;

  const scada::aui::ThemeTokens& tokens = PanelTokens();
  ClearLayout(actions_layout_);
  action_widgets_.clear();

  const QString action_style =
      QStringLiteral(
          "QPushButton{background:%1;color:%2;border:1px solid %3;"
          "border-radius:6px;padding:8px;font-weight:500;}"
          "QPushButton:disabled{color:%4;}")
          .arg(tokens.surface_muted.name(), tokens.fg.name(),
               tokens.border_strong.name(), tokens.fg_subtle.name());

  auto add = [&](const DiagnosticAction& action) {
    auto* button = new QPushButton{QString::fromStdU16String(action.label)};
    button->setStyleSheet(action_style);
    connect(button, &QPushButton::clicked, this, [execute = action.execute] {
      if (execute)
        execute();
    });
    actions_layout_->addWidget(button);

    QLabel* reason = nullptr;
    if (!action.disabled_reason.empty()) {
      reason = new QLabel{QString::fromStdU16String(action.disabled_reason)};
      reason->setWordWrap(true);
      reason->setStyleSheet(QStringLiteral("color:%1;font-size:10.5px;")
                                .arg(tokens.fg_subtle.name()));
      reason->setVisible(false);
      actions_layout_->addWidget(reason);
    }

    action_widgets_.push_back(ActionWidgets{button, reason});
  };

  for (const DiagnosticAction& action : link_actions_)
    add(action);
  for (const DiagnosticAction& action : context_.actions)
    add(action);
}

std::optional<DiagnosticAction> DeviceDiagnosticsPanel::MakeLinkAction(
    const ProtocolLinkAction& action,
    const NodeRef& link) {
  if (action.label.empty() || !link)
    return std::nullopt;

  // No call path wired means the panel would be drawing a button that could
  // never do anything, with no reason an operator could act on. Omit it.
  if (!context_.call_link_method)
    return std::nullopt;

  return DiagnosticAction{
      .label = Translate(action.label),
      .execute =
          [this, link, method_id = action.method_id] {
            context_.call_link_method(link, method_id);
          },
      // Mirrors the method node's UserExecutable (OPC UA Part 3 §5.7.1): the
      // server computes the same predicate from the same session rights and
      // stays the authority, so this only decides whether the operator is
      // offered a live button or an explained one.
      .is_enabled =
          [this] { return !context_.can_call || context_.can_call(); },
      .disabled_reason = Translate(action.denied_reason)};
}

void DeviceDiagnosticsPanel::Clear() {
  online_spec_.reset();
  enabled_spec_.reset();
  readings_.clear();
  link_actions_.clear();
  RebuildActionButtons();
  if (stack_)
    stack_->setCurrentIndex(0);
}

void DeviceDiagnosticsPanel::ShowDevice(const NodeRef& device,
                                        TimedDataService& timed_data_service) {
  online_spec_.reset();
  enabled_spec_.reset();
  online_node_ = {};
  enabled_node_ = {};
  readings_.clear();
  link_readings_.clear();
  link_section_label_.clear();
  link_actions_.clear();

  if (!device) {
    Clear();
    return;
  }

  // Ask for what the rows are read from. The counters usually resolve without
  // it — the tree has browsed the device by the time it can be selected — but
  // the link section is decided by the parent's *type* browse name, which a
  // selection never makes resident, so an unasked-for device silently loses its
  // link rows and its Reconnect action. Guarded by the id so the redraw
  // re-enters once, and so a reply for a device the operator has moved off is
  // dropped rather than repainting another device's counters.
  if (context_.load && loading_id_ != device.node_id()) {
    loading_id_ = device.node_id();
    const QPointer<DeviceDiagnosticsPanel> alive{this};
    // The service is captured by pointer and outlives the panel: the shell owns
    // both, and every live spec this panel holds already refers to it.
    context_.load(device, [this, alive, device, service = &timed_data_service] {
      if (alive && loading_id_ == device.node_id())
        ShowDevice(device, *service);
    });
  }

  device_name_ = QString::fromStdU16String(ToString16(device.display_name()));
  if (NodeRef type = device.type_definition())
    device_type_ = QString::fromStdU16String(ToString16(type.display_name()));
  else
    device_type_.clear();

  // Resolves a diagnostic child, records it for the Value snapshot, and returns
  // a live spec for it (null when the device omits the variable).
  auto connect = [&](const scada::NodeId& declaration_id,
                     std::string_view browse_name,
                     NodeRef& node_out) -> std::unique_ptr<TimedDataSpec> {
    NodeRef child = ResolveDiagnosticChild(device, declaration_id, browse_name);
    if (!child)
      return nullptr;
    node_out = child;
    auto spec =
        std::make_unique<TimedDataSpec>(timed_data_service, child.node_id());
    spec->SetCurrentOnly();
    // The live current value — the only thing a SetCurrentOnly spec delivers —
    // arrives as a PROPERTY_CURRENT change through property_change_handler, not
    // as a buffer update; wiring only update_handler leaves the panel frozen
    // at whatever it showed when the device was selected, since a current-only
    // spec produces no buffer updates.
    spec->update_handler = [this](std::span<const scada::DataValue>) {
      RefreshFromSpecs();
    };
    spec->property_change_handler = [this](const PropertySet&) {
      RefreshFromSpecs();
    };
    return spec;
  };

  // The booleans that drive the hero band.
  online_spec_ =
      connect(scada::devices::id::DeviceType_Online, "Online", online_node_);
  enabled_spec_ =
      connect(scada::devices::id::DeviceType_Enabled, "Enabled", enabled_node_);

  // The live counter readings.
  for (const DiagnosticDescriptor& descriptor : CounterDescriptors()) {
    NodeRef node;
    std::unique_ptr<TimedDataSpec> spec =
        connect(descriptor.declaration_id, descriptor.browse_name, node);
    if (!node)
      continue;
    readings_.push_back(Reading{Tr(descriptor.label), node, std::move(spec)});
  }

  // The device's PARENT LINK, when its protocol is registered (ADR 0007). In
  // -104 the APCI belongs to the TCP connection and several devices share one,
  // so these rows are the link's and are labelled as such.
  link_readings_.clear();
  link_section_label_.clear();
  if (NodeRef type = device.type_definition()) {
    if (const ProtocolDiagnostics* protocol =
            ProtocolDiagnosticsFor(type.browse_name().name())) {
      NodeRef link = device.parent();
      // The parent must actually BE a link. A flat model parents devices onto
      // the Devices folder, and reading link members off a folder finds none --
      // which would draw the section entirely as em-dashes instead of omitting
      // it.
      NodeRef link_type = link ? link.type_definition() : NodeRef{};
      if (link_type && link_type.browse_name().name() == protocol->link_type) {
        link_section_label_ = Tr(protocol->section_label);
        for (const ProtocolField& field : protocol->link_fields) {
          NodeRef node = ResolveDiagnosticChild(link, field.declaration_id,
                                                field.browse_name);
          if (!node)
            continue;
          auto spec = std::make_unique<TimedDataSpec>(timed_data_service,
                                                      node.node_id());
          spec->SetCurrentOnly();
          spec->update_handler = [this](std::span<const scada::DataValue>) {
            RefreshFromSpecs();
          };
          spec->property_change_handler = [this](const PropertySet&) {
            RefreshFromSpecs();
          };
          link_readings_.push_back(
              Reading{Tr(field.label), node, std::move(spec), field.shape});
        }

        // The link's action rides on the same resolved link, and only on it:
        // the method acts on the TCP connection, so without a link there is
        // nothing to reconnect and the button is omitted rather than drawn
        // dead.
        if (std::optional<DiagnosticAction> action =
                MakeLinkAction(protocol->link_action, link)) {
          link_actions_.push_back(*std::move(action));
        }
      }
    }
  }

  RebuildActionButtons();
  RefreshFromSpecs();
}

void DeviceDiagnosticsPanel::RefreshFromSpecs() {
  // A device without an Enabled variable defaults to enabled, so it still shows
  // up/down rather than a spurious "Disabled".
  const bool enabled =
      enabled_node_
          ? VariantIsTruthy(CurrentValue(enabled_spec_.get(), enabled_node_))
          : true;
  const bool online =
      VariantIsTruthy(CurrentValue(online_spec_.get(), online_node_));
  const DeviceLinkBand band = DeviceLinkBandFor(enabled, online);

  QString detail;
  if (band == DeviceLinkBand::kDown)
    detail = Tr("no response from device");
  else if (band == DeviceLinkBand::kDisabled)
    detail = Tr("device is not polled");

  std::vector<DeviceDiagnosticRow> rows;
  rows.reserve(readings_.size() + link_readings_.size() + 1);
  if (!link_readings_.empty()) {
    rows.push_back(DeviceDiagnosticRow{link_section_label_,
                                       {},
                                       /*bad=*/false,
                                       /*heading=*/true});
    for (const Reading& reading : link_readings_) {
      QString value = FormatShaped(
          CurrentValue(reading.spec.get(), reading.node), reading.shape);
      rows.push_back(DeviceDiagnosticRow{reading.label, value, /*bad=*/false});
    }
  }
  for (const Reading& reading : readings_) {
    QString value = FormatCount(CurrentValue(reading.spec.get(), reading.node));
    rows.push_back(DeviceDiagnosticRow{reading.label, value, /*bad=*/false});
  }

  ShowDiagnostics(device_name_, device_type_, band, detail, rows);
}

void DeviceDiagnosticsPanel::RefreshActions() {
  // Parallel to how RebuildActionButtons laid them out: link actions first,
  // then the host's device-wide ones.
  auto action_at = [this](size_t i) -> const DiagnosticAction& {
    return i < link_actions_.size()
               ? link_actions_[i]
               : context_.actions[i - link_actions_.size()];
  };

  for (size_t i = 0; i < action_widgets_.size(); ++i) {
    const DiagnosticAction& action = action_at(i);
    const bool enabled = !action.is_enabled || action.is_enabled();
    action_widgets_[i].button->setEnabled(enabled);
    // The reason appears only while the control is dead, which is the only
    // time an operator needs to be told why.
    if (action_widgets_[i].reason)
      action_widgets_[i].reason->setVisible(!enabled);
  }
}

void DeviceDiagnosticsPanel::ShowDiagnostics(
    const QString& name,
    const QString& type_label,
    DeviceLinkBand band,
    const QString& band_detail,
    const std::vector<DeviceDiagnosticRow>& rows) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();

  name_->setText(name);
  type_->setText(type_label);
  type_->setVisible(!type_label.isEmpty());

  const auto [status_text, band_color] = BandStatus(band, tokens);
  hero_->setStyleSheet(
      QStringLiteral("#diagnosticsHero{background:%1;border:1px solid %2;"
                     "border-radius:6px;}")
          .arg(tokens.surface_muted.name(), band_color.name()));
  hero_status_->setText(status_text);
  hero_status_->setStyleSheet(
      QStringLiteral("#diagnosticsHeroStatus{color:%1;font-size:15px;"
                     "font-weight:700;}")
          .arg(band_color.name()));
  hero_detail_->setText(band_detail);
  hero_detail_->setVisible(!band_detail.isEmpty());

  ClearLayout(rows_layout_);
  for (const DeviceDiagnosticRow& row : rows) {
    if (row.heading) {
      auto* caption = new QLabel{row.label};
      caption->setStyleSheet(
          QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                         "text-transform:uppercase;margin-top:6px;")
              .arg(tokens.fg_subtle.name()));
      rows_layout_->addWidget(caption);
      continue;
    }
    auto* row_widget = new QWidget;
    auto* row_layout = new QHBoxLayout{row_widget};
    row_layout->setContentsMargins(0, 3, 0, 3);
    auto* key = new QLabel{row.label};
    key->setStyleSheet(
        QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
    auto* value =
        new QLabel{row.value.isEmpty() ? QStringLiteral("—") : row.value};
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value->setStyleSheet(QStringLiteral("color:%1;font-weight:600;")
                             .arg((row.bad ? tokens.bad : tokens.fg).name()));
    row_layout->addWidget(key);
    row_layout->addStretch(1);
    row_layout->addWidget(value);
    rows_layout_->addWidget(row_widget);
  }

  RefreshActions();

  stack_->setCurrentIndex(1);
}

DeviceDiagnosticsPanel* MakeDeviceDiagnosticsPanel(
    DeviceDiagnosticsPanelContext context) {
  return new DeviceDiagnosticsPanel(std::move(context));
}
