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

  ShortcutManager() {
    config_["file.new"] = QKeySequence::New;
    config_["file.open"] = QKeySequence::Open;
    config_["file.save"] = QKeySequence::Save;
    config_["file.save_as"] = QKeySequence::SaveAs;

    config_["edit.indent"] = QKeySequence("Tab");
    config_["edit.outdent"] = QKeySequence("Shift+Tab");
    config_["edit.comment"] = QKeySequence("Ctrl+/");
    config_["edit.move_block_up"] = QKeySequence("Alt+Up");
    config_["edit.move_block_dow"] = QKeySequence("Alt+Down");

    config_["navigate.next_match"] = QKeySequence("F3");
    config_["navigate.prev_match"] = QKeySequence("Shift+F3");
    config_["navigate.goto_definition"] = QKeySequence("Ctrl+F12");
    config_["navigate.fuzzy_search"] = QKeySequence("Ctrl+P");

    config_["build.run"] = QKeySequence("F5");
  }

  QKeySequence get(const QString& actionId) const {
    return config_.value(actionId);
  }

 private:
  QMap<QString, QKeySequence> config_;
};

}  // namespace qde::gui

#endif  // GUI_SHORTCUT_MANAGER_HPP_
