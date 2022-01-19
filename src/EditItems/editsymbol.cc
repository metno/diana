/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2021 met.no

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

#include "editsymbol.h"

#include "EditItems/drawingstylemanager.h"
#include "diColour.h"
#include "diGLPainter.h"
#include "drawingsymbol.h"

#include <mi_fieldcalc/math_util.h>

#include <QAction>
#include <QMenu>

namespace EditItem_Symbol {

Symbol::Symbol(int id)
  : DrawingItem_Symbol::Symbol(id)
{
  init();
  updateControlPoints();
}

Symbol::~Symbol()
{
}

DrawingItemBase *Symbol::cloneSpecial(bool setUniqueId) const
{
  Symbol *item = new Symbol(setUniqueId ? -1 : id());
  copyBaseData(item);
  return item;
}

DrawingItemBase::HitType Symbol::hit(const QPointF &pos, bool selected) const
{
  const bool hitSelectedControlPoint = selected && (hitControlPoint(pos) >= 0);
  if (hitSelectedControlPoint)
    return Area;
  else
    return DrawingItem_Symbol::Symbol::hit(pos, selected);
}

// ### similar to PolyLine::mousePress - move common code to base class?
void Symbol::mousePress(QMouseEvent *event, bool &repaintNeeded, bool *multiItemOp)
{
  if (event->button() == Qt::LeftButton) {
    mousePressControlPoints(event, repaintNeeded);
    resizing_ = !pressedCtrlPointIndex_.isEmpty();
    moving_ = !resizing_;
    basePoints_ = points_;
    baseMousePos_ = event->pos();

    if (multiItemOp)
      *multiItemOp = moving_; // i.e. a move operation would apply to all selected items

    EditItemBase::mousePress(event, repaintNeeded, multiItemOp);
  }
}

void Symbol::mouseRelease(QMouseEvent *event, bool &repaintNeeded)
{
  if (resizing_)
    repaintNeeded = true;

  EditItemBase::mouseRelease(event, repaintNeeded);
}

void Symbol::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  if (event->button() == Qt::LeftButton) {
    Q_ASSERT(points_.isEmpty());
    points_.append(QPointF(event->pos().x(), event->pos().y()));
    complete = true;
    repaintNeeded = true;
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
  const QPointF delta = pos - points_.at(0);
  properties_["size"] = miutil::absval(delta.x(), delta.y());
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

void Symbol::drawHoverHighlightingBG(DiGLPainter* gl, bool incomplete, bool selected) const
{
  if (incomplete)
    return;

  // highlight the bounding box boundary
  bool ok = false;
  const int lineWidth = properties().value("style:linewidth").toInt(&ok);
  const int defaultLineWidth = 2;
  const int pad = 6;
  DrawingStyleManager::instance()->highlightPolyLine(gl, this, boundingSquare(),
      (ok ? lineWidth : defaultLineWidth) + pad, QColor(255, 255, 0, 180), true);

  // highlight the control points
  drawControlPoints(gl, QColor(255, 0, 0, 255));
}

void Symbol::drawHoverHighlighting(DiGLPainter* gl, bool incomplete, bool selected) const
{
  if (incomplete)
    return;

  if (hoverCtrlPointIndex_ >= 0) {
    // highlight the control point
    drawHoveredControlPoint(gl, QColor(255, 0, 0, 255), 2);
    drawHoveredControlPoint(gl, QColor(255, 255, 0, 255));
  } else {
    // highlight the control points
    if (selected)
      drawControlPoints(gl, QColor(255, 0, 0, 255));
  }
}

/**
 * Override the draw method in the corresponding drawing class to only draw
 * the bounding rectangle when resizing.
 */
void Symbol::draw(DiGLPainter* gl)
{
  if (points_.isEmpty())
    return;

  if (resizing_) {
    // Draw the bounding box instead of the symbol because it can be expensive
    // to render the SVG for every single intermediate size during the
    // resizing process.
    const QList<QPointF> bbox = boundingSquare();

    gl->setColour(Colour::RED);
    gl->PushAttrib(DiGLPainter::gl_LINE_BIT);
    gl->LineWidth(2);
    gl->Begin(DiGLPainter::gl_LINE_LOOP);
    foreach (QPointF p, boundingSquare())
      gl->Vertex3i(p.x(), p.y(), 1);
    gl->End();
    gl->PopAttrib();

  } else
    DrawingItem_Symbol::Symbol::draw(gl);
}

} // namespace EditItem_Symbol
