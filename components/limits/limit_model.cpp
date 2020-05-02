#include "limit_model.h"

#include "base/format.h"
#include "common/format.h"
#include "common/node_format.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "services/task_manager.h"

LimitModel::LimitModel(LimitDialogContext&& context)
    : LimitDialogContext{std::move(context)} {}

base::string16 LimitModel::GetSourceTitle() const {
  return node_.display_name();
}

LimitModel::Limits LimitModel::GetLimits() const {
  auto lo = node_[data_items::id::AnalogItemType_LimitLo].value();
  auto hi = node_[data_items::id::AnalogItemType_LimitHi].value();
  auto lolo = node_[data_items::id::AnalogItemType_LimitLoLo].value();
  auto hihi = node_[data_items::id::AnalogItemType_LimitHiHi].value();

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
  properties.emplace_back(data_items::id::AnalogItemType_LimitLo, limit_lo);
  properties.emplace_back(data_items::id::AnalogItemType_LimitHi, limit_hi);
  properties.emplace_back(data_items::id::AnalogItemType_LimitLoLo, limit_lolo);
  properties.emplace_back(data_items::id::AnalogItemType_LimitHiHi, limit_hihi);

  task_manager_.PostUpdateTask(node_.node_id(), {}, properties);
}
