#pragma once

#include "controls/types.h"

class DialogService;
class NodeService;

NodeIdSet RunSelectItemsDialog(DialogService& dialog_service, NodeService& node_service);
