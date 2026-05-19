
#include <QApplication>
#include <QStyleFactory>

#include "qde/gui/main_window.hpp"
#include "qde/gui/config_manager.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationDisplayName("Quantum Development Environment");

  auto& cfg = qde::gui::config::ConfigManager::instance();
  cfg.load();
  cfg.save();

  qde::gui::MainWindow window;
  window.show();
  return QApplication::exec();
}