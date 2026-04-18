#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

#include "qde/gui/main_window.hpp"

namespace qde::gui {

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent) : QMainWindow{parent} {
  setWindowTitle("QDE");
  resize(1280, 800);

  // Dark palette
  QPalette p;
  p.setColor(QPalette::Window, QColor(30, 30, 30));
  p.setColor(QPalette::WindowText, QColor(212, 212, 212));
  p.setColor(QPalette::Base, QColor(18, 18, 18));
  p.setColor(QPalette::Text, QColor(212, 212, 212));
  p.setColor(QPalette::Button, QColor(45, 45, 45));
  p.setColor(QPalette::ButtonText, QColor(212, 212, 212));
  p.setColor(QPalette::Highlight, QColor(86, 156, 214));
  setPalette(p);

  editor_ = new TextEditor(this);
  circuitView_ = new QuantumCircuitView(this);
  controller_ = new AppController(circuitView_, editor_, this);

  splitter_ = new QSplitter(Qt::Vertical, this);
  splitter_->addWidget(editor_);
  splitter_->addWidget(circuitView_);
  splitter_->setSizes({400, 360});
  splitter_->setStyleSheet(
      "QSplitter::handle { background: #3a3a3a; height: 4px; }");

  setCentralWidget(splitter_);

  setupMenuBar();
  setupStatusBar();

  connect(editor_, &TextEditor::modifiedChanged, this,
          &MainWindow::onModifiedChanged);
  connect(editor_, &TextEditor::fileChanged, this,
          [this](const QString&) { updateTitle(); });
  connect(controller_, &AppController::parseSuccess, this,
          &MainWindow::onParseSuccess);
  connect(controller_, &AppController::parseError, this,
          &MainWindow::onParseFail);

  // Load example program
  editor_->newFile();
  const QString example =
      "OPENQASM 3.0;\n"
      "include \"qelib1.inc\";\n"
      "\n"
      "qreg q[4];\n"
      "creg c[4];\n";

  // Push text directly into editor via document
  editor_->openFile({});
  editor_->document()->setContent(example);
  editor_->syncEditorToDoc();

  controller_->parseNow();
}

void MainWindow::setupMenuBar() {
  auto* fileMenu = menuBar()->addMenu("&File");
  fileMenu->setStyleSheet(
      "QMenu { background: #2d2d2d; color: #ddd; }"
      "QMenu::item:selected { background: #094771; }");

  const QPointer<QAction> newAct = fileMenu->addAction("&New", QKeySequence::New, this, &MainWindow::newFile);
  const QPointer<QAction> openAct = fileMenu->addAction("&Open...",QKeySequence::Open, this, &MainWindow::openFile);
  fileMenu->addSeparator();
  const QPointer<QAction> saveAct = fileMenu->addAction("&Save", QKeySequence::Save, this, &MainWindow::saveFile);
  const QPointer<QAction> saveAsAct =
      fileMenu->addAction("Save &As…", this, &MainWindow::saveFileAs);
  fileMenu->addSeparator();
  fileMenu->addAction("&Quit", this, &QWidget::close, QKeySequence::Quit);

  Q_UNUSED(newAct)
  Q_UNUSED(openAct)
  Q_UNUSED(saveAct)
  Q_UNUSED(saveAsAct)

  auto* viewMenu = menuBar()->addMenu("&View");
  viewMenu->setStyleSheet(fileMenu->styleSheet());
  auto* splitAct = viewMenu->addAction("Toggle Layout");
  connect(splitAct, &QAction::triggered, this, [this] {
    splitter_->setOrientation(splitter_->orientation() == Qt::Vertical
                                  ? Qt::Horizontal
                                  : Qt::Vertical);
  });

  menuBar()->setStyleSheet(
      "QMenuBar { background: #1e1e1e; color: #ccc; }"
      "QMenuBar::item:selected { background: #094771; }");
}

void MainWindow::setupStatusBar() {
  statusLabel_ = new QLabel("Ready", this);
  statusLabel_->setStyleSheet("color: #aaa; padding: 0 6px;");
  statusBar()->addWidget(statusLabel_);
  statusBar()->setStyleSheet("QStatusBar { background: #1e1e1e; }");
}

void MainWindow::newFile() {
  if (editor_->isModified()) {
    auto btn = QMessageBox::question(this, "Unsaved changes",
                                     "Discard unsaved changes?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;
  }
  editor_->newFile();
  updateTitle();
}

void MainWindow::openFile() {
  QString path = QFileDialog::getOpenFileName(
      this, "Open QASM File", {}, "QASM Files (*.qasm *.qasm2);;All Files (*)");
  if (path.isEmpty()) return;
  editor_->openFile(path);
  updateTitle();
}

void MainWindow::saveFile() {
  editor_->saveFile();
  updateTitle();
}

void MainWindow::saveFileAs() {
  QString path = QFileDialog::getSaveFileName(
      this, "Save As", {}, "QASM Files (*.qasm);;All Files (*)");
  if (!path.isEmpty()) {
    editor_->saveFileAs(path);
    updateTitle();
  }
}

void MainWindow::onParseSuccess() {
  statusLabel_->setText("Parsed successfully");
  statusLabel_->setStyleSheet(
      "color: #4CAF50; padding: 0 6px;");  // Green color for success
}

void MainWindow::onParseFail(const QStringList& errors) {
  statusLabel_->setText(QString("Syntax Error: %1 errors").arg(errors.size()));
  statusLabel_->setStyleSheet(
      "color: #F44336; padding: 0 6px;");  // Red color for failure
}

void MainWindow::onModifiedChanged(bool modified) { updateTitle(); }

void MainWindow::updateTitle() {
  QString title = "QDE";
  if (!editor_->filePath().isEmpty()) title += " — " + editor_->filePath();
  if (editor_->isModified()) title += " *";
  setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (editor_->isModified()) {
    auto btn = QMessageBox::question(this, "Unsaved changes",
                                     "Discard unsaved changes and quit?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) {
      event->ignore();
      return;
    }
  }
  event->accept();
}

}  // namespace qde::gui