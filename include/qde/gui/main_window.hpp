#ifndef GUI_MAIN_WINDOW_HPP_
#define GUI_MAIN_WINDOW_HPP_

#include <QPointer>
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

 protected:
  void closeEvent(QCloseEvent* event) override;

 public slots:
  void newFile();
  void openFile();
  void saveFile();
  void saveFileAs();
  void onParseSuccess() const;
  void onParseFail(const QStringList& errors) const;
  void onModifiedChanged(bool modified);

 private:
  void setupMenuBar();
  void setupStatusBar();
  void updateTitle();

  QPointer<TextEditor> editor_;
  QPointer<QuantumCircuitView> circuitView_;
  QPointer<AppController> controller_;
  QPointer<QSplitter> splitter_;
  QPointer<QLabel> statusLabel_;
};

}  // namespace qde::gui

#endif  // GUI_MAIN_WINDOW_HPP_
