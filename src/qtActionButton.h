#ifndef ACTIONBUTTON_H
#define ACTIONBUTTON_H

#include <QAbstractButton>
class QAction;
     
class ActionButton : public QObject
{ Q_OBJECT;
     
public:
  ActionButton(QAbstractButton *button, QAction* action, QObject* parent=0);
  void setAction(QAction* action);
  
private Q_SLOTS:
  void updateFromAction();

private:
  void doConnect();
  void doDisconnect();
  
private:
  QAbstractButton* mButton;
  QAction* mAction;
};
     
#endif // ACTIONBUTTON_H
