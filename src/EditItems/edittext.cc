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
#include <QDebug>

namespace EditItem_Text {

Text::Text()
{
  cursor_ = 0;
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
  if (points_.isEmpty())
    return false;

  QSizeF size = getStringSize();
  QRectF textbox(points_.at(0), size);

  return textbox.contains(pos);
}

bool Text::hit(const QRectF &bbox) const
{
  if (points_.isEmpty())
    return false;

  QSizeF size = getStringSize();
  QRectF textbox(points_.at(0), size);

  return textbox.intersects(bbox);
}

void Text::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                      QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems,
                      bool *multiItemOp)
{
}

void Text::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  repaintNeeded = true;

  points_.clear();
  points_.append(QPointF(event->pos()));
}

void Text::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  switch (event->key()) {
  case Qt::Key_Escape:
    aborted = true;
    break;
  case Qt::Key_Return:
    complete = true;
    break;
  case Qt::Key_Backspace:
    if (cursor_ > 0) {
      text_ = text_.left(cursor_ - 1) + text_.mid(cursor_);
      cursor_ -= 1;
    }
    break;
  case Qt::Key_Delete:
    if (cursor_ <= text_.size() - 1)
      text_ = text_.left(cursor_) + text_.mid(cursor_ + 1);
    break;
  case Qt::Key_Left:
    cursor_ -= 1;
    break;
  case Qt::Key_Right:
    cursor_ += 1;
    break;
  default:
    text_.insert(cursor_, event->text());
    cursor_ += 1;
  }

  event->accept();
}

void Text::resize(const QPointF &)
{

}

void Text::updateControlPoints()
{
  if (points_.isEmpty())
    return;

  const int size = controlPointSize(), size_2 = size / 2;
  QSizeF string_size = getStringSize();
  QRectF bbox(points_.at(0), string_size);
  bbox.adjust(-size_2, -size_2, -size_2, -size_2);

  controlPoints_.clear();
  controlPoints_.append(QRectF(bbox.left(), bbox.top(), size, size));
  controlPoints_.append(QRectF(bbox.right(), bbox.top(), size, size));
  controlPoints_.append(QRectF(bbox.right(), bbox.bottom(), size, size));
  controlPoints_.append(QRectF(bbox.left(), bbox.bottom(), size, size));
}

void Text::drawHoverHighlighting(bool) const
{
  QSizeF size = getStringSize();
  size.setHeight(qMax(size.height(), qreal(poptions.fontsize)));
  QRectF textbox(points_.at(0), size);

  glColor4ub(255, 0, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(textbox.left(), textbox.top());
  glVertex2f(textbox.right(), textbox.top());
  glVertex2f(textbox.right(), textbox.bottom());
  glVertex2f(textbox.left(), textbox.bottom());
  glEnd();
}

void Text::drawIncomplete() const
{
  if (points_.isEmpty())
    return;

  const int margin = 4;

  QSizeF size = getStringSize();
  size.setHeight(qMax(size.height(), qreal(poptions.fontsize)));

  // Draw a caret.
  glColor3f(1.0, 0.0, 0.0);
  glBegin(GL_LINES);
  glVertex2f(points_.at(0).x() + size.width(), points_.at(0).y() - margin);
  glVertex2f(points_.at(0).x() + size.width(), points_.at(0).y() + size.height() + margin);
  glEnd();
}

QSizeF Text::getStringSize() const
{
  float width, height;
  GLfloat s = qMax(pwidth/maprect.width(), pheight/maprect.height());
  fp->set(poptions.fontname, poptions.fontface, poptions.fontsize * s);
  fp->getStringSize(text_.left(cursor_).toStdString().c_str(), width, height);

  return QSizeF(width, height);
}

} // namespace EditItem_Text
