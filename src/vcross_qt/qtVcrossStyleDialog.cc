
#include "qtVcrossStyleDialog.h"

#define MILOGGER_CATEGORY "diana.VcrossStyleDialog"
#include <miLogger/miLogging.h>

#include "vcross_style_dialog.ui.h"
#include "felt.xpm"

namespace {
enum {
  ModelRole = Qt::UserRole + 1,
  FieldRole
};
std::ostream& operator<<(std::ostream& out, const QString& qstr)
{
  out << qstr.toStdString();
  return out;
}
} // namespace

VcrossStyleDialog::VcrossStyleDialog(QWidget* parent)
  : QDialog(parent)
  , selectionManager(0)
  , ui(new Ui_VcrossStyleDialog)
{
  setupUi();
}

void VcrossStyleDialog::setSelectionManager(VcrossSelectionManager* vsm)
{
  if (selectionManager == vsm)
    return;

  if (selectionManager) {
    disconnect(selectionManager, SIGNAL(fieldAdded(const std::string&, const std::string&, int)),
        this, SLOT(onFieldAdded(const std::string&, const std::string&, int)));
    disconnect(selectionManager, SIGNAL(fieldRemoved(const std::string&, const std::string&, int)),
        this, SLOT(onFieldRemoved(const std::string&, const std::string&, int)));
    disconnect(selectionManager, SIGNAL(fieldsRemoved()),
        this, SLOT(onFieldsRemoved()));
  }
  selectionManager = vsm;
  if (selectionManager) {
    connect(selectionManager, SIGNAL(fieldAdded(const std::string&, const std::string&, int)),
        this, SLOT(onFieldAdded(const std::string&, const std::string&, int)));
    connect(selectionManager, SIGNAL(fieldRemoved(const std::string&, const std::string&, int)),
        this, SLOT(onFieldRemoved(const std::string&, const std::string&, int)));
    connect(selectionManager, SIGNAL(fieldsRemoved()),
        this, SLOT(onFieldsRemoved()));
  }
}

void VcrossStyleDialog::showModelField(const QString& mdl, const QString& fld)
{
  METLIBS_LOG_SCOPE(LOGVAL(mdl) << LOGVAL(fld));
  for (int r=0; r<mPlots->rowCount(); ++r) {
    if (mdl == modelName(r) and fld == fieldName(r)) {
      ui->comboPlot->setCurrentIndex(r);
      slotSelectedPlotChanged(r);
      return;
    }
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

QString VcrossStyleDialog::modelName(int index)
{
  return mPlots->item(index, 0)->data(ModelRole).toString();
}

QString VcrossStyleDialog::fieldName(int index)
{
  return mPlots->item(index, 0)->data(FieldRole).toString();
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

void VcrossStyleDialog::onFieldAdded(const std::string& model, const std::string& field, int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(model) << LOGVAL(field) << LOGVAL(position));
  const QString mdl = QString::fromStdString(model), fld = QString::fromStdString(field);
  QStandardItem* item = new QStandardItem(tr("M: %1 -- F: %2").arg(mdl).arg(fld));
  item->setData(mdl, ModelRole);
  item->setData(fld, FieldRole);
  mPlots->insertRow(position, item);
  ui->comboPlot->setCurrentIndex(position);
  enableWidgets();
}

void VcrossStyleDialog::onFieldUpdated(const std::string& model, const std::string& field, int position)
{
}

void VcrossStyleDialog::onFieldRemoved(const std::string&, const std::string&, int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));
  QList<QStandardItem*> items = mPlots->takeRow(position);
  while (!items.isEmpty())
    delete items.takeFirst();
  enableWidgets();
}

void VcrossStyleDialog::onFieldsRemoved()
{
  METLIBS_LOG_SCOPE();
  mPlots->clear(); // does it also delete the QStandardItems?
  enableWidgets();
}

void VcrossStyleDialog::slotSelectedPlotChanged(int index)
{
  METLIBS_LOG_SCOPE(LOGVAL(index));
  if (index < 0)
    return;

  const QString& mdl = modelName(index), fld = fieldName(index);
  METLIBS_LOG_DEBUG(LOGVAL(mdl) << LOGVAL(fld));
  const std::string opt  = selectionManager->getOptionsAt(index);
  const std::string dflt = selectionManager->defaultOptions(mdl, fld, true);
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
  if (r >= 0) {
    const QString& mdl = modelName(r), fld = fieldName(r);
    selectionManager->updateField(mdl.toStdString(), fld.toStdString(), ui->styleWidget->options());
  }
}
