#include "main_window/command_field_qt.h"

#include "aui/test/app_environment.h"

#include <QAction>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>

#include <gtest/gtest.h>

namespace {

const QString kPrompt = QStringLiteral("Search tags, objects, commands…");

QKeySequence PaletteKey() {
  return QKeySequence{Qt::CTRL | Qt::Key_K};
}

void SendClick(QWidget& widget) {
  QMouseEvent event{QEvent::MouseButtonRelease,
                    QPointF{1, 1},
                    QPointF{1, 1},
                    QPointF{1, 1},
                    Qt::LeftButton,
                    Qt::NoButton,
                    Qt::NoModifier};
  QApplication::sendEvent(&widget, &event);
}

void SendKey(QWidget& widget,
             int key,
             const QString& text,
             Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
  QKeyEvent event{QEvent::KeyPress, key, modifiers, text};
  QApplication::sendEvent(&widget, &event);
}

class CommandFieldTest : public ::testing::Test {
 protected:
  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
};

// The field's whole job is to state that the palette exists, which it cannot do
// with an elided prompt. The fixed 360 px it replaced could not promise this:
// it ignored the font entirely.
TEST_F(CommandFieldTest, IsWideEnoughForThePromptTheGlyphAndTheHint) {
  CommandField field{nullptr, kPrompt, PaletteKey(), {}};

  const int prompt_width =
      QFontMetrics{field.font()}.horizontalAdvance(kPrompt);
  const int hint_width =
      QFontMetrics{field.font()}.horizontalAdvance(field.HintText());

  EXPECT_GT(field.sizeHint().width(), prompt_width + hint_width);
}

// Sizing is in font metrics, not pixels, so the field follows the OS text-size
// accessibility setting instead of clipping under it.
TEST_F(CommandFieldTest, WidthFollowsTheFontSize) {
  CommandField field{nullptr, kPrompt, PaletteKey(), {}};
  const int at_default = field.sizeHint().width();

  QFont larger = field.font();
  larger.setPointSizeF(larger.pointSizeF() * 2);
  field.setFont(larger);

  EXPECT_GT(field.sizeHint().width(), at_default);
}

// One QKeySequence produces both the binding and the hint, in the platform's
// own notation — so the hint cannot come to name a key the shortcut does not.
TEST_F(CommandFieldTest, HintIsTheShortcutInPlatformNotation) {
  CommandField field{nullptr, kPrompt, PaletteKey(), {}};

  EXPECT_FALSE(field.HintText().isEmpty());
  EXPECT_EQ(field.HintText(), PaletteKey().toString(QKeySequence::NativeText));
}

// The prompt must stop before the hint rather than running underneath it.
TEST_F(CommandFieldTest, ReservesTheHintsWidthInTheTextMargins) {
  CommandField field{nullptr, kPrompt, PaletteKey(), {}};

  const int hint_width =
      QFontMetrics{field.font()}.horizontalAdvance(field.HintText());
  EXPECT_GT(field.textMargins().right(), hint_width);
}

// A field with no shortcut draws no hint, and then must not reserve room for
// one either.
TEST_F(CommandFieldTest, WithoutAShortcutThereIsNoHint) {
  CommandField field{nullptr, kPrompt, QKeySequence{}, {}};

  EXPECT_TRUE(field.HintText().isEmpty());
  EXPECT_EQ(field.textMargins().right(), 0);
}

// The magnifier comes from the Qt resource, so a missing or misspelled .qrc
// entry would leave the field silently glyphless.
TEST_F(CommandFieldTest, CarriesALeadingSearchGlyph) {
  CommandField field{nullptr, kPrompt, PaletteKey(), {}};

  ASSERT_EQ(field.actions().size(), 1);
  EXPECT_FALSE(field.actions().first()->icon().isNull());
}

TEST_F(CommandFieldTest, ClickingActivatesWithNoSeedText) {
  int activations = 0;
  QString seeded = QStringLiteral("unset");
  CommandField field{nullptr, kPrompt, PaletteKey(), [&](const QString& text) {
                       ++activations;
                       seeded = text;
                     }};

  SendClick(field);

  EXPECT_EQ(activations, 1);
  EXPECT_TRUE(seeded.isEmpty());
}

// The glyph is a button and swallows clicks that land on it; it must mean the
// same thing as the rest of the field rather than being a dead spot.
TEST_F(CommandFieldTest, ClickingTheGlyphActivatesToo) {
  int activations = 0;
  CommandField field{nullptr, kPrompt, PaletteKey(),
                     [&](const QString&) { ++activations; }};

  ASSERT_EQ(field.actions().size(), 1);
  field.actions().first()->trigger();

  EXPECT_EQ(activations, 1);
}

TEST_F(CommandFieldTest, TypingSeedsTheActivation) {
  QString seeded;
  CommandField field{nullptr, kPrompt, PaletteKey(),
                     [&](const QString& text) { seeded = text; }};

  SendKey(field, Qt::Key_D, QStringLiteral("d"));

  EXPECT_EQ(seeded, QStringLiteral("d"));
}

// A shortcut passing through the field is not the operator typing a query.
TEST_F(CommandFieldTest, ModifiedKeysDoNotActivate) {
  int activations = 0;
  CommandField field{nullptr, kPrompt, PaletteKey(),
                     [&](const QString&) { ++activations; }};

  SendKey(field, Qt::Key_S, QStringLiteral("s"), Qt::ControlModifier);
  SendKey(field, Qt::Key_Tab, QStringLiteral("\t"));

  EXPECT_EQ(activations, 0);
}

// The field must never accumulate text: it is an affordance, and the palette
// owns the input.
TEST_F(CommandFieldTest, NeverTakesTextOfItsOwn) {
  CommandField field{nullptr, kPrompt, PaletteKey(), {}};

  SendKey(field, Qt::Key_D, QStringLiteral("d"));

  EXPECT_TRUE(field.text().isEmpty());
}

}  // namespace
