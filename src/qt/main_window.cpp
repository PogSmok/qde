#include "qde/qt/main_window.hpp"

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent) : QMainWindow{parent} {
 // TODO: implement
}

void MainWindow::newFile() {
 // TODO: implement
}

void MainWindow::openFile() {
 // TODO: implement
}

void MainWindow::saveFile() {
 // TODO: implement
}

void MainWindow::saveFileAs() {
 // TODO: implement
}

void MainWindow::onParseSuccess() {
 // TODO: implement
}

void MainWindow::onParseFail(const QStringList& errors) {
 // TODO: implement
}

void MainWindow::onModifiedChanged(bool modified) {
 // TODO: implement
}

void MainWindow::setupMenuBar() {
 // TODO: implement
}

void MainWindow::setupStatusBar() {
 // TODO: implement
}

void MainWindow::updateTitle() {
 // TODO: implement
}

void MainWindow::closeEvent(QCloseEvent* event) {
 event->accept();
}
