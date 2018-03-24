#pragma once

namespace base {
class Value;
}

namespace xml {
class Node;
}

void SaveValueAsXml(const base::Value& value, xml::Node& node);
base::Value* LoadValueFromXml(const xml::Node& node);
