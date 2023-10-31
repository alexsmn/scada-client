#include "window_definition_util.h"

#include "base/json.h"
#include "common_resources.h"
#include "configuration_tree/configuration_tree_component.h"
#include "controller/controller_mock.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

// Register a controller for the `WindowDefinition` test.

const WindowInfo kTestWindowInfo = {.command_id = 99999,
                                    .name = "TestWindowInfo",
                                    .title = u"TestWindowInfo"};

class TestController : public MockController {
 public:
  explicit TestController(const ControllerContext&) {}
};

REGISTER_CONTROLLER(TestController, kTestWindowInfo);

namespace {

template <class T>
base::Value MakeListValue(std::initializer_list<T> values) {
  base::Value::ListStorage list;
  list.reserve(values.size());
  for (auto&& value : values)
    list.emplace_back(std::move(value));
  return base::Value{std::move(list)};
}

template <class T>
base::Value MakeDictValue(
    std::initializer_list<std::pair<std::string, T>> key_values) {
  base::Value::DictStorage dict;
  dict.reserve(key_values.size());
  for (auto&& [key, value] : key_values) {
    dict.try_emplace(std::move(key),
                     std::make_unique<base::Value>(std::move(value)));
  }
  return base::Value{std::move(dict)};
}

const WindowItems kTestWindowItems = {
    {"empty", base::Value{}},
    {"int", base::Value{123}},
    {"int", base::Value{321}},
    {"string", base::Value{"text"}},
    {"string", base::Value{"more text"}},
    {"list", MakeListValue({1, 2, 3})},
    {"dict", MakeDictValue<int>({{"a", 1}, {"b", 2}, {"c", 3}})},
};

const std::string_view kTestWindowItemsJson = R"(
    [
        { "name": "empty",  "@value": null },
        { "name": "int",    "@value": 123 },
        { "name": "int",    "@value": 321 },
        { "name": "string", "@value": "text" },
        { "name": "string", "@value": "more text" },
        { "name": "list",   "@value": [1, 2, 3] },
        { "name": "dict", "a": 1, "b": 2, "c": 3 }
    ])";

template <class T>
base::Value ValueOf(T&& value) {
  return base::Value{std::forward<T>(value)};
}

template <>
base::Value ValueOf(base::Value&& value) {
  return std::move(value);
}

void ListOfHelper(const base::Value::ListStorage&) {}

template <class T, class... Args>
void ListOfHelper(base::Value::ListStorage& storage, T&& item, Args&&... args) {
  storage.emplace_back(std::forward<T>(item));
  ListOfHelper(storage, std::forward<Args>(args)...);
}

template <class... Args>
base::Value ListOf(Args&&... args) {
  base::Value::ListStorage storage;
  ListOfHelper(storage, std::forward<Args>(args)...);
  return base::Value{std::move(storage)};
}

void DictOfHelper(const base::Value::DictStorage&) {}

template <class Key, class Value, class... Args>
void DictOfHelper(base::Value::DictStorage& storage,
                  Key&& key,
                  Value&& value,
                  Args&&... args) {
  storage.emplace(std::forward<Key>(key),
                  std::make_unique<base::Value>(std::forward<Value>(value)));
  DictOfHelper(storage, std::forward<Args>(args)...);
}

template <class... Args>
base::Value DictOf(Args&&... args) {
  base::Value::DictStorage storage;
  DictOfHelper(storage, std::forward<Args>(args)...);
  return base::Value{std::move(storage)};
}

WindowDefinition MakeTestWindowDefinition() {
  WindowDefinition window_definition{kTestWindowInfo};
  window_definition.id = 1;
  window_definition.size = {200, 450};
  window_definition.AddItem("State").Set(DictOf(
      "columns",
      ListOf(DictOf("ix", 0, "size", 200), DictOf("ix", 1, "size", 213))));
  return window_definition;
}

// The `type` must match `kTestWindowInfo.name`.
const std::string_view kTestWindowDefinitionJson = R"(
    {
        "data": null,
        "height": 450,
        "id": 1,
        "items": [ {
          "columns": [ {
              "ix": 0,
              "size": 200
          }, {
              "ix": 1,
              "size": 213
          } ],
          "name": "State"
        } ],
        "locked": false,
        "type": "TestWindowInfo",
        "width": 200
    })";

}  // namespace

TEST(FromJson, NodeId) {
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value()));
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value(123)));
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value("abcdef")));
  EXPECT_EQ(scada::NodeId(123, 1),
            FromJson<scada::NodeId>(base::Value("TS.123")));
}

TEST(ToJson, NodeId) {
  EXPECT_EQ(base::Value(), ToJson(scada::NodeId()));
  EXPECT_EQ(base::Value("TS.123"), ToJson(scada::NodeId(123, 1)));
}

TEST(FromJson, WindowItems) {
  std::optional<base::Value> json = LoadJsonFromString(kTestWindowItemsJson);
  ASSERT_TRUE(json);

  auto window_items = FromJson<WindowItems>(*json);

  EXPECT_EQ(kTestWindowItems, window_items);
}

TEST(ToJson, WindowItems) {
  auto json = ToJson(kTestWindowItems);

  std::optional<base::Value> expected_json =
      LoadJsonFromString(kTestWindowItemsJson);
  ASSERT_TRUE(expected_json);

  EXPECT_EQ(*expected_json, json);
}

TEST(FromJson, WindowDefinition) {
  std::optional<base::Value> json =
      LoadJsonFromString(kTestWindowDefinitionJson);
  ASSERT_TRUE(json);

  auto window_definition = FromJson<WindowDefinition>(*json);

  EXPECT_EQ(MakeTestWindowDefinition(), window_definition);
}

TEST(ToJson, WindowDefinition) {
  auto json = ToJson(MakeTestWindowDefinition());

  std::optional<base::Value> expected_json =
      LoadJsonFromString(kTestWindowDefinitionJson);
  ASSERT_TRUE(expected_json);

  EXPECT_EQ(*expected_json, json);
}
