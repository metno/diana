
#ifndef QTDIANAAPPLICATION_H
#define QTDIANAAPPLICATION_H 1

#include <QApplication>

class QMessageBox;
class QTimer;

class DianaApplication : public QApplication {
  Q_OBJECT;

public:
  DianaApplication(int& argc, char** argv);

  bool notify(QObject* receiver, QEvent * event) Q_DECL_OVERRIDE;
  int exec();

  void startExceptionReminder();

private Q_SLOTS:
  void onAccepted();
  void onRejected();
  void onTimeout();

private:
  bool hadException;
  QMessageBox* box;
  QTimer* timer;
};

#endif // QTDIANAAPPLICATION_H
