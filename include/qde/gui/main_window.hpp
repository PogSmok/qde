#ifndef GUI_MAIN_WINDOW_HPP_
#define GUI_MAIN_WINDOW_HPP_

#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QSplitter>

#include "qde/gui/app_controller.hpp"
#include "qde/gui/quantum_circuit_view.hpp"
#include "qde/gui/text_editor.hpp"

namespace qde::gui {

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

}  // namespace qge::gui

#endif  // MAIN_WINDOW_HPP_
