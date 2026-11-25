#include "events/qt/event_statistics_dialog.h"

#include "aui/translation.h"

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString ToQString(std::u16string_view text) {
  return QString::fromUtf16(text.data(), static_cast<qsizetype>(text.size()));
}

QTableWidgetItem* NumberItem(int value) {
  auto* item = new QTableWidgetItem{QString::number(value)};
  item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  return item;
}

// A read-only table with a first text column and two count columns.
QTableWidget* MakeTallyTable(const std::u16string& first_header, int rows) {
  auto* table = new QTableWidget{rows, 3};
  table->setHorizontalHeaderLabels({ToQString(first_header),
                                    ToQString(Translate("Count")),
                                    ToQString(Translate("Unacknowledged"))});
  table->verticalHeader()->hide();
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  return table;
}

void SetTallyRow(QTableWidget& table,
                 int row,
                 std::u16string_view title,
                 const events::EventStatistics::Tally& tally) {
  table.setItem(row, 0, new QTableWidgetItem{ToQString(title)});
  table.setItem(row, 1, NumberItem(tally.count));
  table.setItem(row, 2, NumberItem(tally.unacknowledged));
}

}  // namespace

QDialog* MakeEventStatisticsDialog(const events::EventStatistics& statistics,
                                   std::u16string period,
                                   QWidget* parent) {
  auto* dialog = new QDialog{parent};
  dialog->setObjectName(QStringLiteral("eventStatisticsDialog"));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(ToQString(Translate("Event Statistics")));

  auto* layout = new QVBoxLayout{dialog};

  auto* period_label = new QLabel{ToQString(period)};
  period_label->setObjectName(QStringLiteral("eventStatisticsPeriod"));
  layout->addWidget(period_label);

  // Worst band first, as the journal's severity tiles order them; the total
  // last, as a sum is read.
  constexpr scada::aui::SeverityLevel kBands[] = {
      scada::aui::SeverityLevel::kCritical, scada::aui::SeverityLevel::kWarning,
      scada::aui::SeverityLevel::kNone};
  auto* bands = MakeTallyTable(Translate("Severity"), std::size(kBands) + 1);
  bands->setObjectName(QStringLiteral("eventStatisticsBands"));
  int row = 0;
  for (auto level : kBands) {
    SetTallyRow(*bands, row++, events::EventStatisticsBandTitle(level),
                statistics.band(level));
  }
  SetTallyRow(*bands, row, Translate("Total"), statistics.total);
  QFont total_font = bands->font();
  total_font.setBold(true);
  for (int column = 0; column < bands->columnCount(); ++column)
    bands->item(row, column)->setFont(total_font);
  // Exactly as tall as its rows: it is a summary, not a list to scroll, and
  // not where the focus belongs either.
  bands->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  bands->setFocusPolicy(Qt::NoFocus);
  bands->resizeRowsToContents();
  int height =
      bands->horizontalHeader()->sizeHint().height() + 2 * bands->frameWidth();
  for (int i = 0; i < bands->rowCount(); ++i)
    height += bands->rowHeight(i);
  bands->setFixedHeight(height);
  layout->addWidget(bands);

  auto* objects = MakeTallyTable(Translate("Object"),
                                 static_cast<int>(statistics.by_object.size()));
  objects->setObjectName(QStringLiteral("eventStatisticsObjects"));
  for (int i = 0; i < static_cast<int>(statistics.by_object.size()); ++i) {
    const auto& object = statistics.by_object[i];
    SetTallyRow(*objects, i, object.name, object.tally);
  }
  // The same row height as the bands above.
  objects->resizeRowsToContents();
  layout->addWidget(objects, 1);

  auto* buttons = new QDialogButtonBox{QDialogButtonBox::Close};
  QPushButton* copy = buttons->addButton(ToQString(Translate("Copy")),
                                         QDialogButtonBox::ActionRole);
  copy->setObjectName(QStringLiteral("eventStatisticsCopy"));
  QObject::connect(
      copy, &QPushButton::clicked, dialog,
      [text = events::FormatEventStatisticsText(statistics, period)] {
        QApplication::clipboard()->setText(ToQString(text));
      });
  QObject::connect(buttons, &QDialogButtonBox::rejected, dialog,
                   &QDialog::reject);
  layout->addWidget(buttons);

  dialog->resize(dialog->fontMetrics().averageCharWidth() * 70,
                 dialog->fontMetrics().height() * 30);
  return dialog;
}
