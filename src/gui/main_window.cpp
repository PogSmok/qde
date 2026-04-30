#include <QAction>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include "qde/gui/main_window.hpp"
#include "qde/gui/shortcut_manager.hpp"
#include "qde/gui/theme.hpp"

namespace qde::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow{parent},
      editor_(new TextEditor(this)),
      circuitView_(new QuantumCircuitView(this)),
      controller_(new AppController(circuitView_, editor_, this)),
      splitter_(new QSplitter(Qt::Vertical, this)) {
  setWindowTitle("QDE");
  resize(1280, 800);

  QPalette p;
  p.setColor(QPalette::Window, theme::kWindowBackground);
  p.setColor(QPalette::WindowText, theme::kWindowText);
  p.setColor(QPalette::Base, theme::kBaseBackground);
  p.setColor(QPalette::Text, theme::kTextColor);
  p.setColor(QPalette::Button, theme::kButtonBackground);
  p.setColor(QPalette::ButtonText, theme::kButtonText);
  p.setColor(QPalette::Highlight, theme::kHighlightBackground);
  setPalette(p);

  splitter_->addWidget(editor_);
  splitter_->addWidget(circuitView_);
  splitter_->setSizes({400, 360});
  splitter_->setStyleSheet(
      QString("QSplitter::handle { background: %1; height: 4px; }")
          .arg(theme::kSplitterHandleBackground));

  setCentralWidget(splitter_);

  setupMenuBar();
  setupStatusBar();
  setupActions();

  connect(editor_, &TextEditor::modifiedChanged, this,
          &MainWindow::onModifiedChanged);
  connect(editor_, &TextEditor::fileChanged, this,
          [this](const QString&) { updateTitle(); });
  connect(controller_, &AppController::parseSuccess, this,
          &MainWindow::onParseSuccess);
  connect(controller_, &AppController::parseError, this,
          &MainWindow::onParseFail);
}

void MainWindow::setupMenuBar() {
  auto* fileMenu = menuBar()->addMenu("&File");
  fileMenu->setStyleSheet(QString("QMenu { background: %1; color: %2; }"
                                  "QMenu::item:selected { background: %3; }")
                              .arg(theme::kMenuBackground, theme::kMenuText,
                                   theme::kMenuSelectedBackground));

  const QPointer<QAction> newAct = fileMenu->addAction(
      "&New", QKeySequence::New, this, &MainWindow::newFile);
  const QPointer<QAction> openAct = fileMenu->addAction(
      "&Open...", QKeySequence::Open, this, &MainWindow::openFile);
  fileMenu->addSeparator();
  const QPointer<QAction> saveAct = fileMenu->addAction(
      "&Save", QKeySequence::Save, this, &MainWindow::saveFile);
  const QPointer<QAction> saveAsAct =
      fileMenu->addAction("Save &As…", this, &MainWindow::saveFileAs);
  fileMenu->addSeparator();
  fileMenu->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

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
      QString("QMenuBar { background: %1; color: %2; }"
              "QMenuBar::item:selected { background: %3; }")
          .arg(theme::kMenuBarBackground, theme::kMenuBarText,
               theme::kMenuSelectedBackground));
}

void MainWindow::setupStatusBar() {
  statusLabel_ = new QLabel("Ready", this);
  statusLabel_->setStyleSheet(
      QString("color: %1; padding: 0 6px;").arg(theme::kStatusBarText));
  statusBar()->addWidget(statusLabel_);
  statusBar()->setStyleSheet(QString("QStatusBar { background: %1; }")
                                 .arg(theme::kStatusBarBackground));
}

void MainWindow::setupActions() {
  auto& sm = ShortcutManager::instance();

  createAction("New File", sm.get("file.new"), &MainWindow::newFile);
  createAction("Open File", sm.get("file.open"), &MainWindow::openFile);
  createAction("Save File", sm.get("file.save"), &MainWindow::saveFile);
  createAction("Save File As", sm.get("file.save_as"), &MainWindow::saveFileAs);

  createAction("Indent block", sm.get("edit.indent"), editor_,
               &TextEditor::indentBlock);
  createAction("Outdent block", sm.get("edit.outdent"), editor_,
               &TextEditor::outdentBlock);
  createAction("Comment block", sm.get("edit.comment"), editor_,
               &TextEditor::toggleComment);
  createAction("Move block up", sm.get("edit.move_block_up"), editor_,
               &TextEditor::moveBlockUp);
  createAction("Move block down", sm.get("edit.move_block_down"), editor_,
               &TextEditor::moveBlockDown);
}

template <typename Func>
void MainWindow::createAction(const QString& text, const QKeySequence& ks,
                              Func slot) {
  auto* action = new QAction(text, this);
  action->setShortcut(ks);
  action->setShortcutContext(Qt::WindowShortcut);
  this->addAction(action);
  connect(action, &QAction::triggered, this, slot);
}

template <typename ObjPtr, typename Func>
void MainWindow::createAction(const QString& text, const QKeySequence& ks,
                              ObjPtr obj, Func slot) {
  auto* action = new QAction(text, this);
  action->setShortcut(ks);
  action->setShortcutContext(Qt::WindowShortcut);
  this->addAction(action);
  connect(action, &QAction::triggered, obj, slot);
}

void MainWindow::newFile() {
  if (editor_->isModified()) {
    auto btn = QMessageBox::question(this, "Unsaved changes",
                                     "Discard unsaved changes?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) {
      return;
    }
  }
  editor_->newFile();
  updateTitle();
}

void MainWindow::openFile() {
  QString path = QFileDialog::getOpenFileName(
      this, "Open QASM File", {}, "QASM Files (*.qasm *.qasm2);;All Files (*)");
  if (path.isEmpty()) {
    return;
  }
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
    bool isOk = editor_->saveFileAs(path);
    if (isOk) {
      updateTitle();
    }
    // TODO: Handle error
  }
}

void MainWindow::onParseSuccess() const {
  statusLabel_->setText("Parsed successfully");
  statusLabel_->setStyleSheet(
      QString("color: %1; padding: 0 6px;").arg(theme::kSuccessText));
}

void MainWindow::onParseFail(const QStringList& errors) const {
  statusLabel_->setText(QString("Syntax Error: %1 errors").arg(errors.size()));
  statusLabel_->setStyleSheet(
      QString("color: %1; padding: 0 6px;").arg(theme::kErrorText));
}

void MainWindow::onModifiedChanged(bool modified) { updateTitle(); }

void MainWindow::updateTitle() {
  QString title = "QDE";
  if (!editor_->filePath().isEmpty()) {
    title += " — " + editor_->filePath();
  }
  if (editor_->isModified()) {
    title += " *";
  }
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