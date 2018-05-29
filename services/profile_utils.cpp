#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "base/xml.h"

#include <memory>

const base::char16* kTypeStrings[] = {L"NULL",       L"BOOLEAN", L"INTEGER",
                                      L"DOUBLE",     L"STRING",  L"BINARY",
                                      L"DICTIONARY", L"LIST"};

void SaveValueAsXml(const base::Value& value, xml::Node& node) {
  node.SetAttribute("type", kTypeStrings[static_cast<size_t>(value.type())]);

  switch (value.type()) {
    case base::Value::Type::NONE:
      break;

    case base::Value::Type::BOOLEAN: {
      bool v = false;
      value.GetAsBoolean(&v);
      node.set_text(v ? L"1" : L"0");
      break;
    }

    case base::Value::Type::INTEGER: {
      int v = 0;
      value.GetAsInteger(&v);
      node.set_text(base::IntToString16(v).c_str());
      break;
    }

    case base::Value::Type::STRING: {
      base::string16 v;
      value.GetAsString(&v);
      node.set_text(v.c_str());
      break;
    }

    case base::Value::Type::DICTIONARY: {
      const base::DictionaryValue* dict = NULL;
      if (value.GetAsDictionary(&dict)) {
        for (base::DictionaryValue::Iterator i(*dict); !i.IsAtEnd();
             i.Advance()) {
          xml::Node& child = node.AddElement(i.key());
          SaveValueAsXml(i.value(), child);
        }
      }
      break;
    }

    default:
      NOTIMPLEMENTED();
      break;
  }
}

std::unique_ptr<base::Value> LoadValueFromXml(const xml::Node& node) {
  base::string16 type_string = node.GetAttribute("type");

  if (type_string ==
      kTypeStrings[static_cast<size_t>(base::Value::Type::BOOLEAN)]) {
    bool v = node.get_text() == L"1";
    return std::make_unique<base::Value>(v);

  } else if (type_string ==
             kTypeStrings[static_cast<size_t>(base::Value::Type::INTEGER)]) {
    int v = 0;
    return base::StringToInt(node.get_text(), &v)
               ? std::make_unique<base::Value>(v)
               : nullptr;

  } else if (type_string ==
             kTypeStrings[static_cast<size_t>(base::Value::Type::STRING)]) {
    return std::make_unique<base::Value>(node.get_text());

  } else if (type_string ==
             kTypeStrings[static_cast<size_t>(base::Value::Type::DICTIONARY)]) {
    auto dict = std::make_unique<base::DictionaryValue>();
    for (const xml::Node* child = node.first_child; child; child = child->next)
      dict->Set(child->name, LoadValueFromXml(*child));
    return dict;

  } else {
    NOTIMPLEMENTED();
    return NULL;
  }
}
