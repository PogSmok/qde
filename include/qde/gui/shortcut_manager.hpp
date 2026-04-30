#ifndef GUI_SHORTCUT_MANAGER_HPP_
#define GUI_SHORTCUT_MANAGER_HPP_

#include <QKeySequence>
#include <QMap>
#include <QSettings>

namespace qde::gui {

class ShortcutManager {
 public:
  static ShortcutManager& instance() {
    static ShortcutManager inst;
    return inst;
  }

  void loadDefaults() {
    shortcuts_["file.new"] = QKeySequence::New;
    shortcuts_["file.open"] = QKeySequence::Open;
    shortcuts_["file.save"] = QKeySequence::Save;
    shortcuts_["file.saveas"] = QKeySequence::SaveAs;
    shortcuts_["file.close"] = QKeySequence::Close;

    shortcuts_["edit.indent"] = QKeySequence(Qt::Key_Tab);
    shortcuts_["edit.outdent"] = QKeySequence(Qt::Key_Shift | Qt::Key_Tab);
    shortcuts_["edit.comment"] = QKeySequence(Qt::Key_Control | Qt::Key_Slash);
    shortcuts_["edit.moveblock_up"] = QKeySequence(Qt::Key_Alt | Qt::Key_Up);
    shortcuts_["edit.moveblock_dow"] = QKeySequence(Qt::Key_Alt | Qt::Key_Down);

    shortcuts_["navigate.next_match"] = QKeySequence(Qt::Key_F3);
    shortcuts_["navigate.prev_match"] =
        QKeySequence(Qt::Key_Shift | Qt::Key_F3);
    shortcuts_["navigate.goto_definition"] =
        QKeySequence(Qt::Key_Control | Qt::Key_F12);
    shortcuts_["navigate.fuzzy_search"] =
        QKeySequence(Qt::Key_Control | Qt::Key_P);

    shortcuts_["build.run"] = QKeySequence(Qt::Key_F5);
  }

  QKeySequence get(const QString& actionId) const {
    return shortcuts_.value(actionId);
  }

 private:
  QMap<QString, QKeySequence> shortcuts_;
};

}  // namespace qde::gui

#endif  // GUI_SHORTCUT_MANAGER_HPP_
