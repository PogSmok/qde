#include <QAction>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include "qde/gui/config.hpp"
#include "qde/gui/main_window.hpp"
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
  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->setStyleSheet(QString("QMenu { background: %1; color: %2; }"
                                   "QMenu::item:selected { background: %3; }")
                               .arg(theme::kMenuBackground, theme::kMenuText,
                                    theme::kMenuSelectedBackground));

  const QPointer<QAction> new_act = file_menu->addAction(
      "&New", QKeySequence::New, this, &MainWindow::newFile);
  const QPointer<QAction> open_act = file_menu->addAction(
      "&Open...", QKeySequence::Open, this, &MainWindow::openFile);
  file_menu->addSeparator();
  const QPointer<QAction> save_act = file_menu->addAction(
      "&Save", QKeySequence::Save, this, &MainWindow::saveFile);
  const QPointer<QAction> save_as_act =
      file_menu->addAction("Save &As…", this, &MainWindow::saveFileAs);
  file_menu->addSeparator();
  file_menu->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

  Q_UNUSED(new_act)
  Q_UNUSED(open_act)
  Q_UNUSED(save_act)
  Q_UNUSED(save_as_act)

  auto* view_menu = menuBar()->addMenu("&View");
  view_menu->setStyleSheet(file_menu->styleSheet());
  auto* split_act = view_menu->addAction("Toggle Layout");
  connect(split_act, &QAction::triggered, this, [this] {
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
  createAction("New File", config::shortcuts::fileNew.Value(),
               &MainWindow::newFile);
  createAction("Open File", config::shortcuts::fileOpen.Value(),
               &MainWindow::openFile);
  createAction("Save File", config::shortcuts::fileSave.Value(),
               &MainWindow::saveFile);
  createAction("Save File As", config::shortcuts::fileSaveAs.Value(),
               &MainWindow::saveFileAs);

  createAction("Indent block", config::shortcuts::editIndent.Value(), editor_,
               &TextEditor::indentBlock);
  createAction("Outdent block", config::shortcuts::editOutdent.Value(), editor_,
               &TextEditor::outdentBlock);
  createAction("Comment block", config::shortcuts::editComment.Value(), editor_,
               &TextEditor::toggleComment);
  createAction("Move block up", config::shortcuts::editMoveBlockUp.Value(),
               editor_, &TextEditor::moveBlockUp);
  createAction("Move block down", config::shortcuts::editMoveBlockDown.Value(),
               editor_, &TextEditor::moveBlockDown);
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
    bool is_ok = editor_->saveFileAs(path);
    if (is_ok) {
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