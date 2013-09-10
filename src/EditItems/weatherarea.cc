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
#include "weatherarea.h"

namespace EditItem_WeatherArea {

static qreal sqr(qreal x) { return x * x; }

static qreal dist2(const QPointF &v, const QPointF &w) { return sqr(v.x() - w.x()) + sqr(v.y() - w.y()); }

// Returns the distance between \a p and the line between \a v and \a w.
static qreal distance2(const QPointF &p, const QPointF &v, const QPointF &w)
{
    const qreal l2 = dist2(v, w);
    if (l2 == 0) return sqrt(dist2(p, v));
    Q_ASSERT(l2 > 0);
    const qreal t = ((p.x() - v.x()) * (w.x() - v.x()) + (p.y() - v.y()) * (w.y() - v.y())) / l2;
    if (t < 0) return sqrt(dist2(p, v));
    if (t > 1) return sqrt(dist2(p, w));
    QPointF p2(v.x() + t * (w.x() - v.x()), v.y() + t * (w.y() - v.y()));
    return sqrt(dist2(p, p2));
}

class SetGeometryCommand : public QUndoCommand
{
public:
    SetGeometryCommand(WeatherArea *, const QList<QPoint> &, const QList<QPoint> &);
private:
    WeatherArea *item_;
    QList<QPoint> oldGeometry_;
    QList<QPoint> newGeometry_;
    virtual void undo();
    virtual void redo();
};

SetGeometryCommand::SetGeometryCommand(
    WeatherArea *item, const QList<QPoint> &oldGeometry, const QList<QPoint> &newGeometry)
    : item_(item)
    , oldGeometry_(oldGeometry)
    , newGeometry_(newGeometry)
{}

void SetGeometryCommand::undo()
{
    item_->setGeometry(oldGeometry_);
    item_->repaint();
}

void SetGeometryCommand::redo()
{
    item_->setGeometry(newGeometry_);
    item_->repaint();
}

WeatherArea::WeatherArea()
{
    init();
    updateControlPoints();
    color_.setRed(0);
    color_.setGreen(0);
    color_.setBlue(0);
}

WeatherArea::~WeatherArea()
{
    delete addPoint_;
    delete remove_;
    delete removePoint_;
}

void WeatherArea::init()
{
  moving_ = false;
  resizing_ = false;
  pressedCtrlPointIndex_ = -1;
  hoveredCtrlPointIndex_ = -1;
  placementPos_ = 0;
  addPoint_ = new QAction(tr("&Add point"), 0);
  remove_ = new QAction(tr("&Remove"), 0);
  removePoint_ = new QAction(tr("Remove &point"), 0);
  type = Cold;
  s_length = 0;

  // Create an arch shape to use repeatedly on the line.
  float flagstep = M_PI/(nwarmflag-1);

  for (int j = 0; j < nwarmflag; j++) {
    xwarmflag[j]= cos(j * flagstep);
    ywarmflag[j]= sin(j * flagstep);
  }
}

EditItemBase *WeatherArea::copy() const
{
    WeatherArea *newItem = new WeatherArea();
    newItem->points_ = points_;
    newItem->controlPoints_ = controlPoints_;
    newItem->basePoints_ = basePoints_;
    return newItem;
}

void WeatherArea::setType(frontType type)
{
    this->type = type;
    repaint();
}

QList<QPoint> WeatherArea::getPoints() const
{
    return points_;
}

void WeatherArea::setPoints(const QList<QPoint> &points)
{
    points_ = points;
}

// Internal/private methods

bool WeatherArea::hit(const QPoint &pos, bool selected) const
{
    const qreal proximityTolerance = 3.0;
    return ((points_.size() >= 2) && (distance(pos) < proximityTolerance)) || (selected && (hitControlPoint(pos) >= 0));
}

bool WeatherArea::hit(const QRect &rect) const
{
    Q_UNUSED(rect);
    return false; // for now
}

/**
 * Returns the index of the line close to the position specified, or -1 if no
 * line was close enough.
 */
int WeatherArea::hitLine(const QPoint &position) const
{
    if (points_.size() < 2)
        return -1;

    const qreal proximityTolerance = 3.0;
    qreal minDist = distance2(position, points_[0], points_[1]);
    int minIndex = 0;
    int n = points_.size();

    for (int i = 1; i < n; ++i) {
        const qreal dist = distance2(QPointF(position), QPointF(points_.at(i)), QPointF(points_.at((i + 1) % n)));
        if (dist < minDist) {
            minDist = dist;
            minIndex = i;
        }
    }
    
    if (minDist > proximityTolerance)
        return -1;

    return minIndex;
}

/**
 * Returns the index of the line close to the position specified, or -1 if no
 * line was close enough.
 */
int WeatherArea::hitPoint(const QPoint &position) const
{
    if (points_.size() == 0)
        return -1;

    const qreal proximityTolerance = 3.0;
    qreal minDist = sqrt(dist2(QPointF(position), QPointF(points_.at(0))));
    int minIndex = 0;

    for (int i = 1; i < points_.size(); ++i) {
        const qreal dist = sqrt(dist2(QPointF(position), QPointF(points_.at(i))));
        if (dist < minDist) {
            minDist = dist;
            minIndex = i;
        }
    }

    if (minDist > proximityTolerance)
        return -1;

    return minIndex;
}

void WeatherArea::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                              QSet<EditItemBase *> *items, bool *multiItemOp)
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
        if (items) {
            // open a context menu and perform the selected action
            QMenu contextMenu;
            QPoint position = event->pos();

            // Add actions, checking for a click on a line or a point.
            int lineIndex = hitLine(position);
            int pointIndex = hitPoint(position);
            if (lineIndex != -1)
                contextMenu.addAction(addPoint_);
            if (pointIndex != -1)
                contextMenu.addAction(removePoint_);
            contextMenu.addAction(tr("&Remove"));

            QAction *action = contextMenu.exec(event->globalPos(), remove_);
            if (action == remove_)
                remove(repaintNeeded, items);
            else if (action == removePoint_)
                removePoint(repaintNeeded, pointIndex, items);
            else if (action == addPoint_)
                addPoint(repaintNeeded, lineIndex, position, items);
        }
    }
}

void WeatherArea::mouseRelease(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands)
{
    Q_UNUSED(event); 
    Q_UNUSED(repaintNeeded); // no need to set this
    Q_ASSERT(undoCommands);
    if ((moving_ || resizing_) && (geometry() != baseGeometry()))
        undoCommands->append(new SetGeometryCommand(this, baseGeometry(), geometry()));
    moving_ = resizing_ = false;
}

void WeatherArea::mouseMove(QMouseEvent *event, bool &repaintNeeded)
{
    if (moving_) {
        move(event->pos());
        repaintNeeded = true;
    } else if (resizing_) {
        resize(event->pos());
        repaintNeeded = true;
    }
}

void WeatherArea::mouseHover(QMouseEvent *event, bool &repaintNeeded)
{
    hoveredCtrlPointIndex_ = hitControlPoint(event->pos());
    repaintNeeded = true;
}

void WeatherArea::keyPress(QKeyEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands, QSet<EditItemBase *> *items)
{
    Q_UNUSED(repaintNeeded); // no need to set this
    Q_UNUSED(undoCommands); // not used, since the key press currently doesn't modify this item
    // (it may mark it for removal, but adding and removing items is handled on the outside)

    if (items && ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete))) {
        Q_ASSERT(items->contains(this));
        items->remove(this);
    }
}

void WeatherArea::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
    Q_UNUSED(complete);
    Q_UNUSED(aborted);

    if (event->button() == Qt::LeftButton) {

        if (!placementPos_)
            placementPos_ = new QPoint(event->pos());
        points_.append(QPoint(event->pos()));
        updateControlPoints();
        repaintNeeded = true;

    } else if (event->button() == Qt::RightButton) {

        if (points_.size() < 1) {
            // There must be at least one point in the multiline.
            aborted = true;
            repaintNeeded = true;
        }

        Q_ASSERT(placementPos_);
        delete placementPos_;
        placementPos_ = 0;

        if (points_.size() >= 2) {
            complete = true; // causes repaint
        } else {
            aborted = true; // not a complete multiline
            repaintNeeded = true;
        }
    }
}

void WeatherArea::incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded)
{
    if (placementPos_) {
        *placementPos_ = event->pos();
        repaintNeeded = true;
    }
}

void WeatherArea::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
    Q_UNUSED(repaintNeeded);
    Q_UNUSED(complete);
    if (placementPos_ && ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter))) {
        if (points_.size() >= 2) {
            complete = true; // causes repaint
        } else {
            aborted = true; // not a complete multiline
            repaintNeeded = true;
        }
        delete placementPos_;
        placementPos_ = 0;
    } else if (event->key() == Qt::Key_Escape) {
        aborted = true;
        if (placementPos_) {
            delete placementPos_;
            placementPos_ = 0;
            repaintNeeded = true;
        }
    }
}

// Returns the index (>= 0)  of the control point hit by \a pos, or -1 if no
// control point was hit.
int WeatherArea::hitControlPoint(const QPoint &pos) const
{
    for (int i = 0; i < controlPoints_.size(); ++i)
        if (controlPoints_.at(i).contains(pos))
            return i;
    return -1;
}

void WeatherArea::move(const QPoint &pos)
{
    const QPoint delta = pos - baseMousePos_;
    Q_ASSERT(basePoints_.size() == points_.size());
    for (int i = 0; i < points_.size(); ++i)
        points_[i] = basePoints_.at(i) + delta;
    updateControlPoints();
}

void WeatherArea::resize(const QPoint &pos)
{
    const QPoint delta = pos - baseMousePos_;
    Q_ASSERT(pressedCtrlPointIndex_ >= 0);
    Q_ASSERT(pressedCtrlPointIndex_ < controlPoints_.size());
    Q_ASSERT(basePoints_.size() == points_.size());
    points_[pressedCtrlPointIndex_] = basePoints_.at(pressedCtrlPointIndex_) + delta;
    updateControlPoints();
}

void WeatherArea::updateControlPoints()
{
    controlPoints_.clear();
    const int size = 10, size_2 = size / 2;
    foreach (QPoint p, points_)
        controlPoints_.append(QRect(p.x() - size_2, p.y() - size_2, size, size));
}

void WeatherArea::addPoint(bool &repaintNeeded, int index, const QPoint &point, QSet<EditItemBase *> *items)
{
    Q_ASSERT(items);
    Q_ASSERT(items->contains(this));

    points_.insert(index + 1, point);

    updateControlPoints();
    repaintNeeded = true;
}

void WeatherArea::remove(bool &repaintNeeded, QSet<EditItemBase *> *items)
{
    Q_ASSERT(items);
    Q_ASSERT(items->contains(this));

    // Option 1: remove this item only:
    // items->remove(this);

    // Option 2: remove all items:
    QSet<EditItemBase *>::iterator i = items->begin();
    while (i != items->end())
        i = items->erase(i);

    repaintNeeded = true;
}

void WeatherArea::removePoint(bool &repaintNeeded, int index, QSet<EditItemBase *> *items)
{
    Q_ASSERT(items);
    Q_ASSERT(items->contains(this));

    if (points_.size() <= 3)
        items->remove(this);

    if (index >= 0 && index < points_.size())
        points_.removeAt(index);

    hoveredCtrlPointIndex_ = -1;
    updateControlPoints();
    repaintNeeded = true;
}

void WeatherArea::setGeometry(const QList<QPoint> &points)
{
    points_ = points;
    updateControlPoints();
}

// Returns the distance between \a p and the multiline (i.e. the mimimum distance between \a p and any of the line segments).
// If the multiline contains fewer than two points, the function returns -1.
qreal WeatherArea::distance(const QPoint &p) const
{
    if (points_.size() < 2)
        return -1;

    int n = points_.size();

    qreal minDist = -1;
    for (int i = 1; i <= n; ++i) {
        const qreal dist = distance2(QPointF(p), QPointF(points_.at(i - 1)), QPointF(points_.at(i % n)));
        minDist = (i == 1) ? dist : qMin(minDist, dist);
    }
    Q_ASSERT(minDist >= 0);
    return minDist;
}

void WeatherArea::draw(DrawModes modes, bool incomplete)
{
    if (incomplete) {
        if (placementPos_ == 0) {
            return;
        } else {
            // draw the line from the end of the multiline to the current placement position
            glBegin(GL_LINES);
            glColor3ub(0, 0, 255);
            glVertex2i(points_.last().x(), points_.last().y());
            glVertex2i(placementPos_->x(), placementPos_->y());
            glEnd();
        }
    }

    // draw the basic item
    glBegin(GL_LINE_LOOP);
    glColor3ub(color_.red(), color_.green(), color_.blue());
    foreach (QPoint p, points_)
        glVertex2i(p.x(), p.y());
    glEnd();

    // draw control points if we're selected
    if (modes & Selected)
        drawControlPoints();

    // draw highlighting if we're hovered
    if (modes & Hovered)
        drawHoverHighlighting(incomplete);
}

void WeatherArea::drawControlPoints()
{
    glColor3ub(0, 0, 0);
    foreach (QRect c, controlPoints_) {
        glBegin(GL_POLYGON);
        glVertex3i(c.left(),  c.bottom(), 1);
        glVertex3i(c.right(), c.bottom(), 1);
        glVertex3i(c.right(), c.top(),    1);
        glVertex3i(c.left(),  c.top(),    1);
        glEnd();
    }
}

void WeatherArea::drawHoverHighlighting(bool incomplete)
{
    const int pad = 1;
    if (incomplete)
        glColor3ub(0, 200, 0);
    else
        glColor3ub(255, 0, 0);

    if (hoveredCtrlPointIndex_ >= 0) {
        // highlight a control point
        const QRect *r = &controlPoints_.at(hoveredCtrlPointIndex_);
        glPushAttrib(GL_LINE_BIT);
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex3i(r->left() - pad,  r->bottom() + pad, 1);
        glVertex3i(r->right() + pad, r->bottom() + pad, 1);
        glVertex3i(r->right() + pad, r->top() - pad, 1);
        glVertex3i(r->left() - pad,  r->top() - pad, 1);
        glEnd();
        glPopAttrib();
    } else {
        // highlight the multiline itself
        glPushAttrib(GL_LINE_BIT);
        glLineWidth(4);
        glBegin(GL_LINE_LOOP);
        foreach (QPoint p, points_)
            glVertex3i(p.x(), p.y(), 1);
        glEnd();
        glPopAttrib();
    }
}

} // namespace EditItem_WeatherArea
