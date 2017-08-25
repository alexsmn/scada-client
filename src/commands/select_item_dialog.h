#pragma once

#include "controls/types.h"

class DialogService;
class NodeRefService;

NodeIdSet RunSelectItemsDialog(DialogService& dialog_service, NodeRefService& node_service);
