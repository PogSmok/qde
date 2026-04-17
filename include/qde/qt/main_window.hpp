#ifndef MAIN_WINDOW_HPP_
#define MAIN_WINDOW_HPP_

#include <QMainWindow>
#include <QLabel>
#include <QSplitter>
#include <QCloseEvent>

#include "qde/qt/app_controller.hpp"
#include "qde/qt/text_editor.hpp"
#include "qde/qt/quantum_circuit_view.hpp"

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
  virtual ~MainWindow();

 protected:
  void closeEvent(QCloseEvent* event) override;

 public slots:
  void newFile();
  void openFile();
  void saveFile();
  void saveFileAs();
  void onParseSuccess();
  void onParseFail(const QStringList& errors);
  void onModifiedChanged(bool modified);

 private:
  void setupMenuBar();
  void setupStatusBar();
  void updateTitle();

  TextEditor* editor_;
  QuantumCircuitView* circuitView_;
  AppController* controller_;
  QSplitter* splitter_;
  QLabel* statusLabel_;
};

#endif  // MAIN_WINDOW_HPP_
