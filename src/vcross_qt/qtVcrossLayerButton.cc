
#include "qtVcrossLayerButton.h"

#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QPixmap>

#include "felt.xpm"

VcrossLayerButton::VcrossLayerButton(vcross::QtManager_p vcm, int p, QWidget* parent)
  : QToolButton(parent)
  , vcrossm(vcm)
  , position(p)
{
  const std::string model = vcrossm->getModelAt(position),
      reftime = vcrossm->getReftimeAt(position).isoTime(),
      field = vcrossm->getFieldAt(position);
  const QString label = tr("Model: %1 Reftime: %2 Field: %3")
      .arg(QString::fromStdString(model))
      .arg(QString::fromStdString(reftime))
      .arg(QString::fromStdString(field));


  setToolTip(label);
  setCheckable(true);
  setChecked(true);

  updateStyle();

  connect(this, SIGNAL(toggled(bool)), this, SLOT(onShowHide()));

  { vcross::QtManager* m = vcrossm.get();
    connect(m, SIGNAL(fieldAdded(int)),
        this, SLOT(onFieldAdded(int)));
    connect(m, SIGNAL(fieldRemoved(int)),
        this, SLOT(onFieldRemoved(int)));
    connect(m, SIGNAL(fieldOptionsChanged(int)),
        this, SLOT(onFieldOptionsChanged(int)));
    connect(m, SIGNAL(fieldVisibilityChanged(int)),
        this, SLOT(onFieldVisibilityChanged(int)));
  }

  QMenu* menu = new QMenu(this);

  QAction* actionEdit = menu->addAction(tr("Style"));
  connect(actionEdit, SIGNAL(triggered()), SLOT(onEdit()));

  actionShowHide = menu->addAction(tr("Show/Hide"));
  connect(actionShowHide, SIGNAL(triggered()), SLOT(toggle())); // calls toggled signal

  actionUp = menu->addAction(tr("Up"));
  connect(actionUp, SIGNAL(triggered()), SLOT(onUp()));

  actionDown = menu->addAction(tr("Down"));
  connect(actionDown, SIGNAL(triggered()), SLOT(onDown()));

  QAction* actionRemove = menu->addAction(tr("Remove"));
  connect(actionRemove, SIGNAL(triggered()), SLOT(onRemove()));

  setMenu(menu);
  setPopupMode(QToolButton::MenuButtonPopup);
  enableUpDown();
}

void VcrossLayerButton::enableUpDown()
{
  actionDown->setEnabled(position > 0);
  const int n = vcrossm->getFieldCount()-1;
  actionUp->setEnabled(position != n);
}

void VcrossLayerButton::updateStyle()
{
  QPixmap pixmap = createPixmapForStyle(vcrossm->getOptionsAt(position));
  if (pixmap.isNull())
    pixmap = QPixmap(felt_xpm);
  setIcon(pixmap);
}

void VcrossLayerButton::onEdit()
{
  Q_EMIT triggered(position, EDIT);
}

void VcrossLayerButton::onRemove()
{
  Q_EMIT triggered(position, REMOVE);
}

void VcrossLayerButton::onShowHide()
{
  vcrossm->setFieldVisible(position, isChecked());
}

void VcrossLayerButton::onUp()
{
  Q_EMIT triggered(position, UP);
}

void VcrossLayerButton::onDown()
{
  Q_EMIT triggered(position, DOWN);
}

void VcrossLayerButton::onFieldAdded(int p)
{
  if (p <= position)
    position += 1;
  enableUpDown();
}

void VcrossLayerButton::onFieldRemoved(int p)
{
  if (p == position) {
    deleteLater();
    return;
  }
  if (p < position)
    position -= 1;
  enableUpDown();
}

void VcrossLayerButton::onFieldOptionsChanged(int p)
{
  if (p == position)
    updateStyle();
}

void VcrossLayerButton::onFieldVisibilityChanged(int p)
{
  if (p != position)
    return;

  const bool visible = vcrossm->getVisibleAt(position);
  if (isChecked() != visible)
    setChecked(visible);
  if (actionShowHide->isChecked() != visible)
    actionShowHide->setChecked(visible);
}
