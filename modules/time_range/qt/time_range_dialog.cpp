#include "modules/time_range/time_range_dialog.h"
#include "base/time_utils.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "ui_time_range_dialog.h"

#include <QPushButton>

namespace {

QDateTime ToQDateTime(scada::Time time) {
  auto delta = time - scada::Time{};
  return QDateTime::fromMSecsSinceEpoch(InMilliseconds(delta));
}

scada::Time ToTime(QDateTime date_time) {
  return scada::Time{} +
         std::chrono::milliseconds(date_time.toMSecsSinceEpoch());
}

}  // namespace

class TimeRangeDialog final : public QDialog, private TimeRangeContext {
  Q_OBJECT

 public:
  TimeRangeDialog(TimeRangeContext&& context, QWidget* parent = nullptr);

  const scada::RelativeTimeRange& time_range() const { return time_range_; }

 public Q_SLOTS:
  virtual void accept() override;

 private:
  // A custom period whose end is not after its start used to be accepted and
  // then silently replaced by the last hour in `ToTimeRange` (backlog 912).
  bool IsRangeValid() const;
  void UpdateOkButton();

  Ui::TimeRangeDialog ui;
};

#include "time_range_dialog.moc"

TimeRangeDialog::TimeRangeDialog(TimeRangeContext&& context, QWidget* parent)
    : QDialog{parent}, TimeRangeContext{std::move(context)} {
  ui.setupUi(this);

  ui.timeGroupBox->setChecked(!time_range_.dates);

  auto bounds = scada::ToTimeRange(time_range_, /*now=*/scada::Now());
  auto start = ToQDateTime(bounds.first);
  auto end = ToQDateTime(bounds.second);
  // In dates-only mode the resolved end is the midnight AFTER the last day,
  // while the range stores (and accept() writes back) the last day itself —
  // ToTimeRange adds the day. Show the last day, or every OK would move the
  // end one day later.
  if (time_range_.dates)
    end = end.addDays(-1);

  ui.startDateEdit->setDate(start.date());
  ui.endDateEdit->setDate(end.date());

  ui.startTimeEdit->setTime(start.time());
  ui.endTimeEdit->setTime(end.time());

  connect(ui.startDateEdit, &QDateEdit::dateChanged, this,
          &TimeRangeDialog::UpdateOkButton);
  connect(ui.endDateEdit, &QDateEdit::dateChanged, this,
          &TimeRangeDialog::UpdateOkButton);
  connect(ui.startTimeEdit, &QTimeEdit::timeChanged, this,
          &TimeRangeDialog::UpdateOkButton);
  connect(ui.endTimeEdit, &QTimeEdit::timeChanged, this,
          &TimeRangeDialog::UpdateOkButton);
  connect(ui.timeGroupBox, &QGroupBox::toggled, this,
          &TimeRangeDialog::UpdateOkButton);
  UpdateOkButton();
}

bool TimeRangeDialog::IsRangeValid() const {
  // Dates-only: the end is a whole day and `ToTimeRange` adds it, so one day
  // (start == end) is a valid period. With times, the end must be later.
  if (!ui.timeGroupBox->isChecked())
    return ui.startDateEdit->date() <= ui.endDateEdit->date();
  return QDateTime{ui.startDateEdit->date(), ui.startTimeEdit->time()} <
         QDateTime{ui.endDateEdit->date(), ui.endTimeEdit->time()};
}

void TimeRangeDialog::UpdateOkButton() {
  if (QPushButton* ok = ui.buttonBox->button(QDialogButtonBox::Ok))
    ok->setEnabled(IsRangeValid());
}

void TimeRangeDialog::accept() {
  if (!IsRangeValid())
    return;

  bool dates = !ui.timeGroupBox->isChecked();
  QDateTime start{ui.startDateEdit->date(), ui.startTimeEdit->time()};
  QDateTime end{ui.endDateEdit->date(), ui.endTimeEdit->time()};

  time_range_ = scada::RelativeTimeRange{ToTime(start), ToTime(end), dates};

  QDialog::accept();
}

Awaitable<scada::RelativeTimeRange> ShowTimeRangeDialog(
    DialogService& dialog_service,
    TimeRangeContext&& context) {
  auto dialog = std::make_unique<TimeRangeDialog>(
      std::move(context), dialog_service.GetParentWidget());
  return StartMappedModalDialog(std::move(dialog), [](TimeRangeDialog& dialog) {
    return dialog.time_range();
  });
}
