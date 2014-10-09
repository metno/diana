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
#include "EditItems/drawingstylemanager.h"

namespace EditItem_Text {

Text::Text()
{
  cursor_ = -1;
  line_ = -1;
  editAction = new QAction(tr("Edit text"), this);
  connect(editAction, SIGNAL(triggered()), SLOT(editItem()));
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
  QRectF textbox = boundingRect();
  textbox.translate(offset());
  return textbox.contains(pos);
}

bool Text::hit(const QRectF &bbox) const
{
  if (points_.size() < 2)
    return false;

  QRectF textbox = boundingRect();
  textbox.translate(offset());
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

/**
 * Processes a mouse press event when the object is incomplete.
 * This implementation only handles left button clicks, adding two points to the internal
 * list on the first click.
 * Any additional clicks cause the object to be marked as complete.
 */
void Text::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;

  if (event->button() == Qt::LeftButton) {
    if (points_.size() < 2) {
      // Create two points: one for the current mouse position and another to be
      // updated during the following move events.
      points_.append(QPointF(event->pos()));
      points_.append(QPointF(event->pos()));
      cursor_ = -1;

      // Ensure that the manager gets the keyboard focus so that key events are
      // delivered to this item.
      EditItemManager::instance()->setFocus(true);
    } else
      complete = true;
  }
}

void Text::incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  // Update the geographic points and the control points.
  setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*this));
  updateControlPoints();

  repaintNeeded = true;

  // Make the text area editable by using a valid index.
  cursor_ = 0;
  line_ = 0;
  QStringList lines_ = text();
  lines_.append(QString());
  setText(lines_);
}

void Text::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if (cursor_ == -1) {
    event->ignore();
    return;
  }

  QStringList lines_ = text();

  switch (event->key()) {
  case Qt::Key_Escape:
    aborted = true;
    break;
  case Qt::Key_Return:
    if (event->modifiers() & Qt::ShiftModifier)
      complete = true;
    else {
      lines_.insert(line_ + 1, lines_.at(line_).mid(cursor_));
      lines_[line_] = lines_.at(line_).left(cursor_);
      line_ += 1;
      cursor_ = 0;
    }
    break;
  case Qt::Key_Backspace:
    if (cursor_ > 0) {
      lines_[line_] = lines_[line_].left(cursor_ - 1) + lines_[line_].mid(cursor_);
      cursor_ -= 1;
    } else if (line_ > 0) {
      cursor_ = lines_.at(line_ - 1).size();
      lines_[line_ - 1] += lines_[line_];
      lines_.removeAt(line_);
      line_ -= 1;
    }
    break;
  case Qt::Key_Delete:
    if (cursor_ <= lines_[line_].size() - 1)
      lines_[line_] = lines_[line_].left(cursor_) + lines_[line_].mid(cursor_ + 1);
    else if (line_ < lines_.size() - 1) {
      lines_[line_] += lines_[line_ + 1];
      lines_.removeAt(line_ + 1);
    }
    break;
  case Qt::Key_Left:
    if (cursor_ > 0)
      cursor_ -= 1;
    else if (line_ > 0) {
      line_ -= 1;
      cursor_ = lines_.at(line_).size();
    }
    break;
  case Qt::Key_Right:
    if (cursor_ < lines_[line_].size())
      cursor_ += 1;
    else if (line_ < lines_.size() - 1) {
      line_ += 1;
      cursor_ = 0;
    }
    break;
  case Qt::Key_Up:
    if (line_ > 0)
      line_ -= 1;
    break;
  case Qt::Key_Down:
    if (line_ < lines_.size() - 1)
      line_ += 1;
    break;
  case Qt::Key_Home:
    cursor_ = 0;
    break;
  case Qt::Key_End:
    cursor_ = lines_[line_].size();
    break;
  default:
    lines_[line_].insert(cursor_, event->text());
    cursor_ += event->text().size();
  }

  setText(lines_);
  updateControlPoints();
  event->accept();
}

void Text::resize(const QPointF &)
{
}

void Text::updateControlPoints()
{
  if (points_.size() < 2)
    return;

  updateRect();
}

void Text::setPoints(const QList<QPointF> &points)
{
  setGeometry(points);
}

void Text::drawHoverHighlighting(bool, bool) const
{
  QRectF bbox = boundingRect();
  bbox.translate(offset());

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
  if (points_.size() < 2)
    return;

  QRectF bbox = boundingRect();
  bbox.translate(offset());

  if (!text().isEmpty()) {
    glLineStipple(1, 0x5555);
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINE_LOOP);
    glVertex2f(bbox.bottomLeft().x(), bbox.bottomLeft().y());
    glVertex2f(bbox.bottomRight().x(), bbox.bottomRight().y());
    glVertex2f(bbox.topRight().x(), bbox.topRight().y());
    glVertex2f(bbox.topLeft().x(), bbox.topLeft().y());
    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }

  float x = bbox.bottomLeft().x();
  float y = bbox.bottomLeft().y();
  QSizeF size;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();
  GLfloat scale = qMax(StaticPlot::getPhysWidth()/StaticPlot::getMapSize().width(), StaticPlot::getPhysHeight()/StaticPlot::getMapSize().height());
  styleManager->beginText(this, poptions);

  QStringList lines_ = text();

  for (int line = 0; line < line_; ++line) {
    size = getStringSize(lines_.at(line));
    y -= size.height() * (1.0 + spacing_);
  }

  if (cursor_ != -1) {
    size = getStringSize(lines_.at(line_), cursor_);
    // Draw a caret.
    glColor3ub(255, 0, 0);
    glBegin(GL_LINES);
    glVertex2f(x + margin_ + size.width(), y - size.height() - margin_);
    glVertex2f(x + margin_ + size.width(), y - margin_);
    glEnd();
  }
}

void Text::editItem()
{
  EditItemManager::instance()->editItem(this);
  EditItemManager::instance()->setFocus(true);
}

} // namespace EditItem_Text
