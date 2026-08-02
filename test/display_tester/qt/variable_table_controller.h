#pragma once

#include <QTableWidget>

class VariableTableController {
 public:
  VariableTableController(QTableWidget& table,
                          VariableStorage& variable_storage)
      : table_{table}, variable_storage_{variable_storage} {
    table_.setColumnCount(2);
    table_.setHorizontalHeaderLabels({"Name", "Value"});

    QObject::connect(
        &table_, &QTableWidget::itemChanged, [&](QTableWidgetItem* item) {
          if (!loading_ && item->column() == 1) {
            auto name = table_.item(item->row(), 0)->text().toStdString();
            auto new_value = item->text().toInt();
            variable_storage_.SetVariableValue(name, new_value);
          }
        });
  }

  void Load() {
    loading_ = true;

    table_.clear();

    int index = 0;
    for (const auto& [name, data_value] : variable_storage_.variable_values) {
      table_.insertRow(index);
      table_.setItem(index, 0,
                     new QTableWidgetItem{QString::fromStdString(name)});
      table_.setItem(index, 1,
                     new QTableWidgetItem{
                         QString::fromStdString(ToString(data_value.value))});
      index++;
    }

    loading_ = false;
  }

 private:
  QTableWidget& table_;
  VariableStorage& variable_storage_;

  bool loading_ = false;
};
