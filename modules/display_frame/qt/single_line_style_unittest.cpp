#include "display_frame/qt/single_line_style.h"

#include "aui/qt/theme_qt.h"

#include <gtest/gtest.h>

namespace {

const scada::aui::ThemeTokens& Tokens() {
  return scada::aui::GetThemeTokens(scada::aui::Theme::kDark);
}

TEST(SingleLineStyleTest, ClosedIsClosedTokenAndFilled) {
  const auto& t = Tokens();
  EXPECT_EQ(SwitchStyle(t, SwitchState::kClosed, SignalQuality::kGood),
            (SwitchSymbolStyle{t.sl_closed, DeviceShape::kFilled}));
}

TEST(SingleLineStyleTest, OpenIsOpenTokenAndHollow) {
  const auto& t = Tokens();
  EXPECT_EQ(SwitchStyle(t, SwitchState::kOpen, SignalQuality::kGood),
            (SwitchSymbolStyle{t.sl_open, DeviceShape::kHollow}));
}

TEST(SingleLineStyleTest, OpenIsNotAnAlarmColour) {
  // design-language.md §2: an open breaker is neutral, not an alarm.
  const auto& t = Tokens();
  const SwitchSymbolStyle open =
      SwitchStyle(t, SwitchState::kOpen, SignalQuality::kGood);
  EXPECT_NE(open.color, t.bad);
  EXPECT_NE(open.color, t.severity_critical);
  EXPECT_EQ(open.color, t.sl_open);
}

TEST(SingleLineStyleTest, ClosedAndOpenAreDistinctByShapeAndColour) {
  const auto& t = Tokens();
  const SwitchSymbolStyle closed =
      SwitchStyle(t, SwitchState::kClosed, SignalQuality::kGood);
  const SwitchSymbolStyle open =
      SwitchStyle(t, SwitchState::kOpen, SignalQuality::kGood);
  EXPECT_NE(closed.shape, open.shape);
  EXPECT_NE(closed.color, open.color);
}

TEST(SingleLineStyleTest, BadQualityOverridesStateAndShape) {
  const auto& t = Tokens();
  for (SwitchState state :
       {SwitchState::kClosed, SwitchState::kOpen, SwitchState::kIntermediate,
        SwitchState::kUnknown}) {
    const SwitchSymbolStyle bad = SwitchStyle(t, state, SignalQuality::kBad);
    EXPECT_EQ(bad.color, t.bad);
    EXPECT_EQ(bad.shape, DeviceShape::kIndeterminate);
  }
}

TEST(SingleLineStyleTest, IntermediateIsUncertainAndIndeterminate) {
  const auto& t = Tokens();
  EXPECT_EQ(SwitchStyle(t, SwitchState::kIntermediate, SignalQuality::kGood),
            (SwitchSymbolStyle{t.uncertain, DeviceShape::kIndeterminate}));
}

TEST(SingleLineStyleTest, ConductorEnergizedIsLiveAndNotAlarmRed) {
  const auto& t = Tokens();
  const QColor live =
      ConductorColor(t, Energization::kEnergized, SignalQuality::kGood);
  EXPECT_EQ(live, t.sl_live);
  // Energized must read as restrained amber, never alarm-red.
  EXPECT_NE(live, t.bad);
  EXPECT_NE(live, t.severity_critical);
}

TEST(SingleLineStyleTest, ConductorDeEnergizedIsNeutral) {
  const auto& t = Tokens();
  EXPECT_EQ(ConductorColor(t, Energization::kDeEnergized, SignalQuality::kGood),
            t.sl_energized);
}

TEST(SingleLineStyleTest, ConductorBadQualityOverrides) {
  const auto& t = Tokens();
  EXPECT_EQ(ConductorColor(t, Energization::kEnergized, SignalQuality::kBad),
            t.bad);
}

}  // namespace
