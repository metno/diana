
#include "qtVcrossLayerButton.h"

#include <QAction>
#include <QMenu>
#include <QPixmap>

#include "felt.xpm"

VcrossLayerButton::VcrossLayerButton(const QString& model, const QString& field, int p, QWidget* parent)
  : QToolButton(parent)
  , position(p)
{
  setIcon(QPixmap(felt_xpm));
  setToolTip(tr("Model: %1 Field: %2").arg(model).arg(field));
  setCheckable(true);
  setChecked(true);

  connect(this, SIGNAL(toggled(bool)), this, SLOT(onShowHide()));
  
  QMenu* menu = new QMenu(this);

  QAction* actionEdit = menu->addAction(tr("Style"));
  connect(actionEdit, SIGNAL(triggered()), SLOT(onEdit()));

  actionShowHide = menu->addAction(tr("Show/Hide"));
  connect(actionShowHide, SIGNAL(triggered()), SLOT(toggle())); // calls toggled signal

  actionUp = menu->addAction(tr("Up"));
  connect(actionUp, SIGNAL(triggered()), SLOT(onUp()));
  actionUp->setEnabled(false);

  actionDown = menu->addAction(tr("Down"));
  connect(actionDown, SIGNAL(triggered()), SLOT(onDown()));
  actionDown->setEnabled(false);

  QAction* actionRemove = menu->addAction(tr("Remove"));
  connect(actionRemove, SIGNAL(triggered()), SLOT(onRemove()));

  setMenu(menu);
}

void VcrossLayerButton::setPosition(int p, bool last)
{
  position = p;
  actionDown->setEnabled(position > 0);
  actionUp->setEnabled(not last);
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
  Q_EMIT triggered(position, SHOW_HIDE);
}

void VcrossLayerButton::onUp()
{
  Q_EMIT triggered(position, UP);
}

void VcrossLayerButton::onDown()
{
  Q_EMIT triggered(position, DOWN);
}
