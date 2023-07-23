#pragma once

#include <cstdint>

namespace aui {

class DragDropTypes {
 public:
  enum DragOperation {
    DRAG_NONE = 0,
    DRAG_MOVE = 1 << 0,
    DRAG_COPY = 1 << 1,
    DRAG_LINK = 1 << 2
  };

  enum DragEventSource {
    DRAG_EVENT_SOURCE_MOUSE,
    DRAG_EVENT_SOURCE_TOUCH,
  };

#if defined(OS_WIN)
  static uint32_t DragOperationToDropEffect(int drag_operation);
  static int DropEffectToDragOperation(uint32_t effect);
#elif !defined(OS_MACOSX)
  static int DragOperationToGdkDragAction(int drag_operation);
  static int GdkDragActionToDragOperation(int gdk_drag_action);
#endif
};

}  // namespace aui
