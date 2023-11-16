#pragma once

base::Value ToJson(const QVariant& v);
QVariant ToQVariant(const base::Value& v);

base::Value ToJson(const QMap<QString, QVariant>& m) {
  base::Value::DictStorage dict;
  for (const auto& [key, value] : m.toStdMap()) {
    dict.try_emplace(key.toStdString(),
                     std::make_unique<base::Value>(ToJson(value)));
  }
  return base::Value{std::move(dict)};
}

QMap<QString, QVariant> ToQMap(const base::Value& v) {
  QMap<QString, QVariant> map;
  for (const auto& [key, value] : v.DictItems()) {
    map.insert(QString::fromStdString(key), ToQVariant(value));
  }
  return map;
}

base::Value ToJson(const QVariant& v) {
  switch (v.type()) {
    case QVariant::Bool:
      return base::Value{v.toBool()};
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
      return base::Value{v.toInt()};
    case QVariant::Double:
      return base::Value{v.toDouble()};
    case QVariant::String:
      return base::Value{v.toString().toStdString()};
    case QVariant::List: {
      base::Value::ListStorage list;
      for (const auto& item : v.toList()) {
        list.emplace_back(ToJson(item));
      }
      return base::Value{std::move(list)};
    }
    case QVariant::Map: {
      return ToJson(v.toMap());
    }
    default:
      return base::Value{};
  }
}

QVariant ToQVariant(const base::Value& v) {
  switch (v.type()) {
    case base::Value::Type::BOOLEAN:
      return QVariant{v.GetBool()};
    case base::Value::Type::INTEGER:
      return QVariant{v.GetInt()};
    case base::Value::Type::DOUBLE:
      return QVariant{v.GetDouble()};
    case base::Value::Type::STRING:
      return QVariant{QString::fromStdString(v.GetString())};
    case base::Value::Type::LIST: {
      QVariantList list;
      for (const auto& item : v.GetList()) {
        list.append(ToQVariant(item));
      }
      return QVariant{list};
    }
    case base::Value::Type::DICTIONARY: {
      return QVariant{ToQMap(v)};
    }
    default:
      return QVariant{};
  }
}
