#include "commands/select_item_dialog.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>
#include <map>
#include <vector>
#include <set>

#include "base/strings/sys_string_conversions.h"
#include "views/client_utils_views.h"
#include "common_resources.h"
#include "common/scada_node_ids.h"
#include "common/namespaces.h"
#include "common_resources.h"
#include "dialog_service.h"
#include "common/node_ref.h"
#include "translation.h"

class SelectItemDialog : public ATL::CDialogImpl<SelectItemDialog> {
 public:
  enum { IDD = IDD_SELITEM };

  explicit SelectItemDialog(NodeService& node_service);
  
  scada::NodeId GetItemNodeId(HTREEITEM item) const;
  HTREEITEM GetTreeItem(const scada::NodeId& node_id);
  HTREEITEM InsertTree(const NodeRef& node, HTREEITEM parent = NULL);
  void LoadItems();

  BEGIN_MSG_MAP(SelectItemDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    NOTIFY_HANDLER(IDC_TREE, TVN_SELCHANGED, OnTreeSelect);
    NOTIFY_HANDLER(IDC_LIST, LVN_ITEMCHANGED, OnListSelect);
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  LRESULT OnTreeSelect(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
  LRESULT OnListSelect(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

  NodeService& node_service_;

  typedef std::map<scada::NodeId, HTREEITEM> R2I;
  typedef std::vector<scada::NodeId> LI2TR;
  R2I				tree_r2i;
  LI2TR			list_data;
  WTL::CTreeViewCtrl	tree;
  WTL::CListViewCtrl	list;
  scada::NodeId sel_group;

  typedef std::set<scada::NodeId> NodeIdSet;
  NodeIdSet		trids;
};

SelectItemDialog::SelectItemDialog(NodeService& node_service)
    : node_service_{node_service} {
}

LRESULT SelectItemDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());

	tree = GetDlgItem(IDC_TREE);
	list = GetDlgItem(IDC_LIST);

	CImageList images;
	images.Create(IDB_ITEMS, 16, 0, RGB(255, 0, 255));

	tree.SetImageList(images);
	list.SetImageList(images, LVSIL_SMALL);

  /*auto& tree = *configuration_.GetNode(id::DataItems);
	for (auto* node : scada::GetOrganizes(tree)) {
		if (node->GetNodeClass() == scada::NodeClass::Object)
			InsertTree(node);
	}*/

	return 0;
}

LRESULT SelectItemDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i = list.GetNextItem(-1, LVNI_SELECTED);
	while (i != -1) {
		int p = list.GetItemData(i);
		scada::NodeId& trid = list_data[p];
		trids.insert(trid);
		i = list.GetNextItem(i, LVNI_SELECTED);
	}

	EndDialog(IDOK);
	return 0;
}

LRESULT SelectItemDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT SelectItemDialog::OnTreeSelect(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	sel_group = GetItemNodeId(tree.GetSelectedItem());
	LoadItems();
	return 0;
}

LRESULT SelectItemDialog::OnListSelect(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	return 0;
}

HTREEITEM SelectItemDialog::GetTreeItem(const scada::NodeId& node_id) {
	R2I::const_iterator i = tree_r2i.find(node_id);
	return i != tree_r2i.end() ? i->second : NULL;
}

scada::NodeId SelectItemDialog::GetItemNodeId(HTREEITEM item) const {
	if (!item)
		return scada::NodeId();
	return scada::NodeId(tree.GetItemData(item), NamespaceIndexes::GROUP);
}

inline int GetImage(const NodeRef& node) {
	if (node.node_class() == scada::NodeClass::Object)
		return 0;
	else
		return 1;
}

HTREEITEM SelectItemDialog::InsertTree(const NodeRef& node, HTREEITEM parent) {
	// insert new item
	base::string16 name = ToString16(node.display_name());
	int img = GetImage(node);
	UINT mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	HTREEITEM item = tree.InsertItem(mask, name.c_str(), img, img, 0, 0,
	  (LPARAM)node.id().numeric_id(), parent, TVI_LAST);
	assert(item);
	tree_r2i.insert(R2I::value_type(node.id(), item));

	// insert children
	/*for (auto* child : scada::GetOrganizes(*node)) {
		if (child->GetNodeClass() == scada::NodeClass::Object)
			InsertTree(child, item);
	}*/

	return item;
}

void SelectItemDialog::LoadItems() {
	list.DeleteAllItems();

	/*auto* node = configuration_.GetNode(sel_group);
	if (!node)
		return;

	list_data.clear();
	
	int p = 0;
	for (auto* child : scada::GetOrganizes(*node)) {
		list_data.push_back(child->GetId());
		base::string16 name = base::SysNativeMBToWide(child->GetName());
		int img = GetImage(*child);
		int n = list.AddItem(p, 0, name.c_str(), img);
		list.SetItemData(n, p);
		p++;
	}*/
}

NodeIdSet RunSelectItemsDialog(DialogService& dialog_service, NodeService& node_service) {
  SelectItemDialog dlg{node_service};
  if (dlg.DoModal(static_cast<DialogServiceViews&>(dialog_service).GetParentView()) == IDOK)
    return dlg.trids;
  else
    return NodeIdSet();
}
