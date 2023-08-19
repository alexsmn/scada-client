#pragma once

#include "test/display_tester/qt/display_tester_state.h"
#include "test/display_tester/qt/variable_table_controller.h"

#include <QFileDialog>
#include <QSplitter>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

class DisplayTesterWindow : public QWidget {
 public:
  // An empty path is passed for a default view.
  using ViewFactory =
      std::function<QWidget*(const std::filesystem::path& path)>;

  DisplayTesterWindow(DisplayTesterState& state, ViewFactory view_factory)
      : state_{state}, view_factory_{std::move(view_factory)} {
    setLayout(new QVBoxLayout);

    toolbar->setMaximumHeight(20);
    layout()->addWidget(toolbar);

    splitter->setOrientation(Qt::Horizontal);
    layout()->addWidget(splitter);

    splitter->addWidget(variable_table);

    toolbar->addAction("Open Default", [this] {
      opened_view_ = view_factory_({});
      AddView(*opened_view_);
    });

    toolbar->addAction("Open...", [this] {
      if (auto path = QFileDialog::getOpenFileName(this); !path.isEmpty()) {
        opened_view_ = view_factory_(path.toStdWString());
        AddView(*opened_view_);
      }
    });

    toolbar->addAction("Close", [this] {
      if (opened_view_) {
        opened_view_->deleteLater();
        opened_view_ = nullptr;
      }
    });

    toolbar->addAction("Variables",
                       [this] { variable_table_controller_.Load(); });

    resize(1000, 500);
  }

  void AddView(QWidget& widget) {
    widget.setParent(splitter);
    splitter->addWidget(&widget);
  }

  QToolBar* toolbar = new QToolBar{this};
  QSplitter* splitter = new QSplitter{this};
  QTableWidget* variable_table = new QTableWidget{this};

 private:
  DisplayTesterState& state_;
  const ViewFactory view_factory_;

  VariableTableController variable_table_controller_{*variable_table,
                                                     state_.variable_storage};

  QWidget* opened_view_ = nullptr;
};
