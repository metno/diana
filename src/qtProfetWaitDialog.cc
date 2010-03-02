#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtProfetWaitDialog.h"
#include <QProgressBar>
#include <QVBoxLayout>
#include <QTimer>

ProfetWaitDialog::ProfetWaitDialog(QWidget* parent, int timeout, int update)
  : QDialog(parent), timeout_(timeout), update_(update), value_(0)
{
  setWindowTitle(tr("Reconnecting.."));
  
  QVBoxLayout * mainLayout   = new QVBoxLayout(this);
  mainLayout->setMargin(2);

  bar = new QProgressBar(this);
  bar->setRange(0,timeout);
  mainLayout->addWidget(bar);
  
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(showProgress())); 
  timer->start(update);
}

void ProfetWaitDialog::showProgress()
{
  value_ += update_;
  bar->setValue(value_);
  if ( value_ >= timeout_ )
    accept();
}
