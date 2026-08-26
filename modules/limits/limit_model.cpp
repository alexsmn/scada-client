#include "limit_model.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/format.h"
#include "common/format.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_format.h"
#include "scada/co_result.h"
#include "services/task_manager.h"

LimitModel::LimitModel(LimitDialogContext&& context)
    : LimitDialogContext{std::move(context)} {}

std::u16string LimitModel::GetWindowTitle() const {
  return Translate("Limits");
}

std::u16string LimitModel::GetSourceTitle() const {
  return ToString16(node_.display_name());
}

LimitModel::Limits LimitModel::GetLimits() const {
  auto lo = node_[scada::data_items::id::AnalogItemType_LimitLo].value();
  auto hi = node_[scada::data_items::id::AnalogItemType_LimitHi].value();
  auto lolo = node_[scada::data_items::id::AnalogItemType_LimitLoLo].value();
  auto hihi = node_[scada::data_items::id::AnalogItemType_LimitHiHi].value();

  return {FormatValue(node_, lo, {}, 0), FormatValue(node_, hi, {}, 0),
          FormatValue(node_, lolo, {}, 0), FormatValue(node_, hihi, {}, 0)};
}

void LimitModel::WriteLimits(const Limits& limits) {
  // A second Apply while the first is still in flight would post the same
  // bands again and race two completions onto one dialog.
  if (writing_)
    return;

  auto limit_lo =
      limits.lo.empty() ? scada::Variant() : ParseWithDefault(limits.lo, 0.0);
  auto limit_hi =
      limits.hi.empty() ? scada::Variant() : ParseWithDefault(limits.hi, 0.0);
  auto limit_lolo = limits.lolo.empty() ? scada::Variant()
                                        : ParseWithDefault(limits.lolo, 0.0);
  auto limit_hihi = limits.hihi.empty() ? scada::Variant()
                                        : ParseWithDefault(limits.hihi, 0.0);

  scada::NodeProperties properties;
  properties.emplace_back(scada::data_items::id::AnalogItemType_LimitLo,
                          limit_lo);
  properties.emplace_back(scada::data_items::id::AnalogItemType_LimitHi,
                          limit_hi);
  properties.emplace_back(scada::data_items::id::AnalogItemType_LimitLoLo,
                          limit_lolo);
  properties.emplace_back(scada::data_items::id::AnalogItemType_LimitHiHi,
                          limit_hihi);

  writing_ = true;

  // `PostUpdateTask` returns a lazy awaitable — spawn it detached so the
  // update actually runs. Discarded, it never starts, and the operator's edits
  // are silently dropped while the dialog closes as if they had been written.
  // Same defect as the one fixed for ID_ITEM_ENABLE/ID_ITEM_DISABLE in
  // `modules/configuration/configuration_module.cpp`.
  //
  // The status is awaited rather than discarded: it is the only thing that
  // distinguishes a written band from a refused one, and the operator is owed
  // that distinction. Holds a weak_ptr rather than `this` — the dialog now
  // outlives the post (it stays open until `completion_handler` runs) but it
  // can still be closed from underneath one, and a rejected dialog destroys
  // the model while its task is in flight.
  CoSpawn(executor_,
          [&task_manager = task_manager_, model = weak_from_this(),
           node_id = node_.node_id(),
           properties = std::move(properties)]() mutable -> Awaitable<void> {
            auto status = co_await task_manager.PostUpdateTask(
                node_id, /*attributes=*/{}, std::move(properties));
            if (auto model_ptr = model.lock())
              model_ptr->OnWriteComplete(status);
          });
}

void LimitModel::OnWriteComplete(const scada::Status& status) {
  writing_ = false;

  if (status) {
    completion_handler(true);
    return;
  }

  // Nothing to report through: complete anyway rather than stranding the
  // dialog with a disabled Apply button and no explanation.
  if (!dialog_service_) {
    completion_handler(false);
    return;
  }

  CoSpawn(executor_, [completion_handler = completion_handler,
                      dialog_service = dialog_service_,
                      message = ToString16(status) + u'.',
                      title = GetWindowTitle()]() mutable {
    return ReportWriteErrorAsync(std::move(completion_handler),
                                 *dialog_service, std::move(message),
                                 std::move(title));
  });
}

Awaitable<void> LimitModel::ReportWriteErrorAsync(
    std::function<void(bool ok)> completion_handler,
    DialogService& dialog_service,
    std::u16string message,
    std::u16string title) {
  try {
    co_await dialog_service.RunMessageBox(message, title,
                                          MessageBoxMode::Error);
  } catch (...) {
  }

  // False, so the dialog stays open on the values the operator typed and the
  // rejected edit can be corrected or retried. Completing outside the try
  // matters: a dialog service that throws must still release the dialog, or a
  // failed write leaves Apply disabled for good.
  completion_handler(false);
  co_return;
}
