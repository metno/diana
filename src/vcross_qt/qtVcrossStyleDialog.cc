
#include "qtVcrossStyleDialog.h"

#define MILOGGER_CATEGORY "diana.VcrossStyleDialog"
#include <miLogger/miLogging.h>

#include "vcross_style_dialog.ui.h"
#include "felt.xpm"

namespace {
enum {
  ModelRole = Qt::UserRole + 1,
  ReftimeRole,
  FieldRole
};
} // namespace

VcrossStyleDialog::VcrossStyleDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui_VcrossStyleDialog)
{
  setupUi();
}

void VcrossStyleDialog::setManager(vcross::QtManager_p vsm)
{
  if (vcrossm == vsm)
    return;

  if (vcrossm) {
    disconnect(vcrossm.get(), SIGNAL(fieldAdded(int)),
        this, SLOT(onFieldAdded(int)));
    disconnect(vcrossm.get(), SIGNAL(fieldOptionsChanged(int)),
        this, SLOT(onFieldUpdated(int)));
    disconnect(vcrossm.get(), SIGNAL(fieldRemoved(int)),
        this, SLOT(onFieldRemoved(int)));
  }
  vcrossm = vsm;
  if (vcrossm) {
    connect(vcrossm.get(), SIGNAL(fieldAdded(int)),
        this, SLOT(onFieldAdded(int)));
    connect(vcrossm.get(), SIGNAL(fieldOptionsChanged(int)),
        this, SLOT(onFieldUpdated(int)));
    connect(vcrossm.get(), SIGNAL(fieldRemoved(int)),
        this, SLOT(onFieldRemoved(int)));
  }
}

void VcrossStyleDialog::showModelField(int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  if (index >= 0 && index<mPlots->rowCount()) {
    ui->comboPlot->setCurrentIndex(index);
    slotSelectedPlotChanged(index);
  }
}

void VcrossStyleDialog::setupUi()
{
  ui->setupUi(this);
  ui->labelPlot->setPixmap(QPixmap(felt_xpm).scaledToHeight(24)); // FIXME using ui->comboPlot->height() does not work
  connect(ui->styleWidget, SIGNAL(canResetOptions(bool)),
      ui->buttonResetOptions, SLOT(setEnabled(bool)));

  mPlots = new QStandardItemModel(this);
  ui->comboPlot->setModel(mPlots);

  enableWidgets(); // we assume that ui->styleWidget->isEnabled == true;
}

void VcrossStyleDialog::enableWidgets()
{
  const bool hasPlots = (mPlots->rowCount() > 0), hadPlots = ui->styleWidget->isEnabled();
  if (hasPlots != hadPlots) {
    ui->styleWidget->setEnabled(hasPlots);
    ui->buttonResetOptions->setEnabled(hasPlots); // FIXME also need to check with ui->styleWidget
    ui->buttonApply->setEnabled(hasPlots);
  }
}

void VcrossStyleDialog::onFieldAdded(int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));
  const QString mdl = QString::fromStdString(vcrossm->getModelAt(position)),
      rt = QString::fromStdString(vcrossm->getReftimeAt(position).isoTime()),
      fld = QString::fromStdString(vcrossm->getFieldAt(position));
  QStandardItem* item = new QStandardItem(tr("M: %1 -- R: %2 -- F: %3")
      .arg(mdl).arg(rt).arg(fld));
  mPlots->insertRow(position, item);
  ui->comboPlot->setCurrentIndex(position);
  enableWidgets();
}

void VcrossStyleDialog::onFieldUpdated(int position)
{
  if (position == ui->comboPlot->currentIndex())
    slotSelectedPlotChanged(position);
}

void VcrossStyleDialog::onFieldRemoved(int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));
  QList<QStandardItem*> items = mPlots->takeRow(position);
  while (!items.isEmpty())
    delete items.takeFirst();
  enableWidgets();
}

void VcrossStyleDialog::slotSelectedPlotChanged(int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  if (index < 0)
    return;

  const std::string fld = vcrossm->getFieldAt(index);
  const std::string opt  = vcrossm->getOptionsAt(index);
  const std::string dflt = vcrossm->getPlotOptions(fld, true);
  METLIBS_LOG_DEBUG(LOGVAL(index) << LOGVAL(fld) << LOGVAL(opt) << LOGVAL(dflt));
  ui->styleWidget->setOptions(opt, dflt);
}

void VcrossStyleDialog::slotResetPlotOptions()
{
  ui->styleWidget->resetOptions();
}

void VcrossStyleDialog::slotApply()
{
  METLIBS_LOG_SCOPE();
  const int r = ui->comboPlot->currentIndex();
  if (r >= 0)
    vcrossm->updateField(r, ui->styleWidget->options());
}
