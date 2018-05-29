#pragma once

#include <memory>

namespace base {
class Value;
}

namespace xml {
class Node;
}

void SaveValueAsXml(const base::Value& value, xml::Node& node);
std::unique_ptr<base::Value> LoadValueFromXml(const xml::Node& node);
