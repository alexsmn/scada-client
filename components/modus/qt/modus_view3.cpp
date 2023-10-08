#include "components/modus/qt/modus_view3.h"

#include "filesystem/file_util.h"
#include "controller/window_definition.h"
#include "timed_data/timed_data_spec.h"

#include <QEvent>
#include <QPaintEngine>
#include <QPainter>
#include <QtWinExtras/qwinfunctions.h>

namespace {

const int kSelectionInset = 3;
const float kHitTolerance = 6.0f;

QString GetDefaultPropertyName(const Schematic::Element& element) {
  int type = static_cast<QString>(element["typ"]).toInt();
  if (type == 134)
    return "Tech.value";
  else
    return "Tech.closed";
}

}  // namespace

// ModusView3

ModusView3::ModusView3(TimedDataService& timed_data_service)
    : timed_data_service_{timed_data_service} {
  setScene(&scene_);
  view_.setDocument(&document_);
}

ModusView3::~ModusView3() {}

void ModusView3::Open(const WindowDefinition& definition) {
  path_ = GetPublicFilePath(definition.path);

  document_.Load(QString::fromStdU16String(path_.u16string()));

  for (auto* element : document_.elements())
    CreateBindings(*element);
}

std::filesystem::path ModusView3::GetPath() const {
  return path_;
}

void ModusView3::Save(WindowDefinition& definition) {}

bool ModusView3::ShowContainedItem(const scada::NodeId& item_id) {
  // TODO:
  return false;
}

void ModusView3::CreateBindings(Schematic::Element& element) {
  QString binding_strings = element["Tech.keyLink"];
  if (binding_strings.isEmpty())
    return;

  auto& binding = bindings_.emplace_back(element);

  for (const auto& binding_string :
       binding_strings.split('=', Qt::SkipEmptyParts)) {
    const auto& parts = binding_string.split('=');
    if (parts.size() == 1)
      binding.Bind(GetDefaultPropertyName(element), timed_data_service_,
                   parts[0]);
    else if (parts.size() == 2)
      binding.Bind(parts[0], timed_data_service_, parts[1]);
  }

  binding.Update();
}
