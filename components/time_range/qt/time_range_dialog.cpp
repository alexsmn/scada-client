#include "components/time_range/time_range_dialog.h"

#include "aui/qt/message_loop_qt.h"
#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "net/net_executor_adapter.h"
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

  const TimeRange& time_range() const { return time_range_; }

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::TimeRangeDialog ui;
};

#include "time_range_dialog.moc"

TimeRangeDialog::TimeRangeDialog(TimeRangeContext&& context, QWidget* parent)
    : QDialog{parent}, TimeRangeContext{std::move(context)} {
  ui.setupUi(this);

  ui.timeGroupBox->setChecked(!time_range_.dates);

  auto bounds = ToDateTimeRange(time_range_, /*now=*/base::Time::Now());
  auto start = ToQDateTime(bounds.first);
  auto end = ToQDateTime(bounds.second);

  ui.startDateEdit->setDate(start.date());
  ui.endDateEdit->setDate(end.date());

  ui.startTimeEdit->setTime(start.time());
  ui.endTimeEdit->setTime(end.time());
}

void TimeRangeDialog::accept() {
  bool dates = !ui.timeGroupBox->isChecked();
  QDateTime start{ui.startDateEdit->date(), ui.startTimeEdit->time()};
  QDateTime end{ui.endDateEdit->date(), ui.endTimeEdit->time()};

  time_range_ = TimeRange{ToTime(start), ToTime(end), dates};

  QDialog::accept();
}

namespace {

Awaitable<TimeRange> RunTimeRangeDialogAsync(
    std::unique_ptr<TimeRangeDialog> dialog) {
  auto executor = co_await boost::asio::this_coro::executor;
  promise<TimeRange> result;

  QObject::connect(dialog.get(), &QDialog::accepted,
                   [&dialog = *dialog, result]() mutable {
                     result.resolve(dialog.time_range());
                     dialog.deleteLater();
                   });

  QObject::connect(dialog.get(), &QDialog::rejected,
                   [&dialog = *dialog, result]() mutable {
                     result.reject(std::exception{});
                     dialog.deleteLater();
                   });

  dialog->setModal(true);
  dialog.release()->show();

  co_return co_await AwaitPromise(executor, std::move(result));
}

}  // namespace

promise<TimeRange> ShowTimeRangeDialog(DialogService& dialog_service,
                                       TimeRangeContext&& context) {
  auto dialog = std::make_unique<TimeRangeDialog>(
      std::move(context), dialog_service.GetParentWidget());
  auto executor = std::make_shared<MessageLoopQt>();
  return ToPromise(NetExecutorAdapter{executor},
                   RunTimeRangeDialogAsync(std::move(dialog)));
}
