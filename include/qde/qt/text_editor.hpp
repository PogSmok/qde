#ifndef TEXT_EDITOR_HPP_
#define TEXT_EDITOR_HPP_

#include <QWidget>

// TODO: implement
class TextEditor : public QWidget {
  Q_OBJECT
 public:
  explicit TextEditor(QWidget* parent = nullptr);
  virtual ~TextEditor();
};

#endif // TEXT_EDITOR_HPP_