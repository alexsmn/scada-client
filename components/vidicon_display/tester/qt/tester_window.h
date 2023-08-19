#pragma once

#include "components/vidicon_display/tester/qt/variable_table_controller.h"

#include <QSplitter>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

class TesterWindow : public QWidget {
 public:
  explicit TesterWindow(TesterState& state) : state{state} {
    setLayout(new QVBoxLayout);

    toolbar->setMaximumHeight(20);
    layout()->addWidget(toolbar);

    splitter->setOrientation(Qt::Horizontal);
    layout()->addWidget(splitter);

    splitter->addWidget(variable_table);

    toolbar->addAction("Variables", [&] { variable_table_controller.Load(); });

    resize(1000, 500);
  }

  TesterState& state;

  QToolBar* toolbar = new QToolBar{this};
  QSplitter* splitter = new QSplitter{this};
  QTableWidget* variable_table = new QTableWidget{this};

  VariableTableController variable_table_controller{*variable_table,
                                                    state.variable_storage};
};
