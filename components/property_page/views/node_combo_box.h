#pragma once

#include "base/memory/weak_ptr.h"
#include "common/node_ref.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <map>

namespace scada {
class ViewService;
}

class NodeService;

class NodeComboBox {
 public:
  void Init(HWND combo_box_handle);

  WTL::CComboBox& combo_box() { return combo_box_; }

  void Fill(scada::ViewService& view_service,
            NodeService& node_service,
            const scada::NodeId& root_node_id,
            const scada::NodeId& type_definition_id,
            const scada::NodeId& selected_node_id);

  bool Select(const scada::NodeId& node_id);

  NodeRef GetSelection() const;

 private:
  WTL::CComboBox combo_box_;

  std::map<base::string16, NodeRef> nodes_;

  base::WeakPtrFactory<NodeComboBox> weak_ptr_factory_{this};
};

class ItemComboBox {
 public:
  void Init(HWND combo_box_handle);

  WTL::CComboBox& combo_box() { return combo_box_; }

  void Fill(scada::ViewService& view_service,
            NodeService& node_service,
            const scada::NodeId& device_id);

  scada::NodeId GetSelectedId() const;

  base::string16 GetText() const;
  void SetText(base::StringPiece16 text);

 private:
  WTL::CComboBox combo_box_;

  std::map<base::string16, scada::NodeId> component_items_;

  base::WeakPtrFactory<ItemComboBox> weak_ptr_factory_{this};
};
