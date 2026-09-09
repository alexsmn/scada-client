#include "main_window/command_palette_qt.h"

#include "aui/qt/key_codes.h"
#include "aui/translation.h"
#include "controller/command_handler.h"
#include "controller/command_manager.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// Entry ids at or above this are extra (non-command) items; the offset from the
// base is the index into extra_items_. Real command ids are small resource ids,
// so this leaves no overlap.
constexpr unsigned kExtraItemBase = 1u << 30;

// Right-aligned shortcut hint for a command, or empty if it has none. Mirrors
// the toolbar/menu key-sequence construction (key code + modifier bitmask).
std::u16string ShortcutText(const CommandDescriptor& descriptor) {
  if (!descriptor.shortcut)
    return {};
  return scada::aui::ToQKeySequence(descriptor.shortcut->modifiers(),
                                    descriptor.shortcut->key_code())
      .toString(QKeySequence::NativeText)
      .toStdU16String();
}

}  // namespace

CommandPalette::CommandPalette(QWidget* parent,
                               const CommandManager& command_manager,
                               HandlerResolver resolver,
                               std::vector<ExtraItem> extra_items)
    : QDialog{parent},
      resolver_{std::move(resolver)},
      extra_items_{std::move(extra_items)} {
  setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  setObjectName("commandPalette");
  setModal(true);

  for (const CommandDescriptor* descriptor : command_manager.commands()) {
    std::u16string title = descriptor->GetTitle();
    if (title.empty())
      continue;
    entries_.push_back(
        {descriptor->command_id, std::move(title), ShortcutText(*descriptor)});
  }

  // Non-command entries are matched by title alongside commands; their entry
  // ids are offset past any real command id so activation can tell them apart.
  for (size_t i = 0; i < extra_items_.size(); ++i) {
    if (extra_items_[i].title.empty())
      continue;
    entries_.push_back({static_cast<unsigned>(kExtraItemBase + i),
                        extra_items_[i].title, extra_items_[i].detail});
  }

  filter_ = new QLineEdit{this};
  filter_->setObjectName("commandPaletteFilter");
  filter_->setPlaceholderText(
      QString::fromStdU16String(Translate("Type a command…")));
  filter_->setClearButtonEnabled(true);
  filter_->installEventFilter(this);

  list_ = new QListWidget{this};
  list_->setObjectName("commandPaletteList");
  list_->setUniformItemSizes(true);
  list_->setFocusPolicy(Qt::NoFocus);

  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(filter_);
  layout->addWidget(list_);

  connect(filter_, &QLineEdit::textChanged, this, [this] { Refilter(); });
  connect(list_, &QListWidget::itemActivated, this,
          [this](QListWidgetItem*) { ActivateCurrent(); });
  connect(list_, &QListWidget::itemClicked, this,
          [this](QListWidgetItem*) { ActivateCurrent(); });

  resize(560, 380);
  Refilter();
  filter_->setFocus();
}

CommandPalette::~CommandPalette() = default;

void CommandPalette::PresetFilter(const QString& text) {
  filter_->setText(text);
  filter_->setCursorPosition(static_cast<int>(text.length()));
}

void CommandPalette::Refilter() {
  const std::u16string query = filter_->text().toStdU16String();
  std::vector<CommandEntry> matches = RankCommandMatches(entries_, query);

  list_->clear();
  for (const CommandEntry& entry : matches) {
    QString label = QString::fromStdU16String(entry.title);
    if (!entry.detail.empty())
      label += QStringLiteral("\t") + QString::fromStdU16String(entry.detail);
    auto* item = new QListWidgetItem{label, list_};
    item->setData(Qt::UserRole, entry.command_id);
  }
  if (list_->count() > 0)
    list_->setCurrentRow(0);
}

void CommandPalette::MoveSelection(int delta) {
  const int count = list_->count();
  if (count == 0)
    return;
  int row = list_->currentRow() + delta;
  row = std::clamp(row, 0, count - 1);
  list_->setCurrentRow(row);
}

void CommandPalette::ActivateCurrent() {
  QListWidgetItem* item = list_->currentItem();
  if (!item)
    return;
  const unsigned command_id = item->data(Qt::UserRole).toUInt();

  // Close before running: the action may open its own dialog/view, which should
  // be parented to the main window rather than sit behind the modal palette.
  close();

  if (command_id >= kExtraItemBase) {
    const size_t index = command_id - kExtraItemBase;
    if (index < extra_items_.size() && extra_items_[index].activate)
      extra_items_[index].activate();
    return;
  }

  if (resolver_) {
    if (CommandHandler* handler = resolver_(command_id);
        handler && handler->IsCommandEnabled(command_id)) {
      handler->ExecuteCommand(command_id);
    }
  }
}

bool CommandPalette::eventFilter(QObject* watched, QEvent* event) {
  if (watched == filter_ && event->type() == QEvent::KeyPress) {
    auto* key_event = static_cast<QKeyEvent*>(event);
    switch (key_event->key()) {
      case Qt::Key_Down:
        MoveSelection(1);
        return true;
      case Qt::Key_Up:
        MoveSelection(-1);
        return true;
      case Qt::Key_PageDown:
        MoveSelection(list_->count());
        return true;
      case Qt::Key_PageUp:
        MoveSelection(-list_->count());
        return true;
      case Qt::Key_Return:
      case Qt::Key_Enter:
        ActivateCurrent();
        return true;
      default:
        break;
    }
  }
  return QDialog::eventFilter(watched, event);
}
