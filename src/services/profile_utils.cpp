#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "client/base/xml.h"

const base::char16* kTypeStrings[] = { L"NULL",
                                 L"BOOLEAN",
                                 L"INTEGER",
                                 L"DOUBLE",
                                 L"STRING",
                                 L"BINARY",
                                 L"DICTIONARY",
                                 L"LIST" };

void SaveValueAsXml(const base::Value& value, xml::Node& node) {
  node.SetAttribute("type", kTypeStrings[value.GetType()]);

  switch (value.GetType()) {
    case base::Value::TYPE_NULL:
      break;

    case base::Value::TYPE_BOOLEAN: {
      bool v = false;
      value.GetAsBoolean(&v);
      node.set_text(v ? L"1" : L"0");
      break;
    }

    case base::Value::TYPE_INTEGER: {
      int v = 0;
      value.GetAsInteger(&v);
      node.set_text(base::IntToString16(v).c_str());
      break;
    }

    case base::Value::TYPE_STRING: {
      base::string16 v;
      value.GetAsString(&v);
      node.set_text(v.c_str());
      break;
    }

    case base::Value::TYPE_DICTIONARY: {
      const base::DictionaryValue* dict = NULL;
      if (value.GetAsDictionary(&dict)) {
        for (base::DictionaryValue::Iterator i(*dict); !i.IsAtEnd(); i.Advance()) {
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

base::Value* LoadValueFromXml(const xml::Node& node) {
  base::string16 type_string = node.GetAttribute("type");

  if (type_string == kTypeStrings[base::Value::TYPE_BOOLEAN]) {
    bool v = node.get_text() == L"1";
    return new base::FundamentalValue(v);

  } else if (type_string == kTypeStrings[base::Value::TYPE_INTEGER]) {
    int v = 0;
    return base::StringToInt(node.get_text(), &v) ?
        new base::FundamentalValue(v) : NULL;

  } else if (type_string == kTypeStrings[base::Value::TYPE_STRING]) {
    return new base::StringValue(node.get_text());

  } else if (type_string == kTypeStrings[base::Value::TYPE_DICTIONARY]) {
    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
    for (const xml::Node* child = node.first_child; child; child = child->next)
      dict->Set(child->name, LoadValueFromXml(*child));
    return dict.release();

  } else {
    NOTIMPLEMENTED();
    return NULL;
  }
}
