#include <QApplication>
#include <QIcon>

#include "qde/gui/config_manager.hpp"
#include "qde/gui/main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication const app(argc, argv);
  QApplication::setApplicationDisplayName("Quantum Development Environment");
  QApplication::setWindowIcon(QIcon(":/resources/app_icon.ico"));

  // auto& cfg = qde::gui::config::ConfigManager::Instance();
  qde::gui::config::ConfigManager::Load();
  qde::gui::config::ConfigManager::Save();

  qde::gui::MainWindow window;
  window.show();
  return QApplication::exec();
}