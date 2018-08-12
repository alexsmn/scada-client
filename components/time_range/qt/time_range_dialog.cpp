#include "components/time_range/time_range_dialog.h"

#include "services/dialog_service.h"
#include "ui_time_range_dialog.h"

namespace {

QDateTime ToQDateTime(base::Time time) {
  auto delta = time - base::Time::UnixEpoch();
  return QDateTime::fromMSecsSinceEpoch(delta.InMilliseconds());
}

base::Time ToTime(QDateTime date_time) {
  return base::Time::UnixEpoch() +
         base::TimeDelta::FromMilliseconds(date_time.toMSecsSinceEpoch());
}

}  // namespace

class TimeRangeDialog final : public QDialog, private TimeRangeContext {
  Q_OBJECT

 public:
  TimeRangeDialog(TimeRangeContext&& context, QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::TimeRangeDialog ui;
};

#include "time_range_dialog.moc"

TimeRangeDialog::TimeRangeDialog(TimeRangeContext&& context, QWidget* parent)
    : QDialog{parent}, TimeRangeContext{std::move(context)} {
  ui.setupUi(this);

  ui.timeGroupBox->setChecked(time_range_.dates);

  auto bounds = GetTimeRangeBounds(time_range_);
  auto start = ToQDateTime(bounds.first);
  auto end = ToQDateTime(bounds.second);

  ui.startDateEdit->setDate(start.date());
  ui.endDateEdit->setDate(end.date());

  ui.startTimeEdit->setTime(start.time());
  ui.endTimeEdit->setTime(end.time());
}

void TimeRangeDialog::accept() {
  bool dates = ui.timeGroupBox->isChecked();
  QDateTime start{ui.startDateEdit->date(), ui.startTimeEdit->time()};
  QDateTime end{ui.endDateEdit->date(), ui.endTimeEdit->time()};

  time_range_ = TimeRange{ToTime(start), ToTime(end), dates};

  QDialog::accept();
}

bool ShowTimeRangeDialog(DialogService& dialog_service,
                         TimeRangeContext&& context) {
  TimeRangeDialog dialog{std::move(context), dialog_service.GetParentWidget()};
  return dialog.exec() == TimeRangeDialog::Accepted;
}
