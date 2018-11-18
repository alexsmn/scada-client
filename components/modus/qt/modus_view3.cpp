#include "components/modus/qt/modus_view3.h"

#include <QtWinExtras/qwinfunctions.h>
#include <qevent.h>
#include <qpaintengine.h>
#include <qpainter.h>

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "client_utils.h"
#include "components/modus/libmodus/modus_binding2.h"
#include "components/modus/libmodus/modus_module2.h"

#include "libmodus/gfx/canvas.h"
#include "libmodus/render/renderer.h"
#include "libmodus/render/shape.h"
#include "libmodus/scheme/element.h"
#include "libmodus/scheme/property_def.h"
#include "libmodus/scheme/scheme.h"
#include "libmodus/scheme/serialization.h"
#include "libmodus/scheme/value.h"

namespace {
const int kSelectionInset = 3;
const float kHitTolerance = 6.0f;
}  // namespace

ModusView3::ModusView3(TimedDataService& timed_data_service)
    : timed_data_service_{timed_data_service} {
  setDocument(&document_);
}

ModusView3::~ModusView3() {
  setDocument(nullptr);
}

void ModusView3::Open(const base::FilePath& path) {
  path_ = path;

  document_.Load(QString::fromStdWString(path_.value()));
}

base::FilePath ModusView3::GetPath() const {
  return path_;
}

bool ModusView3::ShowContainedItem(const scada::NodeId& item_id) {
  // TODO:
  return false;
}

htsde2::IHTSDEForm2* ModusView3::GetSdeForm() {
  return nullptr;
}
