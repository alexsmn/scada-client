#include "components/object_tree/object_tree_component.h"

#include "component_api.h"
#include "components/object_tree/object_tree_view.h"
#include "controller/controller_registry.h"

REGISTER_CONTROLLER(ObjectTreeView, kObjectTreeWindowInfo);

/*
class ObjectTreeComponent {
 public:
  ObjectTreeComponent(ComponentApi& api) {
    api.RegisterController<ObjectTreeView>(kObjectTreeWindowInfo);
  }
};

REGISTER_COMPONENT(ObjectTreeComponent);
*/

/*void InitObjectTreeComponent(ComponentApi& api) {
  api.RegisterPreference("searchBar.visible", base::Value::Type::BOOLEAN,
                         L"Строка поиска в панели объектов");
}*/
