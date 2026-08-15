#include "limit_model.h"

#include "base/awaitable.h"
#include "base/format.h"
#include "common/format.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_format.h"
#include "scada/co_result.h"
#include "services/task_manager.h"

LimitModel::LimitModel(LimitDialogContext&& context)
    : LimitDialogContext{std::move(context)} {}

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

  // `PostUpdateTask` returns a lazy awaitable — spawn it detached so the
  // update actually runs. Discarded, it never starts, and the operator's edits
  // are silently dropped while the dialog closes as if they had been written.
  // Same defect as the one fixed for ID_ITEM_ENABLE/ID_ITEM_DISABLE in
  // `modules/configuration/configuration_module.cpp`.
  // Captures the TaskManager by reference and nothing else from `*this`: the
  // dialog destroys its LimitModel on accept(), so a coroutine holding `this`
  // would run against a freed model. The task manager is module-scoped and
  // outlives it.
  CoSpawn(executor_,
          [&task_manager = task_manager_, node_id = node_.node_id(),
           properties = std::move(properties)]() mutable -> Awaitable<void> {
            (void)co_await task_manager.PostUpdateTask(
                node_id, /*attributes=*/{}, std::move(properties));
          });
}
