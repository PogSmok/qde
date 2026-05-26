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

  SetupMenuBar();
  SetupStatusBar();
  SetupActions();

  connect(editor_, &TextEditor::ModifiedChanged, this,
          &MainWindow::OnModifiedChanged);
  connect(editor_, &TextEditor::FileChanged, this,
          [this](const QString&) { UpdateTitle(); });
  connect(controller_, &AppController::ParseSuccess, this,
          &MainWindow::OnParseSuccess);
  connect(controller_, &AppController::ParseError, this,
          &MainWindow::OnParseFail);
}

void MainWindow::SetupMenuBar() {
  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->setStyleSheet(QString("QMenu { background: %1; color: %2; }"
                                   "QMenu::item:selected { background: %3; }")
                               .arg(theme::kMenuBackground, theme::kMenuText,
                                    theme::kMenuSelectedBackground));

  const QPointer<QAction> new_act = file_menu->addAction(
      "&New", QKeySequence::New, this, &MainWindow::NewFile);
  const QPointer<QAction> open_act = file_menu->addAction(
      "&Open...", QKeySequence::Open, this, &MainWindow::OpenFile);
  file_menu->addSeparator();
  const QPointer<QAction> save_act = file_menu->addAction(
      "&Save", QKeySequence::Save, this, &MainWindow::SaveFile);
  const QPointer<QAction> save_as_act =
      file_menu->addAction("Save &As…", this, &MainWindow::SaveFileAs);
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

void MainWindow::SetupStatusBar() {
  statusLabel_ = new QLabel("Ready", this);
  statusLabel_->setStyleSheet(
      QString("color: %1; padding: 0 6px;").arg(theme::kStatusBarText));
  statusBar()->addWidget(statusLabel_);
  statusBar()->setStyleSheet(QString("QStatusBar { background: %1; }")
                                 .arg(theme::kStatusBarBackground));
}

void MainWindow::SetupActions() {
  CreateAction("New File", config::shortcuts::file_new.Value(),
               &MainWindow::NewFile);
  CreateAction("Open File", config::shortcuts::file_open.Value(),
               &MainWindow::OpenFile);
  CreateAction("Save File", config::shortcuts::file_save.Value(),
               &MainWindow::SaveFile);
  CreateAction("Save File As", config::shortcuts::file_save_as.Value(),
               &MainWindow::SaveFileAs);

  CreateAction("Indent block", config::shortcuts::edit_indent.Value(), editor_,
               &TextEditor::IndentBlock);
  CreateAction("Outdent block", config::shortcuts::edit_outdent.Value(),
               editor_, &TextEditor::OutdentBlock);
  CreateAction("Comment block", config::shortcuts::edit_comment.Value(),
               editor_, &TextEditor::ToggleComment);
  CreateAction("Move block up", config::shortcuts::edit_move_block_up.Value(),
               editor_, &TextEditor::MoveBlockUp);
  CreateAction("Move block down",
               config::shortcuts::edit_move_block_down.Value(), editor_,
               &TextEditor::MoveBlockDown);
}

template <typename Func>
void MainWindow::CreateAction(const QString& text, const QKeySequence& ks,
                              Func slot) {
  auto* action = new QAction(text, this);
  action->setShortcut(ks);
  action->setShortcutContext(Qt::WindowShortcut);
  this->addAction(action);
  connect(action, &QAction::triggered, this, slot);
}

template <typename ObjPtr, typename Func>
void MainWindow::CreateAction(const QString& text, const QKeySequence& ks,
                              const ObjPtr& obj, Func slot) {
  auto* action = new QAction(text, this);
  action->setShortcut(ks);
  action->setShortcutContext(Qt::WindowShortcut);
  this->addAction(action);
  connect(action, &QAction::triggered, obj, slot);
}

void MainWindow::NewFile() {
  if (editor_->IsModified()) {
    auto btn = QMessageBox::question(this, "Unsaved changes",
                                     "Discard unsaved changes?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) {
      return;
    }
  }
  editor_->NewFile();
  UpdateTitle();
}

void MainWindow::OpenFile() {
  QString const path = QFileDialog::getOpenFileName(
      this, "Open QASM File", {}, "QASM Files (*.qasm *.qasm2);;All Files (*)");
  if (path.isEmpty()) {
    return;
  }
  editor_->OpenFile(path);
  UpdateTitle();
}

void MainWindow::SaveFile() {
  editor_->SaveFile();
  UpdateTitle();
}

void MainWindow::SaveFileAs() {
  QString const path = QFileDialog::getSaveFileName(
      this, "Save As", {}, "QASM Files (*.qasm);;All Files (*)");
  if (!path.isEmpty()) {
    bool const is_ok = editor_->SaveFileAs(path);
    if (is_ok) {
      UpdateTitle();
    }
    // TODO: Handle error
  }
}

void MainWindow::OnParseSuccess() const {
  statusLabel_->setText("Parsed successfully");
  statusLabel_->setStyleSheet(
      QString("color: %1; padding: 0 6px;").arg(theme::kSuccessText));
}

void MainWindow::OnParseFail(const QStringList& errors) const {
  statusLabel_->setText(QString("Syntax Error: %1 errors").arg(errors.size()));
  statusLabel_->setStyleSheet(
      QString("color: %1; padding: 0 6px;").arg(theme::kErrorText));
}

void MainWindow::OnModifiedChanged(bool /*modified*/) { UpdateTitle(); }

void MainWindow::UpdateTitle() {
  QString title = "QDE";
  if (!editor_->FilePath().isEmpty()) {
    title += " — " + editor_->FilePath();
  }
  if (editor_->IsModified()) {
    title += " *";
  }
  setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (editor_->IsModified()) {
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