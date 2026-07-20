#include "events/qt/severity_tile_strip.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QHBoxLayout>
#include <QLabel>

#include <utility>

namespace events {
namespace {

// The design tokens for the active theme. The strip is only built under a token
// theme, so the dark fallback is never actually used.
const scada::aui::ThemeTokens& StripTokens() {
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
}

}  // namespace

SeverityTileStrip::SeverityTileStrip(SeverityTileCountsProvider counts,
                                     QWidget* parent)
    : QWidget{parent}, counts_{std::move(counts)} {
  setObjectName(QStringLiteral("severityTileStrip"));

  auto* layout = new QHBoxLayout{this};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(6);

  Refresh();
}

void SeverityTileStrip::Refresh() {
  const SeverityTileCounts counts = counts_ ? counts_() : SeverityTileCounts{};

  // UI strings go through Translate() here — the builder stays
  // translation-agnostic.
  const std::vector<SeverityTile> tiles =
      BuildSeverityTiles(counts, Translate("Critical"), Translate("Warning"),
                         Translate("Unacknowledged"));

  // One label per tile the builder produces, so the tile set and its display
  // order stay owned by BuildSeverityTiles() rather than by this widget.
  while (tiles_.size() < tiles.size()) {
    auto* tile = new QLabel{this};
    tile->setObjectName(
        QStringLiteral("severityTile%1").arg(static_cast<int>(tiles_.size())));
    tile->setMargin(2);
    layout()->addWidget(tile);
    tiles_.push_back(tile);
  }

  const scada::aui::ThemeTokens& tokens = StripTokens();

  for (size_t i = 0; i < tiles_.size() && i < tiles.size(); ++i) {
    const SeverityTile& tile = tiles[i];
    tiles_[i]->setText(QStringLiteral("%1 %2")
                           .arg(QString::fromStdU16String(tile.caption))
                           .arg(tile.count));

    // Calm when the count is zero (muted, plain); asserted when it is not
    // (bold, severity-coloured). A tile without a severity colour —
    // unacknowledged, a workflow state — asserts in the primary text token
    // instead, so it never reads as a severity.
    if (tile.count == 0) {
      tiles_[i]->setStyleSheet(
          QStringLiteral("color:%1;").arg(tokens.fg_muted.name()));
      continue;
    }
    const QColor color = tile.color ? tile.color->qcolor() : tokens.fg;
    tiles_[i]->setStyleSheet(
        QStringLiteral("color:%1;font-weight:700;").arg(color.name()));
  }
}

SeverityTileStrip* MakeSeverityTileStrip(SeverityTileCountsProvider counts,
                                         QWidget* parent) {
  // Opt-in: the KPI tiles are reshell chrome, absent from the legacy look.
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;

  return new SeverityTileStrip{std::move(counts), parent};
}

}  // namespace events
