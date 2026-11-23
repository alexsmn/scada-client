#include "graph/trend_pens.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

// Hue in degrees beside each pen; the gaps are the reserved hues (red/orange/
// amber/yellow 330°–70°, quality green ≈158°, low-severity blue ≈210°).
// Contrast is against #171717 / #ffffff / #000000.
const std::array<QColor, 6> kTrendPens = {
    QColor{0x16, 0x92, 0x9f},  // teal    186°  4.82 / 3.72 / 5.65
    QColor{0x94, 0x66, 0xd8},  // violet  264°  4.40 / 4.08 / 5.15
    QColor{0x67, 0x91, 0x24},  // olive    83°  4.83 / 3.72 / 5.65
    QColor{0xb0, 0x48, 0xc0},  // purple  292°  3.87 / 4.63 / 4.53
    QColor{0x66, 0x72, 0xe0},  // indigo  234°  4.31 / 4.16 / 5.05
    QColor{0x78, 0x80, 0x8a},  // slate   213°  4.48 / 4.00 / 5.25 (grey)
};

double RelativeLuminance(const QColor& color) {
  auto channel = [](int value) {
    const double c = value / 255.0;
    return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * channel(color.red()) + 0.7152 * channel(color.green()) +
         0.0722 * channel(color.blue());
}

double ContrastRatio(const QColor& a, const QColor& b) {
  const double la = RelativeLuminance(a);
  const double lb = RelativeLuminance(b);
  return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

bool IsUsed(const QColor& color, std::span<const QColor> used) {
  return std::ranges::any_of(used, [&color](const QColor& other) {
    return other.rgba() == color.rgba();
  });
}

}  // namespace

std::span<const QColor> TrendPenSequence() {
  return kTrendPens;
}

bool IsReadablePenOnBackground(const QColor& color, const QColor& background) {
  // Below WCAG's 3:1 on purpose: this gates an operator's own colour too, and
  // refusing a merely dim choice would override them for little gain.
  constexpr double kMinimumLineContrast = 2.2;
  return color.alpha() != 0 &&
         ContrastRatio(color, background) >= kMinimumLineContrast;
}

QColor ChooseNewPenColor(const QColor& preferred,
                         const QColor& background,
                         std::span<const QColor> used) {
  if (preferred.isValid() && IsReadablePenOnBackground(preferred, background) &&
      !IsUsed(preferred, used)) {
    return preferred;
  }

  for (const QColor& pen : kTrendPens) {
    if (IsReadablePenOnBackground(pen, background) && !IsUsed(pen, used))
      return pen;
  }

  // Every pen is taken: repeat the sequence in order, so the seventh series
  // gets the first pen again rather than whatever is left over.
  const size_t start = used.size() % kTrendPens.size();
  for (size_t i = 0; i < kTrendPens.size(); ++i) {
    const QColor& pen = kTrendPens[(start + i) % kTrendPens.size()];
    if (IsReadablePenOnBackground(pen, background))
      return pen;
  }

  // A canvas no pen is readable on (an operator-chosen mid grey).
  return kTrendPens[start];
}
