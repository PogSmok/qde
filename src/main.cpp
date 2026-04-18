#include <QApplication>
#include <QStyleFactory>

#include "qde/gui/main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationDisplayName("Quantum Development Environment");
  app.setOrganizationName("StormWave");

  qde::gui::MainWindow window;
  window.show();
  return app.exec();
}