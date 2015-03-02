
#include "qtVcrossLayerBar.h"

#include "qtUtility.h"
#include "qtVcrossLayerButton.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.VcrossLayerBar"
#include <miLogger/miLogging.h>

static const char MIMETYPE_VCROSS_LAYER_DRAG[] =
    "application/x-diana-vcross-layer-position";

VcrossLayerBar::VcrossLayerBar(QWidget* parent)
  : QWidget(parent)
{
  QBoxLayout* l = new QVBoxLayout(this);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(1);
  setLayout(l);

  setAcceptDrops(true);
}

void VcrossLayerBar::setManager(vcross::QtManager_p vsm)
{
  METLIBS_LOG_SCOPE();
  if (vcrossm) {
    METLIBS_LOG_ERROR("vcross manager already set");
    return;
  }

  vcrossm = vsm;

  { vcross::QtManager* m = vcrossm.get();
    connect(m, SIGNAL(fieldAdded(int)),
        SLOT(onFieldAdded(int)));
    connect(m, SIGNAL(fieldRemoved(int)),
        SLOT(onFieldRemoved(int)));
  }
}

void VcrossLayerBar::onFieldAction(int position, int action)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));

  if (action == VcrossLayerButton::EDIT) {
    Q_EMIT requestStyleEditor(position);
  } else if (action == VcrossLayerButton::REMOVE) {
    vcrossm->removeField(position);
  } else if (action == VcrossLayerButton::UP) {
    vcrossm->moveField(position, position+1);
  } else if (action == VcrossLayerButton::DOWN) {
    vcrossm->moveField(position, position-1);
  } else if (action == VcrossLayerButton::SHOW_HIDE) {
    QBoxLayout* lbl = static_cast<QBoxLayout*>(layout());
    QWidgetItem* wi = static_cast<QWidgetItem*>(lbl->itemAt(position));
    VcrossLayerButton *button = static_cast<VcrossLayerButton*>(wi->widget());
    vcrossm->setFieldVisible(position, button->isChecked());
  }
}

void VcrossLayerBar::onFieldAdded(int position)
{
  METLIBS_LOG_SCOPE();
  VcrossLayerButton* button = new VcrossLayerButton(vcrossm, position, this);
  connect(button, SIGNAL(triggered(int, int)),
      SLOT(onFieldAction(int, int)));
  connect(button, SIGNAL(startDrag(int)),
      SLOT(onStartDrag(int)));

  static_cast<QBoxLayout*>(layout())
      ->insertWidget(position, button);
}

void VcrossLayerBar::onFieldRemoved(int position)
{
  METLIBS_LOG_SCOPE();
  delete layout()->takeAt(position);
}

void VcrossLayerBar::onStartDrag(int position)
{
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;
  mimeData->setData(MIMETYPE_VCROSS_LAYER_DRAG,
      QString::number(position).toUtf8());
  drag->setMimeData(mimeData);
  drag->setPixmap(createPixmapForStyle(vcrossm->getOptionsAt(position)));

  drag->exec(Qt::MoveAction | Qt::CopyAction);
}

void VcrossLayerBar::dragEnterEvent(QDragEnterEvent* event)
{
  METLIBS_LOG_SCOPE();
  if (vcrossm && event->source() == this)
    event->acceptProposedAction();
  else
    event->ignore();
}

void VcrossLayerBar::dragMoveEvent(QDragMoveEvent* event)
{
  int drag_position, drop_position;
  if (acceptableDrop(event, drag_position, drop_position))
    event->acceptProposedAction();
  else
    event->ignore();
}

void VcrossLayerBar::dropEvent(QDropEvent *event)
{
  METLIBS_LOG_SCOPE();
  int drag_position, drop_position;
  if (!acceptableDrop(event, drag_position, drop_position)) {
    event->ignore();
  } else {
    METLIBS_LOG_DEBUG(LOGVAL(drag_position) << LOGVAL(drop_position));
    if (event->proposedAction() == Qt::MoveAction) {
      vcrossm->moveField(drag_position, drop_position);
    } else {
      const vcross::QtManager::PlotSpec ps(vcrossm->getModelAt(drag_position),
          vcrossm->getReftimeAt(drag_position),
          vcrossm->getFieldAt(drag_position));
      vcrossm->addField(ps, vcrossm->getOptionsAt(drag_position),
          drop_position+1, false);
    }
    event->acceptProposedAction();
  }
}

bool VcrossLayerBar::acceptableDrop(const QDropEvent* event,
    int& drag_position, int& drop_position)
{
  if (!vcrossm || event->source() != this)
    return false;
  if (!event->mimeData()->hasFormat(MIMETYPE_VCROSS_LAYER_DRAG))
    return false;

  drop_position = fieldIndex(event->pos());
  drag_position = QString::fromUtf8(event->mimeData()
      ->data(MIMETYPE_VCROSS_LAYER_DRAG)).toInt();

  if (drop_position < 0)
    return false;
  const Qt::DropAction proposed = event->proposedAction();
  if (proposed == Qt::MoveAction && drag_position != drop_position)
    return true;
  if (proposed == Qt::CopyAction)
    return true;
  return false;
}

int VcrossLayerBar::fieldIndex(const QPoint& pos)
{
  const QBoxLayout* l = static_cast<QBoxLayout*>(layout());
  const int drop_y = pos.y();

  int drop_position = -1;
  for (int p = 0; p<l->count(); ++p) {
    QWidgetItem* wi = static_cast<QWidgetItem*>(l->itemAt(p));
    VcrossLayerButton *button = static_cast<VcrossLayerButton*>(wi->widget());
    const int button_mid = button->y() + button->height()/2;
    if (p == 0 && drop_y < button_mid)
      drop_position = p;
    else if (drop_y >= button_mid)
      drop_position = p;
  }
  return drop_position;
}
