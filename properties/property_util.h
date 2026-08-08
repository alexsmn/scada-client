#pragma once

#include "base/any_executor.h"

#include "aui/models/edit_data.h"
#include "node_service/node_ref.h"

#include <memory>

struct PropertyContext;

// The "no target selected" choice offered by a reference property, and the
// sentinel `SetText` compares the edited text against to detect it. A function
// rather than a constant because the text is translated, and a namespace-scope
// constant would be initialised before there is a QApplication to translate
// against.
std::u16string ChoiceNone();

void SetTextHelper(const PropertyContext& context,
                   const NodeRef& node,
                   const scada::NodeId& prop_decl_id,
                   std::u16string_view text);

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const std::u16string_view& name,
                              const scada::NodeId& node_type_id);

scada::aui::EditData::AsyncChoiceHandler MakeAsyncChoiceHandler(
    AnyExecutor executor,
    const NodeRef& parent,
    const scada::NodeId& type_definition_id);
