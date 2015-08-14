
#include "qtVcrossLayerList.h"

#include "qtUtility.h"

#include <QListView>
#include <QStringListModel>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.VcrossLayerList"
#include <miLogger/miLogging.h>

static const char MIMETYPE_VCROSS_LAYER_DRAG[] =
    "application/x-diana-vcross-layer-position";

VcrossLayerList::VcrossLayerList(QWidget* parent)
  : QWidget(parent)
{
  QBoxLayout* l = new QVBoxLayout(this);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(1);
  setLayout(l);

  plots = new QStringListModel(this);
  plotsList = new QListView(this);
  plotsList->setSelectionMode(QAbstractItemView::MultiSelection);
  plotsList->setModel(plots);

  connect(plotsList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
      this, SLOT(onPlotSelectionChanged()));

  l->addWidget(plotsList);
}

void VcrossLayerList::setManager(vcross::QtManager_p vm)
{
  METLIBS_LOG_SCOPE();
  if (vcrossm) {
    METLIBS_LOG_ERROR("vcross manager already set");
    return;
  }

  vcrossm = vm;

  { vcross::QtManager* m = vcrossm.get();
    connect(m, SIGNAL(fieldAdded(int)),
        SLOT(onFieldAdded(int)));
    connect(m, SIGNAL(fieldRemoved(int)),
        SLOT(onFieldRemoved(int)));
  }

  QStringList plotsList;
  const size_t fields = vm->getFieldCount();
  for (size_t f = 0; f < fields; ++f)
    plotsList << fieldText(f);
  plots->setStringList(plotsList);
}

QString VcrossLayerList::fieldText(int position)
{
  const std::string model = vcrossm->getModelAt(position),
      reftime = vcrossm->getReftimeAt(position).isoTime(),
      field = vcrossm->getFieldAt(position);
  const QString label = QString::fromStdString(model)
      + " / " + QString::fromStdString(reftime)
      + " / " + QString::fromStdString(field);
  return label;
}

void VcrossLayerList::onFieldAdded(int position)
{
  METLIBS_LOG_SCOPE();
  QStringList plotsList = plots->stringList();
  plotsList.insert(position, fieldText(position));
  plots->setStringList(plotsList);
}

void VcrossLayerList::onFieldRemoved(int position)
{
  METLIBS_LOG_SCOPE();
  QStringList plotsList = plots->stringList();
  plotsList.removeAt(position);
  plots->setStringList(plotsList);
}

void VcrossLayerList::onPlotSelectionChanged()
{
  Q_EMIT selectionChanged();
}

void VcrossLayerList::selectAll()
{
  diutil::selectAllRows(plotsList);
}

QList<int> VcrossLayerList::selected() const
{
  QList<int> selected;
  const QModelIndexList si = plotsList->selectionModel()->selectedIndexes();
  for (int i=0; i<si.size(); ++i)
    selected << si.at(i).row();
  return selected;
}
