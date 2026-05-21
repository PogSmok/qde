#ifndef GUI_MAIN_WINDOW_HPP_
#define GUI_MAIN_WINDOW_HPP_

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
  void NewFile();
  void OpenFile();
  void SaveFile();
  void SaveFileAs();
  void OnParseSuccess() const;
  void OnParseFail(const QStringList& errors) const;
  void OnModifiedChanged(bool modified);

 private:
  void SetupMenuBar();
  void SetupStatusBar();
  void SetupActions();

  template <class Func>
  void CreateAction(const QString& text, const QKeySequence& ks, Func slot);

  template <class ObjPtr, class Func>
  void CreateAction(const QString& text, const QKeySequence& ks, ObjPtr obj,
                    Func slot);
  void UpdateTitle();

  QPointer<TextEditor> editor_;
  QPointer<QuantumCircuitView> circuitView_;
  QPointer<AppController> controller_;
  QPointer<QSplitter> splitter_;
  QPointer<QLabel> statusLabel_;
};

}  // namespace qde::gui

#endif  // GUI_MAIN_WINDOW_HPP_
