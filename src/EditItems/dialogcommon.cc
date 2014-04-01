/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "empty.xpm"
#include <EditItems/dialogcommon.h>
#include <QToolButton>
#include <QIcon>

namespace EditItems {

CheckableLabel::CheckableLabel(bool checked, const QPixmap &pixmap, const QString &checkedToolTip, const QString &uncheckedToolTip, bool clickable)
  : checked_(checked)
  , pixmap_(pixmap)
  , checkedToolTip_(checkedToolTip)
  , uncheckedToolTip_(uncheckedToolTip)
  , clickable_(clickable)
{
  setMargin(0);
  setChecked(checked_);
}

void CheckableLabel::setChecked(bool enabled)
{
  checked_ = enabled;
  if (checked_) {
    setPixmap(pixmap_);
    setToolTip(checkedToolTip_);
  } else {
    setPixmap(empty_xpm);
    setToolTip(uncheckedToolTip_);
  }
}

void CheckableLabel::mousePressEvent(QMouseEvent *event)
{
  if (clickable_ && (event->button() & Qt::LeftButton))
    setChecked(!checked_);
  emit mouseClicked(event);
  emit checked(checked_);
}

ClickableLabel::ClickableLabel(const QString &name)
  : QLabel(name)
{
  setMargin(0);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
  emit mouseClicked(event);
}

void ClickableLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
  emit mouseDoubleClicked(event);
}

ScrollArea::ScrollArea(QWidget *parent)
  : QScrollArea(parent)
{
}

void ScrollArea::keyPressEvent(QKeyEvent *event)
{
  event->ignore();
}

QToolButton *createToolButton(const QIcon &icon, const QString &toolTip, const QObject *object, const char *method)
{
  QToolButton *button = new QToolButton;
  button->setIcon(icon);
  button->setToolTip(toolTip);
  QObject::connect(button, SIGNAL(clicked()), object, method);
  return button;
}

} // namespace
