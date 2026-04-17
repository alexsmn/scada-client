#include "window_definition_util.h"

#include "base/json.h"
#include "resources/common_resources.h"
#include "controller/controller_mock.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

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

boost::json::value MakeListValue(std::initializer_list<int> values) {
  boost::json::array list;
  list.reserve(values.size());
  for (auto value : values)
    list.emplace_back(value);
  return boost::json::value{std::move(list)};
}

boost::json::value MakeDictValue(
    std::initializer_list<std::pair<std::string, int>> key_values) {
  boost::json::object obj;
  for (auto&& [key, value] : key_values) {
    obj[key] = value;
  }
  return boost::json::value{std::move(obj)};
}

const WindowItems kTestWindowItems = {
    {"empty", boost::json::value{}},
    {"int", boost::json::value{123}},
    {"int", boost::json::value{321}},
    {"string", boost::json::value{"text"}},
    {"string", boost::json::value{"more text"}},
    {"list", MakeListValue({1, 2, 3})},
    {"dict", MakeDictValue({{"a", 1}, {"b", 2}, {"c", 3}})},
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
boost::json::value ValueOf(T&& value) {
  return boost::json::value{std::forward<T>(value)};
}

template <>
boost::json::value ValueOf(boost::json::value&& value) {
  return std::move(value);
}

boost::json::value DictOf(std::string key1, int val1,
                           std::string key2, int val2) {
  boost::json::object obj;
  obj[key1] = val1;
  obj[key2] = val2;
  return boost::json::value{std::move(obj)};
}

boost::json::value ListOf(boost::json::value v1, boost::json::value v2) {
  boost::json::array arr;
  arr.emplace_back(std::move(v1));
  arr.emplace_back(std::move(v2));
  return boost::json::value{std::move(arr)};
}

boost::json::value DictOf(std::string key1, boost::json::value val1) {
  boost::json::object obj;
  obj[key1] = std::move(val1);
  return boost::json::value{std::move(obj)};
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
  EXPECT_FALSE(FromJson<scada::NodeId>(boost::json::value()));
  EXPECT_FALSE(FromJson<scada::NodeId>(boost::json::value(123)));
  EXPECT_FALSE(FromJson<scada::NodeId>(boost::json::value("abcdef")));
  EXPECT_EQ(scada::NodeId(123, 1),
            FromJson<scada::NodeId>(boost::json::value("TS.123")));
}

TEST(ToJson, NodeId) {
  EXPECT_EQ(boost::json::value(), ToJson(scada::NodeId()));
  EXPECT_EQ(boost::json::value("TS.123"), ToJson(scada::NodeId(123, 1)));
}

TEST(FromJson, WindowItems) {
  auto json = LoadJsonFromString(kTestWindowItemsJson);
  ASSERT_TRUE(json);

  auto window_items = FromJson<WindowItems>(*json);

  EXPECT_EQ(kTestWindowItems, window_items);
}

TEST(ToJson, WindowItems) {
  auto json = ToJson(kTestWindowItems);

  auto expected_json = LoadJsonFromString(kTestWindowItemsJson);
  ASSERT_TRUE(expected_json);

  EXPECT_EQ(*expected_json, json);
}

TEST(FromJson, WindowDefinition) {
  auto json = LoadJsonFromString(kTestWindowDefinitionJson);
  ASSERT_TRUE(json);

  auto window_definition = FromJson<WindowDefinition>(*json);

  EXPECT_EQ(MakeTestWindowDefinition(), window_definition);
}

TEST(ToJson, WindowDefinition) {
  auto json = ToJson(MakeTestWindowDefinition());

  auto expected_json = LoadJsonFromString(kTestWindowDefinitionJson);
  ASSERT_TRUE(expected_json);

  EXPECT_EQ(*expected_json, json);
}

TEST(FromToJson, TimeRange_Day) {
  TimeRange time_range{TimeRange::Type::Day};
  EXPECT_EQ(time_range, FromJson<TimeRange>(ToJson(time_range)));
}

TEST(FromToJson, TimeRange_Interval) {
  TimeRange time_range{base::TimeDelta::FromHours(3)};
  EXPECT_EQ(time_range, FromJson<TimeRange>(ToJson(time_range)));
}
