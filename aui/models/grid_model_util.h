#pragma once

namespace aui {

class GridModel;
class GridRange;

void ExpandGridRange(GridModel& model,
                     const GridRange& range,
                     const GridRange& new_range,
                     bool increment);

}  // namespace aui
