
#include "qtVcrossModelPage.h"

#include "qtUtility.h"

#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>

#include "vcross_modelpage.ui.h"

#define MILOGGER_CATEGORY "diana.VcrossAddPlotDialog"
#include <miLogger/miLogging.h>

VcrossModelPage::VcrossModelPage(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui_VcrossModelPage)
{
  ui->setupUi(this);

  modelNames = new QStringListModel(this);
  modelSorter = new QSortFilterProxyModel(this);
  modelSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  modelSorter->setSourceModel(modelNames);
  ui->modelList->setModel(modelSorter);

  connect(ui->modelFilter, SIGNAL(textChanged(const QString&)),
      this, SLOT(onFilter(const QString&)));
  connect(ui->modelList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(checkComplete()));
  connect(ui->modelList, SIGNAL(activated(const QModelIndex&)),
      this, SLOT(listActivated()));
}

void VcrossModelPage::setManager(vcross::QtManager_p vm)
{
  vcrossm = vm;
}

void VcrossModelPage::initialize(bool forward)
{
  if (forward) {
    ui->modelFilter->clear();

    const std::vector<std::string> models = vcrossm->getAllModels();
    QStringList msl;
    for (size_t i=0; i<models.size(); ++i)
      msl << QString::fromStdString(models[i]);

    modelNames->setStringList(msl);
  }
}

bool VcrossModelPage::isComplete() const
{
  return ui->modelList->selectionModel()->selectedIndexes().size() == 1;
}

QString VcrossModelPage::selected() const
{
  const QModelIndexList si = ui->modelList->selectionModel()->selectedIndexes();
  if (si.size() == 1)
    return modelNames->stringList().at(modelSorter->mapToSource(si.at(0)).row());
  else
    return QString();
}

void VcrossModelPage::onFilter(const QString& text)
{
  modelSorter->setFilterFixedString(text);
  if (modelSorter->rowCount() == 1)
    diutil::selectAllRows(ui->modelList);
}

void VcrossModelPage::checkComplete()
{
  Q_EMIT completeStatusChanged(isComplete());
}

void VcrossModelPage::listActivated()
{
  if (isComplete())
    Q_EMIT requestNext();
}
