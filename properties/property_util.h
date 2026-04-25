#pragma once

#include "aui/models/edit_data.h"
#include "node_service/node_ref.h"

#include <memory>

class Executor;
struct PropertyContext;

extern const std::u16string_view kChoiceNone;

void SetTextHelper(const PropertyContext& context,
                   const NodeRef& node,
                   const scada::NodeId& prop_decl_id,
                   std::u16string_view text);

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const std::u16string_view& name,
                              const scada::NodeId& node_type_id);

aui::EditData::AsyncChoiceHandler MakeAsyncChoiceHandler(
    std::shared_ptr<Executor> executor,
    const NodeRef& parent,
    const scada::NodeId& type_definition_id);
