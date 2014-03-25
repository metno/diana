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
#include "diPlot.h"
#include "diFontManager.h"

namespace EditItem_Text {

Text::Text()
{
  cursor_ = -1;
  line_ = -1;
  updateControlPoints();
}

Text::~Text()
{
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
  if (points_.size() < 2)
    return false;

  // Have we hit a control point?
  if (selected && (hitControlPoint(pos) >= 0))
    return true;

  QRectF textbox(points_.at(0), points_.at(1));
  return textbox.contains(pos);
}

bool Text::hit(const QRectF &bbox) const
{
  if (points_.size() < 2)
    return false;

  QRectF textbox(points_.at(0), points_.at(1));
  return textbox.intersects(bbox);
}

void Text::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                      QSet<QSharedPointer<DrawingItemBase> > *items,
                      const QSet<QSharedPointer<DrawingItemBase> > *selItems,
                      bool *multiItemOp)
{
  if (event->button() == Qt::LeftButton) {
    pressedCtrlPointIndex_ = hitControlPoint(event->pos());
    resizing_ = (pressedCtrlPointIndex_ >= 0);
    moving_ = !resizing_;
    basePoints_ = points_;
    baseMousePos_ = event->pos();

    if (multiItemOp)
      *multiItemOp = moving_; // i.e. a move operation would apply to all selected items
  }
}

void Text::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;

  if (points_.size() < 2) {
    // Create two points: one for the current mouse position and another to be
    // updated during the following move events.
    points_.append(QPointF(event->pos()));
    points_.append(QPointF(event->pos()));
    cursor_ = -1;
  } else
    complete = true;
}

void Text::incompleteMouseMove(QMouseEvent *event, bool &repaintNeeded)
{
  // Set the second point, ensure that there is a corresponding geographic point.
  points_[1] = QPointF(event->pos());
  setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*this));

  repaintNeeded = true;
}

void Text::incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  // Ensure that the first points is above and to the left of the second.
  QRectF rect = QRectF(points_[0], points_[1]).normalized();
  points_[0] = rect.bottomLeft();
  points_[1] = rect.topRight();

  // Update the geographic points and the control points.
  setLatLonPoints(DrawingManager::instance()->getLatLonPoints(*this));
  updateControlPoints();

  repaintNeeded = true;

  // Make the text area editable by using a valid index.
  cursor_ = 0;
  line_ = 0;
  lines_.append(QString());
}

void Text::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if (cursor_ == -1) {
    event->ignore();
    return;
  }

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

  updateControlPoints();
  event->accept();
}

void Text::resize(const QPointF &pos)
{
  const int size = controlPointSize(), size_2 = size / 2;

  switch (pressedCtrlPointIndex_) {
  case 0:
    points_[0] = QPointF(pos.x(), pos.y());
    break;
  case 1:
    points_[0] = QPointF(points_.at(0).x(), pos.y());
    points_[1] = QPointF(pos.x(), points_.at(1).y());
    break;
  case 2:
    points_[1] = QPointF(pos.x(), pos.y());
    break;
  case 3:
    points_[0] = QPointF(pos.x(), points_.at(0).y());
    points_[1] = QPointF(points_.at(1).x(), pos.y());
    break;
  default:
    ;
  }

  updateControlPoints();
}

void Text::updateControlPoints()
{
  if (points_.size() < 2)
    return;

  const int size = controlPointSize(), size_2 = size / 2;
  QRectF bbox(points_.at(0), points_.at(1));
  bbox.adjust(-size_2, -size_2, -size_2, -size_2);

  controlPoints_.clear();
  controlPoints_.append(QRectF(bbox.left(), bbox.top(), size, size));
  controlPoints_.append(QRectF(bbox.right(), bbox.top(), size, size));
  controlPoints_.append(QRectF(bbox.right(), bbox.bottom(), size, size));
  controlPoints_.append(QRectF(bbox.left(), bbox.bottom(), size, size));
}

void Text::setPoints(const QList<QPointF> &points)
{
  setGeometry(points);
}

void Text::drawHoverHighlighting(bool) const
{
  glColor3ub(255, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(points_[0].x(), points_[0].y());
  glVertex2f(points_[1].x(), points_[0].y());
  glVertex2f(points_[1].x(), points_[1].y());
  glVertex2f(points_[0].x(), points_[1].y());
  glEnd();
}

void Text::drawIncomplete() const
{
  if (points_.size() < 2)
    return;

  if (lines_.isEmpty())
    glColor3ub(128, 128, 128);
  else {
    glLineStipple(1, 0x5555);
    glEnable(GL_LINE_STIPPLE);
  }

  glBegin(GL_LINE_LOOP);
  glVertex2f(points_[0].x(), points_[0].y());
  glVertex2f(points_[1].x(), points_[0].y());
  glVertex2f(points_[1].x(), points_[1].y());
  glVertex2f(points_[0].x(), points_[1].y());
  glEnd();

  if (!lines_.isEmpty())
    glDisable(GL_LINE_STIPPLE);

  float x = points_.at(0).x() + margin_;
  float y = points_.at(0).y() - margin_;
  QSizeF size;

  for (int line = 0; line <= line_; ++line) {
    if (line < line_)
      size = getStringSize(lines_.at(line));
    else
      size = getStringSize(lines_.at(line), cursor_);

    y -= size.height() * (1.0 + spacing_);
  }

  if (cursor_ != -1) {
    size = getStringSize(lines_.at(line_), cursor_);
    // Draw a caret.
    glColor3ub(255, 0, 0);
    glBegin(GL_LINES);
    glVertex2f(x + size.width(), y + size.height() + margin_);
    glVertex2f(x + size.width(), y + size.height() * spacing_ - margin_);
    glEnd();
  }
}

} // namespace EditItem_Text
