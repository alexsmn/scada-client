#pragma once

#include "base/lifetime.h"
#include "common/vds_runtime_api.h"
#include "vds_runtime/qt/vds_runtime_loader.h"

#include <QImage>
#include <QWidget>

#include <filesystem>
#include <functional>
#include <memory>

class VdsRuntimeWidget : public QWidget {
  Q_OBJECT

 public:
  explicit VdsRuntimeWidget(QWidget* parent = nullptr);
  ~VdsRuntimeWidget() override;

  bool Open(const std::filesystem::path& path,
            int32_t kind = TC_VDS_RUNTIME_DOCUMENT_KIND_AUTO);
  const QString& error_message() const SCADA_LIFETIME_BOUND {
    return error_message_;
  }
  const std::filesystem::path& path() const SCADA_LIFETIME_BOUND {
    return path_;
  }
  QString title() const { return title_; }

  using SelectionCallback = std::function<void(QString data_source)>;
  void set_selection_callback(SelectionCallback callback) {
    selection_callback_ = std::move(callback);
  }

  using DoubleClickCallback = std::function<void()>;
  void set_double_click_callback(DoubleClickCallback callback) {
    double_click_callback_ = std::move(callback);
  }

  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;

 private:
  QString FormatError(const QString& title,
                      const TcVdsRuntimeError& error) const;
  QPointF WidgetToPage(const QPoint& point) const;

  VdsRuntimeLoader loader_;
  TcVdsRuntimeDocument document_ = nullptr;
  TcVdsRuntimeDocumentInfo document_info_{};
  std::filesystem::path path_;
  QString title_;
  QString error_message_;

  SelectionCallback selection_callback_;
  DoubleClickCallback double_click_callback_;
};
