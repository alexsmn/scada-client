#pragma once

#include <functional>

class MainWindow;
class OpenedView;
class WindowDefinition;

using OpenedViewFactory =
    std::function<std::unique_ptr<OpenedView>(MainWindow& main_window,
                                              WindowDefinition& window_def)>;
