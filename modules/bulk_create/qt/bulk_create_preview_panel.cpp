#include "bulk_create/qt/bulk_create_preview_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include <string_view>
#include <utility>

namespace {

const scada::aui::ThemeTokens& PanelTokens() {
  return scada::aui::ActiveThemeTokens();
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

QSpinBox* MakeSpin(int min, int max, int value) {
  auto* spin = new QSpinBox;
  spin->setRange(min, max);
  spin->setValue(value);
  return spin;
}

}  // namespace

BulkCreatePreviewPanel::BulkCreatePreviewPanel(QWidget* parent)
    : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("bulkCreatePreviewPanel"));
  setStyleSheet(QStringLiteral("#bulkCreatePreviewPanel{background:%1;}")
                    .arg(tokens.bg_elevated.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(14, 14, 14, 14);
  root->setSpacing(10);
  root->addWidget(BuildForm());
  root->addWidget(BuildPreview());

  Refresh();
}

BulkCreatePreviewPanel::~BulkCreatePreviewPanel() = default;

QWidget* BulkCreatePreviewPanel::BuildForm() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* form_host = new QWidget;
  auto* form = new QFormLayout{form_host};
  form->setContentsMargins(0, 0, 0, 0);
  form->setLabelAlignment(Qt::AlignLeft);

  name_template_ = new QLineEdit(QStringLiteral("TS{n} current"));
  name_template_->setObjectName(QStringLiteral("nameTemplate"));
  node_id_template_ = new QLineEdit(QStringLiteral("ns=2;s=RTU.TS{n}.I"));
  node_id_template_->setObjectName(QStringLiteral("nodeIdTemplate"));
  start_index_ = MakeSpin(0, 1000000, 1);
  count_ = MakeSpin(0, 100000, 12);
  count_->setObjectName(QStringLiteral("countSpin"));
  index_step_ = MakeSpin(1, 100000, 1);
  ioa_start_ = MakeSpin(0, 1000000, 4001);
  ioa_step_ = MakeSpin(1, 100000, 1);

  form->addRow(Tr("Name template"), name_template_);
  form->addRow(Tr("NodeId template"), node_id_template_);
  form->addRow(Tr("Start index"), start_index_);
  form->addRow(Tr("Count"), count_);
  form->addRow(Tr("Index step"), index_step_);
  form->addRow(Tr("IOA start"), ioa_start_);
  form->addRow(Tr("IOA step"), ioa_step_);

  // Muted field-label colour so the grid reads as the focus.
  form_host->setStyleSheet(
      QStringLiteral("QLabel{color:%1;}").arg(tokens.fg_muted.name()));

  connect(name_template_, &QLineEdit::textChanged, this,
          [this](const QString&) { Refresh(); });
  connect(node_id_template_, &QLineEdit::textChanged, this,
          [this](const QString&) { Refresh(); });
  for (QSpinBox* spin :
       {start_index_, count_, index_step_, ioa_start_, ioa_step_}) {
    connect(spin, &QSpinBox::valueChanged, this, [this](int) { Refresh(); });
  }

  return form_host;
}

QWidget* BulkCreatePreviewPanel::BuildPreview() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* host = new QWidget;
  auto* layout = new QVBoxLayout{host};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(6);

  summary_ = new QLabel;
  summary_->setObjectName(QStringLiteral("previewSummary"));
  summary_->setStyleSheet(
      QStringLiteral("color:%1;font-weight:600;").arg(tokens.fg.name()));
  layout->addWidget(summary_);

  preview_ = new QTableWidget;
  preview_->setObjectName(QStringLiteral("previewGrid"));
  preview_->setColumnCount(5);
  preview_->setHorizontalHeaderLabels(
      {QStringLiteral("#"), Tr("Name"), Tr("NodeId"), Tr("IOA"), Tr("Status")});
  preview_->verticalHeader()->setVisible(false);
  preview_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  preview_->setSelectionMode(QAbstractItemView::NoSelection);
  preview_->horizontalHeader()->setStretchLastSection(true);
  preview_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  preview_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
  layout->addWidget(preview_);

  return host;
}

BulkCreateParams BulkCreatePreviewPanel::CurrentParams() const {
  BulkCreateParams params;
  params.name_template = name_template_->text().toStdU16String();
  params.node_id_template = node_id_template_->text().toStdU16String();
  params.start_index = start_index_->value();
  params.count = count_->value();
  params.index_step = index_step_->value();
  params.ioa_start = ioa_start_->value();
  params.ioa_step = ioa_step_->value();
  return params;
}

void BulkCreatePreviewPanel::SetExistingNodeIds(
    std::set<std::u16string> existing) {
  existing_node_ids_ = std::move(existing);
  Refresh();
}

void BulkCreatePreviewPanel::SetParams(const BulkCreateParams& params) {
  name_template_->setText(QString::fromStdU16String(params.name_template));
  node_id_template_->setText(
      QString::fromStdU16String(params.node_id_template));
  start_index_->setValue(params.start_index);
  count_->setValue(params.count);
  index_step_->setValue(params.index_step);
  ioa_start_->setValue(params.ioa_start);
  ioa_step_->setValue(params.ioa_step);
  Refresh();
}

void BulkCreatePreviewPanel::Refresh() {
  if (!preview_ || !summary_)
    return;

  const scada::aui::ThemeTokens& tokens = PanelTokens();
  rows_ = ExpandBulkCreate(CurrentParams(), existing_node_ids_);

  preview_->setRowCount(static_cast<int>(rows_.size()));
  for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
    const BulkCreatePreviewRow& row = rows_[i];
    auto set_cell = [&](int column, const QString& text) {
      preview_->setItem(i, column, new QTableWidgetItem(text));
    };
    set_cell(0, QString::number(row.number));
    set_cell(1, QString::fromStdU16String(row.name));
    set_cell(2, QString::fromStdU16String(row.node_id));
    set_cell(3, QString::number(row.ioa));

    auto* status =
        new QTableWidgetItem(row.conflict ? Tr("exists") : Tr("new"));
    if (row.conflict)
      status->setForeground(tokens.bad);
    else
      status->setForeground(tokens.good);
    preview_->setItem(i, 4, status);
  }

  const BulkCreateSummary summary = SummarizeBulkCreate(rows_);
  // e.g. "23 new · 1 conflict".
  QString text = QStringLiteral("%1 %2").arg(summary.new_count).arg(Tr("new"));
  if (summary.conflict_count > 0) {
    text += QStringLiteral(" · %1 %2")
                .arg(summary.conflict_count)
                .arg(Tr("conflict"));
  }
  summary_->setText(text);
}

BulkCreatePreviewPanel* MakeBulkCreatePreviewPanel() {
  return new BulkCreatePreviewPanel;
}
