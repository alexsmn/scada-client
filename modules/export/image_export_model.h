#pragma once

#include <string>

class QPainter;
class QSize;

// A view whose contents are a picture rather than rows — the graph. Printing
// scales the picture onto the page, and "Save as Image" writes it to a file
// (backlog 874). Views with rows implement ExportModel instead.
class ImageExportModel {
 public:
  virtual ~ImageExportModel() = default;

  // The picture's natural size, in device-independent pixels.
  virtual QSize GetImageSize() const = 0;

  // Paints the picture at its natural size, with its top-left corner at the
  // painter's origin. The caller scales the painter to fit a page.
  virtual void RenderImage(QPainter& painter) = 0;

  // What a printout's header should say beyond the window title and period.
  virtual std::u16string GetExportDescription() { return {}; }
};
