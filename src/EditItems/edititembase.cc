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

#include "edititembase.h"
#include "diColour.h"
#include "diGLPainter.h"
#include "qtUtility.h"

#include <qglobal.h>

EditItemBase::EditItemBase()
  : moving_(false)
  , resizing_(false)
{}

QPointF EditItemBase::hoverPos() const
{
  return hoverPos_;
}

void EditItemBase::init()
{
  moving_ = false;
  resizing_ = false;
  pressedCtrlPointIndex_.clear();
  hoverCtrlPointIndex_ = -1;
}

void EditItemBase::copyBaseData(EditItemBase *item) const
{
  item->setGeometry(geometry());
}

QList<QPointF> EditItemBase::geometry() const
{
  return ConstDrawing(this)->points_;
}

void EditItemBase::setGeometry(const QList<QPointF> &points)
{
  Drawing(this)->points_ = points;
  updateControlPoints();
}

QList<QPointF> EditItemBase::baseGeometry() const
{
  return basePoints_;
}

QList<QPointF> EditItemBase::getBasePoints() const
{
  return baseGeometry();
}

// Returns the index (>= 0)  of the control point hit by \a pos, or -1 if no
// control point was hit.
int EditItemBase::hitControlPoint(const QPointF &pos) const
{
  for (int i = 0; i < controlPoints_.size(); ++i)
    if (controlPoints_.at(i).contains(pos))
      return i;
  return -1;
}

// Moves the item by the specified amount (i.e. \a pos is relative to the item's current position).
void EditItemBase::moveBy(const QPointF &pos)
{
  baseMousePos_ = QPointF();
  basePoints_ = Drawing(this)->points_;
  move(pos);
}

void EditItemBase::move(const QPointF &pos)
{
  const QPointF delta = pos - baseMousePos_;
  Q_ASSERT(basePoints_.size() == Drawing(this)->points_.size());
  for (int i = 0; i < Drawing(this)->points_.size(); ++i)
    Drawing(this)->points_[i] = basePoints_.at(i) + delta;
}

// Moves the point at index \a i to the new position \a pos.
void EditItemBase::movePointTo(int i, const QPointF &pos)
{
  Q_ASSERT(i >= 0);
  Q_ASSERT(i < Drawing(this)->points_.size());
  Drawing(this)->points_[i] = pos;
  Drawing(this)->setLatLonPoints(DrawingManager::instance()->getLatLonPoints((Drawing(this))));
}

static void drawRect(DiGLPainter* gl, const QRectF &r, int pad, int z = 1)
{
  gl->Begin(DiGLPainter::gl_POLYGON);
  gl->Vertex3i(r.left() - pad,  r.bottom() + pad, z);
  gl->Vertex3i(r.right() + pad, r.bottom() + pad, z);
  gl->Vertex3i(r.right() + pad, r.top() - pad,    z);
  gl->Vertex3i(r.left() - pad,  r.top() - pad,    z);
  gl->End();
}

static bool isJoinedEndPoint(int joinCount, int joinId, int i, int n)
{
  return (joinCount > 1) && (((joinId < 0) && (i == 0)) || ((joinId > 0) && (i == (n - 1))));
}

void EditItemBase::drawControlPoints(DiGLPainter* gl, const QColor &color, const QColor &joinColor, int pad) const
{
  gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

  const Colour joinC = diutil::fromQColor(joinColor);
  const Colour colorC = diutil::fromQColor(color);

  const int jId = ConstDrawing(this)->joinId();
  const int jCount = ConstDrawing(this)->joinCount();
  const int n = controlPoints_.size();
  for (int i = 0; i < n; ++i) {
    int extraPad = 0;
    if (isJoinedEndPoint(jCount, jId, i, n)) {
      gl->setColour(joinC);
      extraPad = 1;
    } else if (pressedCtrlPointIndex_.contains(i))
      gl->setColour(Colour::RED);
    else
      gl->setColour(colorC);

    drawRect(gl, controlPoints_.at(i), pad + extraPad);
  }

  gl->PopAttrib();
}

void EditItemBase::drawHoveredControlPoint(DiGLPainter* gl, const QColor &color, int pad) const
{
  Q_ASSERT(hoverCtrlPointIndex_ >= 0);
  gl->PushAttrib(DiGLPainter::gl_POLYGON_BIT);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  gl->setColour(diutil::fromQColor(color));
  drawRect(gl,
        controlPoints_.at(hoverCtrlPointIndex_),
        pad + (isJoinedEndPoint(ConstDrawing(this)->joinCount(), ConstDrawing(this)->joinId(), hoverCtrlPointIndex_, controlPoints_.size()) ? 1 : 0));
  gl->PopAttrib();
}

/**
 * Draws the item.
 * \a modes indicates whether the item is selected, hovered, both, or neither.
 * \a incomplete is true iff the item is in the process of being completed (i.e. during manual placement of a new item).
 * \a editingStyle is true iff the item's style is currently being edited.
 */
void EditItemBase::draw(DiGLPainter* gl, DrawModes modes, bool incomplete, bool editingStyle)
{
  // NOTE: if we're editing the style, highlighting and control points would only be in the way

  if ((!editingStyle) && (modes & Hovered))
    drawHoverHighlightingBG(gl, incomplete, modes & Selected);

  Drawing(this)->draw(gl);

  if (incomplete)
    drawIncomplete(gl);

  if (!editingStyle) {
    if (modes & Hovered)
      drawHoverHighlighting(gl, incomplete, modes & Selected);
    if (modes & Selected)
      drawControlPoints(gl);
  }
}

/**
 * The default draw method only draws normal, unselected, non-hovered, complete items.
 */
void EditItemBase::draw(DiGLPainter* gl)
{
  draw(gl, Normal, false);
}

/**
 * Handles a mouse press event for an item in its normal state.
 *
 * \a event is the event.
 *
 * \a repaintNeeded is set to true iff the scene needs to be repainted (typically if the item
 * modified it's state in a way that is reflected visually).
 *
 * \a multiItemOp is, if non-null, set to true iff the event starts an operation that may involve
 * other items (such as a move operation).
 */
void EditItemBase::mousePress(QMouseEvent *event, bool &repaintNeeded, bool *multiItemOp)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
  Q_UNUSED(multiItemOp)
}

/**
 * Handles a mouse press \a event where the click may intersect with a control point,
 * updating the set of selected points and updating the \a repaintNeeded flag as
 * necessary.
 */
void EditItemBase::mousePressControlPoints(QMouseEvent *event, bool &repaintNeeded)
{
  if (!(event->modifiers() & Qt::ControlModifier)) {
    pressedCtrlPointIndex_.clear();
    repaintNeeded = true;
  }

  int index = hitControlPoint(event->pos());
  if (index > -1) {
    pressedCtrlPointIndex_.insert(index);
    repaintNeeded = true;
  }
}

/**
 * Handles a mouse press event for an item in the process of being completed (i.e. during manual
 * placement of a new item).
 *
 * \a complete is set to true iff the item is in a complete state upon returning from the function.
 *
 * \a aborted is set to true iff completing the item should be cancelled. This causes the item to
 * be deleted.
 *
 * See mousePress() for the documentation of other arguments.
 */
void EditItemBase::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
  Q_UNUSED(complete)
  Q_UNUSED(aborted)
}

void EditItemBase::mouseRelease(QMouseEvent *event, bool &repaintNeeded)
{
  Q_UNUSED(event);
  Q_UNUSED(repaintNeeded); // no need to set this
  moving_ = resizing_ = false;
}

void EditItemBase::mouseMove(QMouseEvent *event, bool &repaintNeeded)
{
  if (moving_) {
    move(event->pos());
    repaintNeeded = true;
  } else if (resizing_) {
    resize(event->pos());
    repaintNeeded = true;
  }
}

void EditItemBase::mouseHover(QMouseEvent *event, bool &repaintNeeded, bool)
{
  hoverCtrlPointIndex_ = hitControlPoint(event->pos());
  repaintNeeded = true;
}

void EditItemBase::mouseDoubleClick(QMouseEvent *event, bool &repaintNeeded)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
}

void EditItemBase::keyPress(QKeyEvent *event, bool &repaintNeeded)
{
  if (event->modifiers() & Qt::ShiftModifier) {
    QPointF pos;
    const qreal nudgeVal = 1; // nudge item by this much
    switch (event->key()) {
    case Qt::Key_Left:
      pos += QPointF(-nudgeVal, 0);
      break;
    case Qt::Key_Right:
      pos += QPointF(nudgeVal, 0);
      break;
    case Qt::Key_Down:
      pos += QPointF(0, -nudgeVal);
      break;
    case Qt::Key_Up:
      pos += QPointF(0, nudgeVal);
      break;
    default:
      return;
    }

    moveBy(pos);
    repaintNeeded = true;
    event->accept();
  }
}

void EditItemBase::keyRelease(QKeyEvent *event, bool &repaintNeeded)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
}

void EditItemBase::incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
  Q_UNUSED(complete)
  Q_UNUSED(aborted)
}

void EditItemBase::incompleteMouseMove(QMouseEvent *event, bool &repaintNeeded)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
}

void EditItemBase::incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
}

void EditItemBase::incompleteMouseDoubleClick(QMouseEvent *event, bool &repaintNeeded, bool &complete,
                                              bool &aborted)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
  Q_UNUSED(complete)
  Q_UNUSED(aborted)
}

void EditItemBase::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
  Q_UNUSED(complete)
  Q_UNUSED(aborted)
}

void EditItemBase::incompleteKeyRelease(QKeyEvent *event, bool &repaintNeeded)
{
  Q_UNUSED(event)
  Q_UNUSED(repaintNeeded)
}

QVariantMap EditItemBase::clipboardVarMap() const
{
  QVariantMap vmap = ConstDrawing(this)->properties();
  vmap.insert("type", metaObject()->className());
  QVariantList vpoints;
  foreach (QPointF p, ConstDrawing(this)->getLatLonPoints())
    vpoints.append(p);
  vmap.insert("points", vpoints);
  return vmap;
}

QString EditItemBase::clipboardPlainText() const
{
  QString s;
  foreach (QPointF p, DrawingManager::instance()->PhysToGeo(ConstDrawing(this)->getPoints()))
    s.append(QString("(%1, %2) ").arg(p.x()).arg(p.y()));
  return s;
}

QList<QAction *> EditItemBase::actions(const QPoint &) const
{
  return QList<QAction *>();
}

void EditItemBase::updateHoverPos(const QPoint &pos)
{
  hoverPos_ = pos;
}
