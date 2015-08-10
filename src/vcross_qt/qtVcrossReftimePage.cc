
#include "qtVcrossReftimePage.h"

#include "qtUtility.h"

#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#include "vcross_reftimepage.ui.h"

#define MILOGGER_CATEGORY "diana.VcrossAddPlotDialog"
#include <miLogger/miLogging.h>

VcrossReftimePage::VcrossReftimePage(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui_VcrossReftimePage)
{
  ui->setupUi(this);

  referenceTimes = new QStringListModel(this);
  ui->reftimeList->setModel(referenceTimes);

  connect(ui->reftimeList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkComplete()));
  connect(ui->reftimeList, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(listActivated()));
}

void VcrossReftimePage::setManager(vcross::QtManager_p vm)
{
  vcrossm = vm;
}

void VcrossReftimePage::initialize(const QString& model, bool forward)
{
  ui->reftimeLabelModel->setText(tr("Chosen model: %1").arg(model));
  if (forward) {
    diutil::OverrideCursor waitCursor;
    const vcross::QtManager::vctime_v reftimes = vcrossm->getModelReferenceTimes(model.toStdString());
    QStringList rsl;
    for (size_t i=0; i<reftimes.size(); ++i)
      rsl << QString::fromStdString(reftimes[i].isoTime());

    referenceTimes->setStringList(rsl);
    if (referenceTimes->rowCount() > 0) {
      const QModelIndex latest = referenceTimes->index(referenceTimes->rowCount()-1, 0);
      ui->reftimeList->selectionModel()->setCurrentIndex(latest, QItemSelectionModel::ClearAndSelect);
    }
  }
}

bool VcrossReftimePage::isComplete() const
{
  return ui->reftimeList->selectionModel()->selectedIndexes().size() == 1;
}

QString VcrossReftimePage::selected() const
{
  const QModelIndexList si = ui->reftimeList->selectionModel()->selectedIndexes();
  if (si.size() == 1)
    return referenceTimes->stringList().at(si.at(0).row());
  else
    return QString();
}

void VcrossReftimePage::checkComplete()
{
  Q_EMIT completeStatusChanged(isComplete());
}

void VcrossReftimePage::listActivated()
{
  if (isComplete())
    Q_EMIT requestNext();
}
