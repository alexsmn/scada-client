#include "main_window/command_palette_qt.h"

#include "controller/command_handler.h"
#include "controller/command_manager.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>

#include <gtest/gtest.h>

#include <map>
#include <vector>

namespace {

// Records which commands were executed and lets a test mark some disabled, so
// the palette's enabled-gating and dispatch can be observed.
class RecordingHandler : public CommandHandler {
 public:
  bool IsCommandEnabled(unsigned command_id) const override {
    auto it = enabled_.find(command_id);
    return it == enabled_.end() ? true : it->second;
  }
  void ExecuteCommand(unsigned command_id) override {
    executed_.push_back(command_id);
  }

  std::map<unsigned, bool> enabled_;
  std::vector<unsigned> executed_;
};

// Registers a command with the given id/title (empty title allowed, to check
// the palette skips it).
void AddCommand(CommandManager& manager, unsigned id, std::u16string title) {
  manager.RegisterCommand(
      CommandDescriptor{.command_id = id, .title = std::move(title)});
}

// The palette's list, found by the object name it sets on construction.
QListWidget* ListOf(CommandPalette& palette) {
  return palette.findChild<QListWidget*>("commandPaletteList");
}

QLineEdit* FilterOf(CommandPalette& palette) {
  return palette.findChild<QLineEdit*>("commandPaletteFilter");
}

std::vector<std::u16string> VisibleTitles(CommandPalette& palette) {
  QListWidget* list = ListOf(palette);
  std::vector<std::u16string> titles;
  for (int row = 0; row < list->count(); ++row)
    titles.push_back(list->item(row)->text().toStdU16String());
  return titles;
}

// Sends a key press to the filter field so the palette's installed event
// filter runs (navigation / activation), matching real keyboard input.
void PressKey(CommandPalette& palette, int key) {
  QKeyEvent event{QEvent::KeyPress, key, Qt::NoModifier};
  QApplication::sendEvent(FilterOf(palette), &event);
}

CommandPalette::HandlerResolver ResolverFor(CommandHandler& handler) {
  return [&handler](unsigned) -> CommandHandler* { return &handler; };
}

// Boots a single QApplication for the widget tests (Qt requires one to exist
// before any QWidget is constructed).
QApplication& App() {
  static int argc = 0;
  static QApplication app{argc, nullptr};
  return app;
}

class CommandPaletteTest : public ::testing::Test {
 protected:
  CommandPaletteTest() { App(); }
};

TEST_F(CommandPaletteTest, ListsCommandsSkippingEmptyTitles) {
  CommandManager manager;
  AddCommand(manager, 1, u"Open Display");
  AddCommand(manager, 2, u"");  // no title -> not offered
  AddCommand(manager, 3, u"Write Value");

  RecordingHandler handler;
  CommandPalette palette{nullptr, manager, ResolverFor(handler)};

  EXPECT_EQ(VisibleTitles(palette),
            (std::vector<std::u16string>{u"Open Display", u"Write Value"}));
}

TEST_F(CommandPaletteTest, TypingFiltersTheList) {
  CommandManager manager;
  AddCommand(manager, 1, u"Open Display");
  AddCommand(manager, 2, u"Write Value");
  AddCommand(manager, 3, u"Acknowledge Alarm");

  RecordingHandler handler;
  CommandPalette palette{nullptr, manager, ResolverFor(handler)};

  FilterOf(palette)->setText(QStringLiteral("val"));

  EXPECT_EQ(VisibleTitles(palette),
            (std::vector<std::u16string>{u"Write Value"}));
}

TEST_F(CommandPaletteTest, PresetFilterSeedsAndNarrowsTheQuery) {
  CommandManager manager;
  AddCommand(manager, 1, u"Open Display");
  AddCommand(manager, 2, u"Write Value");
  AddCommand(manager, 3, u"Acknowledge Alarm");

  RecordingHandler handler;
  CommandPalette palette{nullptr, manager, ResolverFor(handler)};
  palette.PresetFilter(QStringLiteral("val"));

  EXPECT_EQ(VisibleTitles(palette),
            (std::vector<std::u16string>{u"Write Value"}));
}

TEST_F(CommandPaletteTest, EnterRunsTheSelectedCommand) {
  CommandManager manager;
  AddCommand(manager, 1, u"Open Display");
  AddCommand(manager, 2, u"Write Value");

  RecordingHandler handler;
  CommandPalette palette{nullptr, manager, ResolverFor(handler)};

  // Filter down to the single command, then activate it.
  FilterOf(palette)->setText(QStringLiteral("write"));
  PressKey(palette, Qt::Key_Return);

  EXPECT_EQ(handler.executed_, (std::vector<unsigned>{2}));
}

TEST_F(CommandPaletteTest, DisabledCommandIsNotExecuted) {
  CommandManager manager;
  AddCommand(manager, 1, u"Write Value");

  RecordingHandler handler;
  handler.enabled_[1] = false;
  CommandPalette palette{nullptr, manager, ResolverFor(handler)};

  PressKey(palette, Qt::Key_Return);

  EXPECT_TRUE(handler.executed_.empty());
}

TEST_F(CommandPaletteTest, ArrowKeysMoveSelectionBeforeActivating) {
  CommandManager manager;
  AddCommand(manager, 1, u"Alpha");
  AddCommand(manager, 2, u"Beta");
  AddCommand(manager, 3, u"Gamma");

  RecordingHandler handler;
  CommandPalette palette{nullptr, manager, ResolverFor(handler)};

  // Row 0 (Alpha) is selected initially; move down twice to Gamma (id 3).
  PressKey(palette, Qt::Key_Down);
  PressKey(palette, Qt::Key_Down);
  PressKey(palette, Qt::Key_Return);

  EXPECT_EQ(handler.executed_, (std::vector<unsigned>{3}));
}

}  // namespace
