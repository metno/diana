/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2014 met.no

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

#include "edittext.h"
#include "diEditItemManager.h"
#include "diPlot.h"
#include "diFontManager.h"
#include "diPlotModule.h"
#include "EditItems/drawingstylemanager.h"
#include "EditItems/dialogcommon.h"
#include <EditItems/modifytextcommand.h>
#include <QAction>

namespace EditItem_Text {

Text::Text()
{
  editAction = new QAction(tr("Edit text"), this);
  connect(editAction, SIGNAL(triggered()), SLOT(editText()));
}

Text::~Text()
{
}

QList<QAction *> Text::actions(const QPoint &pos) const
{
  QList<QAction *> acts;
  acts << editAction;
  return acts;
}

DrawingItemBase *Text::cloneSpecial() const
{
  Text *item = new Text;
  copyBaseData(item);
  // ### copy special data from this into item ... TBD
  return item;
}

bool Text::hit(const QPointF &pos, bool selected) const
{
  QRectF textbox = drawingRect();
  return textbox.contains(pos);
}

bool Text::hit(const QRectF &bbox) const
{
  if (points_.size() < 2)
    return false;

  QRectF textbox = drawingRect();
  return textbox.intersects(bbox);
}

void Text::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                      QSet<QSharedPointer<DrawingItemBase> > *items,
                      const QSet<QSharedPointer<DrawingItemBase> > *selItems,
                      bool *multiItemOp)
{
  if (event->button() == Qt::LeftButton) {
    pressedCtrlPointIndex_ = -1;
    resizing_ = (pressedCtrlPointIndex_ >= 0);
    moving_ = !resizing_;
    basePoints_ = points_;
    baseMousePos_ = event->pos();

    if (multiItemOp)
      *multiItemOp = moving_; // i.e. a move operation would apply to all selected items
  }
}

void Text::mouseDoubleClick(QMouseEvent *, bool &)
{
  editText();
}

/**
 * Processes a mouse press event when the object is incomplete.
 * This implementation only handles left button clicks, adding two points to the internal
 * list on the first click.
 * Any additional clicks cause the object to be marked as complete.
 */
void Text::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if (event->button() == Qt::LeftButton) {
    Q_ASSERT(points_.isEmpty());
    // set initial bounding box corners
    points_.append(QPointF(event->pos()));
    points_.append(QPointF(event->pos()));

    if ((!editText()) || text().join("\n").trimmed().isEmpty()) {
      aborted = true;
    } else {
      complete = true;
      repaintNeeded = true;
      updateControlPoints();
    }
  }
}

void Text::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  Q_UNUSED(repaintNeeded);
  Q_UNUSED(complete);
  if (event->key() == Qt::Key_Escape) {
    aborted = true;
  }
}

void Text::resize(const QPointF &)
{
}

void Text::updateControlPoints()
{
  updateRect();

  controlPoints_.clear();
  const int size = controlPointSize(), size_2 = size / 2;
  const QRectF r = drawingRect();
  controlPoints_.append(QRectF( r.bottomLeft().x() - size_2,  r.bottomLeft().y() - size_2, size, size));
  controlPoints_.append(QRectF(r.bottomRight().x() - size_2, r.bottomRight().y() - size_2, size, size));
  controlPoints_.append(QRectF(   r.topRight().x() - size_2,    r.topRight().y() - size_2, size, size));
  controlPoints_.append(QRectF(    r.topLeft().x() - size_2,     r.topLeft().y() - size_2, size, size));
}

void Text::setPoints(const QList<QPointF> &points)
{
  setGeometry(points);
}

void Text::drawHoverHighlighting(bool, bool) const
{
  QRectF bbox = drawingRect();

  glColor3ub(255, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(bbox.bottomLeft().x(), bbox.bottomLeft().y());
  glVertex2f(bbox.bottomRight().x(), bbox.bottomRight().y());
  glVertex2f(bbox.topRight().x(), bbox.topRight().y());
  glVertex2f(bbox.topLeft().x(), bbox.topLeft().y());
  glEnd();
}

void Text::drawIncomplete() const
{
  if (points_.isEmpty())
    return;

  // mark the insertion point with a red square
  const float x = points_.first().x();
  const float y = points_.first().y();
  const float s = 4;
  glColor3ub(255, 0, 0);
  glBegin(GL_POLYGON);
  glVertex2f(x - s, y - s);
  glVertex2f(x + s, y - s);
  glVertex2f(x + s, y + s);
  glVertex2f(x - s, y + s);
  glEnd();
}

bool Text::editText()
{
  const QStringList oldText = text();

  EditItems::TextEditor textEditor(text().join("\n"));
  textEditor.setWindowTitle("Edit Text");
  if (textEditor.exec() == QDialog::Accepted) {
    const QString t = textEditor.text().trimmed();
    if (!t.isEmpty()) {
      const QStringList newText = t.split("\n");

      if (newText != oldText)
        EditItemManager::instance()->undoStack()->push(
              new EditItems::ModifyTextCommand("modify text", QSharedPointer<DrawingItem_Text::Text>(this), oldText, newText));
      return true;
    } else {
      return false;
    }
  }
  return false;
}

} // namespace EditItem_Text
