#pragma once

#include <boost/json.hpp>
#include <QMap>
#include <QString>
#include <QVariant>

boost::json::value ToJson(const QVariant& v);
QVariant ToQVariant(const boost::json::value& v);

inline boost::json::value ToJson(const QMap<QString, QVariant>& m) {
  boost::json::object obj;
  for (const auto& [key, value] : m.toStdMap()) {
    obj[key.toStdString()] = ToJson(value);
  }
  return boost::json::value{std::move(obj)};
}

inline QMap<QString, QVariant> ToQMap(const boost::json::value& v) {
  QMap<QString, QVariant> map;
  if (v.is_object()) {
    for (const auto& [key, value] : v.as_object()) {
      map.insert(QString::fromStdString(std::string{key}), ToQVariant(value));
    }
  }
  return map;
}

inline boost::json::value ToJson(const QVariant& v) {
  switch (v.type()) {
    case QVariant::Bool:
      return boost::json::value{v.toBool()};
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
      return boost::json::value{v.toInt()};
    case QVariant::Double:
      return boost::json::value{v.toDouble()};
    case QVariant::String:
      return boost::json::value{v.toString().toStdString()};
    case QVariant::List: {
      boost::json::array list;
      for (const auto& item : v.toList()) {
        list.emplace_back(ToJson(item));
      }
      return boost::json::value{std::move(list)};
    }
    case QVariant::Map: {
      return ToJson(v.toMap());
    }
    default:
      return boost::json::value{};
  }
}

inline QVariant ToQVariant(const boost::json::value& v) {
  if (v.is_bool())
    return QVariant{v.as_bool()};
  if (v.is_int64())
    return QVariant{static_cast<int>(v.as_int64())};
  if (v.is_double())
    return QVariant{v.as_double()};
  if (v.is_string())
    return QVariant{QString::fromStdString(std::string{v.as_string()})};
  if (v.is_array()) {
    QVariantList list;
    for (const auto& item : v.as_array()) {
      list.append(ToQVariant(item));
    }
    return QVariant{list};
  }
  if (v.is_object()) {
    return QVariant{ToQMap(v)};
  }
  return QVariant{};
}
