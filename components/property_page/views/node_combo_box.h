#pragma once

#include "base/memory/weak_ptr.h"
#include "client_utils.h"
#include "node_service/node_ref.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <map>

class NodeService;

class NodeComboBox {
 public:
  void Init(HWND combo_box_handle);

  WTL::CComboBox& combo_box() { return combo_box_; }

  void Fill(const NodeRef& root,
            const scada::NodeId& type_definition_id,
            const scada::NodeId& selected_node_id);

  bool Select(const scada::NodeId& node_id);

  NodeRef GetSelection() const;

 private:
  WTL::CComboBox combo_box_;

  NamedNodes nodes_;

  base::WeakPtrFactory<NodeComboBox> weak_ptr_factory_{this};
};

class ItemComboBox {
 public:
  void Init(NodeService& node_service, HWND combo_box_handle);

  WTL::CComboBox& combo_box() { return combo_box_; }

  void SetDeviceId(const scada::NodeId& device_id);

  scada::NodeId GetNodeId() const;
  void SetNodeId(const scada::NodeId& node_id);

 private:
  void AddNodesRecursive(const scada::NodeId& parent_id,
                         const std::vector<scada::NodeId>& reference_type_ids,
                         const std::wstring& name_prefix);

  NodeService* node_service_ = nullptr;

  WTL::CComboBox combo_box_;

  scada::NodeId device_id_;

  std::map<std::wstring, scada::NodeId> items_;
  std::map<scada::NodeId, std::wstring> node_ids_;

  base::WeakPtrFactory<ItemComboBox> weak_ptr_factory_{this};
};
