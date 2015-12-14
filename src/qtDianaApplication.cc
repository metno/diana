
#include "qtDianaApplication.h"

#include <QTimer>
#include <QMessageBox>

#define MILOGGER_CATEGORY "diana.DianaApplication"
#include <miLogger/miLogging.h>

DianaApplication::DianaApplication(int& argc, char** argv)
  : QApplication(argc, argv)
  , hadException(false)
  , box(0)
  , timer(0)
{
}

bool DianaApplication::notify(QObject* receiver, QEvent* event)
{
  try {
    return QApplication::notify(receiver, event);
  } catch(std::exception& e) {
    hadException = true;
    METLIBS_LOG_ERROR("exception in main loop: " << e.what());
  } catch(...) {
    hadException = true;
    METLIBS_LOG_ERROR("exception in main loop");
  }
  startExceptionReminder();
  return false;
}

int DianaApplication::exec()
{
  int c = -1;
  try {
    c = QApplication::exec();
  } catch(std::exception& e) {
    hadException = true;
    METLIBS_LOG_ERROR("exception in exec: " << e.what());
  } catch(...) {
    hadException = true;
    METLIBS_LOG_ERROR("exception in exec");
  }
  if (hadException && c == 0)
    c = -1;
  return c;
}

void DianaApplication::startExceptionReminder()
{
  if (box)
    return;

  const int reminder_s = 3;
  box = new QMessageBox(QMessageBox::Critical, tr("Severe Error"),
      tr("A severe error has occurred. "
          "You MUST exit diana as soon as possible. "
          "A reminder will be shown every %1s.").arg(reminder_s));

  box->addButton(tr("Keep Going"), QMessageBox::RejectRole);
  box->addButton(tr("Exit Now"), QMessageBox::AcceptRole);

  timer = new QTimer(box);
  timer->setInterval(reminder_s * 1000); // ms
  timer->setSingleShot(true);

  connect(box, SIGNAL(accepted()), SLOT(onAccepted()));
  connect(box, SIGNAL(rejected()), SLOT(onRejected()));
  connect(timer, SIGNAL(timeout()), SLOT(onTimeout()));

  box->show();
}

void DianaApplication::onAccepted()
{
  METLIBS_LOG_SCOPE();
  quit();
}

void DianaApplication::onRejected()
{
  METLIBS_LOG_SCOPE();
  box->hide();
  timer->start();
}

void DianaApplication::onTimeout()
{
  METLIBS_LOG_SCOPE();
  box->show();
}
