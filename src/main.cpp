#include <QApplication>
#include <QStyleFactory>

#include "qde/gui/main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationDisplayName("Quantum Development Environment");
  QApplication::setOrganizationName("StormWave");

  qde::gui::MainWindow window;
  window.show();
  return QApplication::exec();
}