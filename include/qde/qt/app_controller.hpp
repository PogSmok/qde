#ifndef APP_CONTROLLER_HPP_
#define APP_CONTROLLER_HPP_

#include <QObject>

// TODO: implement
class AppController : public QObject {
  Q_OBJECT
 public:
  explicit AppController(QObject* parent = nullptr);
  virtual ~AppController();
};

#endif // APP_CONTROLLER_HPP_