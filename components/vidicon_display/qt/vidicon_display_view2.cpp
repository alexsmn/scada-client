#include "components/vidicon_display/qt/vidicon_display_view2.h"

#include "components/vidicon_display/qt/gdi_widget.h"
#include "components/vidicon_display/qt/vidicon_display_lib.h"
#include "services/vidicon/teleclient.h"
#include "services/vidicon/vidicon_client.h"
#include "window_definition.h"

namespace {

inline RECT ToRECT(const QRect& rect) {
  return {.left = rect.left(),
          .top = rect.top(),
          .right = rect.right(),
          .bottom = rect.bottom()};
}

inline QRect ToQRect(const RECT& rect) {
  return QRect{rect.left, rect.top, rect.right - rect.left,
               rect.bottom - rect.top};
}

class DisplayWidget : public GdiWidget {
 public:
  explicit DisplayWidget(QWidget* parent = nullptr) : GdiWidget{parent} {
    display_.set_invalidate_handler([this](const vidicon::display_rect& rect) {
      update(ToQRect(display_.viewport_rect(display_viewport(), rect)));
    });
  }

  void open(const std::filesystem::path& path, IClient& teleclient) {
    display_.open(path, teleclient);
  }

 protected:
  vidicon::display_viewport display_viewport() const {
    return {.rect = ToRECT(viewport()->rect())};
  }

  virtual void paint(HDC dc, const RECT& rect) override {
    display_.draw(dc, display_viewport());
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
