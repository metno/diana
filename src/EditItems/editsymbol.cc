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

#include <GL/gl.h>
#include "drawingsymbol.h"
#include "editsymbol.h"
#include <QDomDocument>
#include <QFileDialog>

#include <QMessageBox>

namespace EditItem_Symbol {

Symbol::Symbol()
{
    init();
    updateControlPoints();
}

Symbol::~Symbol()
{
}

bool Symbol::hit(const QPointF &pos, bool selected) const
{
    const bool hitSelectedControlPoint = selected && (hitControlPoint(pos) >= 0);
    const QPolygonF polygon(boundingSquare().toVector());
    const bool hitInterior = polygon.containsPoint(pos, Qt::OddEvenFill);
    return hitSelectedControlPoint || hitInterior;
}

bool Symbol::hit(const QRectF &rect) const
{
    Q_UNUSED(rect);
    return false; // for now
}


// ### similar to PolyLine::mousePress - move common code to base class?
void Symbol::mousePress(
    QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
    QSet<DrawingItemBase *> *itemsToCopy, QSet<DrawingItemBase *> *itemsToEdit,
    QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems, bool *multiItemOp)
{
    Q_ASSERT(undoCommands);

    if (event->button() == Qt::LeftButton) {
        pressedCtrlPointIndex_ = hitControlPoint(event->pos());
        resizing_ = (pressedCtrlPointIndex_ >= 0);
        moving_ = !resizing_;
        basePoints_ = points_;
        baseMousePos_ = event->pos();

        if (multiItemOp)
            *multiItemOp = moving_; // i.e. a move operation would apply to all selected items

    } else if (event->button() == Qt::RightButton) {
        if (selItems) {
            // open a context menu and perform the selected action
            QMenu contextMenu;
            QAction remove_act(tr("&Remove"), 0);
            QAction copyItems_act(tr("&Copy"), 0);
            QAction editItems_act(tr("P&roperties..."), 0);
            QAction save_act(tr("Save ..."), 0);

            // add actions
            contextMenu.addAction(&remove_act);
            if (itemsToCopy)
              contextMenu.addAction(&copyItems_act);
            if (itemsToEdit)
              contextMenu.addAction(&editItems_act);
            contextMenu.addAction(&save_act);
            QAction *action = contextMenu.exec(event->globalPos(), &remove_act);
            if (action == &remove_act)
                remove(repaintNeeded, items, selItems);
            else if (action == &copyItems_act) {
                Q_ASSERT(itemsToCopy);
                QSet<DrawingItemBase *>::const_iterator it;
                for (it = selItems->begin(); it != selItems->end(); ++it) {
                    Symbol *symbol = qobject_cast<Symbol *>(Editing(*it));
                    if (symbol)
                        itemsToCopy->insert(symbol);
                }
            } else if (action == &editItems_act) {
                Q_ASSERT(itemsToEdit);
                //Q_ASSERT(items->contains(this));
                itemsToEdit->insert(this);
            } else if (action == &save_act) {
              EditItemManager::instance()->saveItemsToFile();
            }
        }
    }
}

void Symbol::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if (event->button() == Qt::LeftButton) {
    Q_ASSERT(points_.isEmpty());
    points_.append(QPointF(event->pos()));
    complete = true; // causes repaint
  }
}

void Symbol::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  Q_UNUSED(repaintNeeded);
  Q_UNUSED(complete);
  if (event->key() == Qt::Key_Escape) {
    aborted = true;
  }
}

void Symbol::resize(const QPointF &pos)
{
  Q_ASSERT(points_.size() == 1);
  const QPointF delta = pos - points_.first();
  const qreal radius = sqrt(sqr(delta.x()) + sqr(delta.y()));
  size_ = (2 / sqrt(2)) * radius;
  updateControlPoints();
}

void Symbol::updateControlPoints()
{
    controlPoints_.clear();
    const int size = controlPointSize(), size_2 = size / 2;
    foreach (QPointF p, boundingSquare())
        controlPoints_.append(QRectF(p.x() - size_2, p.y() - size_2, size, size));
}

void Symbol::setPoints(const QList<QPointF> &points)
{
  setGeometry(points);
}

void Symbol::remove(bool &repaintNeeded, QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems)
{
    // Option 1: remove this item only:
    // items->remove(this);

    // Option 2: remove all selected items:
    items->subtract(*selItems);

    repaintNeeded = true;
}

void Symbol::drawHoverHighlighting(bool incomplete) const
{
  if (incomplete)
    glColor3ub(0, 200, 0);
  else
    glColor3ub(255, 0, 0);

  if (hoveredCtrlPointIndex_ >= 0) {
    EditItemBase::drawHoveredControlPoint();
  } else {
    // highlight the bounding box
    glPushAttrib(GL_LINE_BIT);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    foreach (QPointF p, boundingSquare())
      glVertex3i(p.x(), p.y(), 1);
    glEnd();
    glPopAttrib();
  }
}

} // namespace EditItem_Symbol
