#pragma once

#include <QApplication>

#include "client_application.h"

class ClientApplicationQt : public QApplication,
                            public ClientApplication {
 public:
  ClientApplicationQt(int argc, char** argv);

  // ClientApplication
  virtual bool Init() override;
  virtual int Run(int show) override;
  virtual void Quit() override;

 protected:
  // ClientApplication
  virtual std::unique_ptr<MainWindow> CreateMainWindow(MainWindowContext&& context) override;
  virtual bool ShowLoginDialogImpl(const DataServicesContext& context, DataServices& services) override;
};

extern ClientApplicationQt* g_application_qt;
