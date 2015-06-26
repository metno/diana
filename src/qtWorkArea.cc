/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtWorkArea.h"
#include "qtGLwidget.h"
#include "diEditItemManager.h"
#include "diController.h"

#include <QVBoxLayout>

#include <qpixmap.h>
#include <qcursor.h>
#include <paint_cursor.xpm>
#include <paint_add_cursor.xpm>
#include <paint_remove_cursor.xpm>
#include <paint_forbidden_cursor.xpm>

WorkArea::WorkArea(Controller *co,  QWidget* parent)
  : QWidget(parent)
  , glw(new GLwidget(co))
  , qw(DiPaintable::createWidget(glw, this))
  , currentCursor(keep_it)
{
  qw->setMinimumSize( 300, 200 );
  qw->setMouseTracking(true);

  EditItemManager *editm = static_cast<EditItemManager *>(co->getManager("EDITDRAWING"));
  if (editm)
    connect(editm, SIGNAL(repaintNeeded()), qw, SLOT(updateGL())); // e.g. during undo/redo

  connect(glw, SIGNAL(changeCursor(cursortype)),
      this, SLOT(changeCursor(cursortype)));

  QVBoxLayout* vlayout = new QVBoxLayout(this);
  vlayout->addWidget(qw, 1);
  vlayout->activate();

  changeCursor(normal_cursor);
}

WorkArea::~WorkArea()
{
  delete qw;
  delete glw;
}

void WorkArea::updateGL()
{
  qw->update();
}

void WorkArea::changeCursor(const cursortype c)
{
  if (c == keep_it || c == currentCursor)
    return;

  switch (c) {
  case edit_cursor:
    qw->setCursor(Qt::ArrowCursor);
    break;
  case edit_move_cursor:
    qw->setCursor(Qt::ArrowCursor);
    break;
  case edit_value_cursor:
    qw->setCursor(Qt::UpArrowCursor);
    break;
  case draw_cursor:
    qw->setCursor(Qt::PointingHandCursor);
    break;
  case paint_select_cursor:
    qw->setCursor(Qt::UpArrowCursor);
    break;
  case paint_move_cursor:
    qw->setCursor(Qt::SizeAllCursor);
    break;
  case paint_draw_cursor:
    qw->setCursor(QCursor(QPixmap(paint_cursor_xpm), 0, 16));
    break;
  case paint_add_cursor:
    qw->setCursor(QCursor(QPixmap(paint_add_cursor_xpm), 7, 1));
    break;
  case paint_remove_cursor:
    qw->setCursor(QCursor(QPixmap(paint_remove_cursor_xpm), 7, 1));
    break;
  case paint_forbidden_cursor:
    qw->setCursor(QCursor(QPixmap(paint_forbidden_cursor_xpm), 7, 1));
    break;
  case normal_cursor:
  default:
    qw->setCursor(Qt::ArrowCursor);
    break;
  }
  currentCursor = c;
}
