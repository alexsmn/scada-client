#include "components/vidicon_display/qt/vidicon_display_view2.h"

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "components/vidicon_display/qt/gdi_widget.h"
#include "components/vidicon_display/qt/vidicon_display_lib.h"
#include "services/vidicon/teleclient.h"
#include "services/vidicon/vidicon_client.h"
#include "window_definition.h"

namespace {

class DisplayWidget : public GdiWidget {
 public:
  explicit DisplayWidget(QWidget* parent = nullptr) : GdiWidget{parent} {}

  void open(const std::filesystem::path& path, IClient& teleclient) {
    display_.open(path, teleclient);
  }

 protected:
  virtual void paint(HDC dc, const RECT& rect) override {
    /*HBRUSH hbrGreen = CreateHatchBrush(HS_BDIAGONAL, RGB(0, 255, 0));

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, hbrGreen);
    Rectangle(dc, 0, 0, rect.right, rect.bottom);

    SelectObject(dc, GetStockObject(NULL_BRUSH));
    Ellipse(dc, 50, 50, rect.right - 100, rect.bottom - 100);

    QString text("Test GDI Paint");
    SetTextAlign(dc, TA_CENTER | TA_BASELINE);
    TextOutW(dc, (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2,
             (LPCWSTR)text.utf16(), text.size());

    DeleteObject(hbrGreen);*/

    display_.draw(dc);
  }

 private:
  vidicon::display display_{display_library_};

  inline static vidicon::display_library display_library_;
};

}  // namespace

VidiconDisplayView2::VidiconDisplayView2(vidicon::VidiconClient& vidicon_client)
    : vidicon_client_{vidicon_client} {}

VidiconDisplayView2::~VidiconDisplayView2() = default;

UiView* VidiconDisplayView2::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  auto widget = std::make_unique<DisplayWidget>();
  widget->open(path_, vidicon_client_.teleclient());

  widget_ = widget.get();
  return widget.release();
}

void VidiconDisplayView2::Save(WindowDefinition& definition) {
  definition.path = path_;
}
