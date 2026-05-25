#include <QApplication>
#include <QStyleFactory>

#include "qde/gui/config_manager.hpp"
#include "qde/gui/main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationDisplayName("Quantum Development Environment");

  auto& cfg = qde::gui::config::ConfigManager::Instance();
  cfg.Load();
  cfg.Save();

  qde::gui::MainWindow window;
  window.show();
  return QApplication::exec();
}