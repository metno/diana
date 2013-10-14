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
#include "weatherfront.h"

#define DRAW_COLD \
  { \
    glBegin(GL_POLYGON); \
    glVertex2f(xstart, ystart); \
    glVertex2f(xend, yend); \
    glVertex2f(xtop, ytop); \
    glEnd(); \
  }

#define DRAW_WARM \
  { \
    glPushMatrix(); \
    glTranslatef(xm, ym, 0.0); \
    glRotatef(atan2(dys,dxs)*180./M_PI,0.0,0.0,1.0); \
    glBegin(GL_POLYGON); \
    for (j=0; j<nwarmflag; j++) \
      glVertex2f(r * xwarmflag[j], r * ywarmflag[j]); \
    glEnd(); \
    glPopMatrix(); \
  }

namespace EditItem_WeatherFront {

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

WeatherFront::WeatherFront()
{
    init();
    updateControlPoints();
    color_.setRed(0);
    color_.setGreen(0);
    color_.setBlue(0);
}

WeatherFront::~WeatherFront()
{
    delete remove_;
    delete split_;
    delete merge_;
}

void WeatherFront::init()
{
  pressedCtrlPointIndex_ = -1;
  hoveredCtrlPointIndex_ = -1;
  placementPos_ = 0;
  remove_ = new QAction(tr("Remove"), 0);
  split_ = new QAction(tr("Split"), 0);
  merge_ = new QAction(tr("Merge"), 0);
  type = Cold;
  s_length = 0;

  // Create an arch shape to use repeatedly on the line.
  float flagstep = M_PI/(nwarmflag-1);

  for (int j = 0; j < nwarmflag; j++) {
    xwarmflag[j]= cos(j * flagstep);
    ywarmflag[j]= sin(j * flagstep);
  }
}

EditItemBase *WeatherFront::copy() const
{
    WeatherFront *newItem = new WeatherFront();
    newItem->points_ = points_;
    newItem->controlPoints_ = controlPoints_;
    newItem->basePoints_ = basePoints_;
    return newItem;
}

void WeatherFront::setType(frontType type)
{
    this->type = type;
    repaint();
}

QList<QPointF> WeatherFront::getPoints() const
{
    return geometry();
}

void WeatherFront::setPoints(const QList<QPointF> &points)
{
    setGeometry(points);
}

QList<QPointF> WeatherFront::baseGeometry() const
{
    return basePoints_;
}

QList<QPointF> WeatherFront::getBasePoints() const
{
    return baseGeometry();
}

bool WeatherFront::hit(const QPointF &pos, bool selected) const
{
    const qreal proximityTolerance = 3.0;
    return ((points_.size() >= 2) && (distance(pos) < proximityTolerance)) || (selected && (hitControlPoint(pos) >= 0));
}

bool WeatherFront::hit(const QRectF &rect) const
{
    Q_UNUSED(rect);
    return false; // for now
}

/**
 * Returns the index of the line close to the position specified, or -1 if no
 * line was close enough.
 */
int WeatherFront::hitLine(const QPointF &position) const
{
    if (points_.size() < 2)
        return -1;

    const qreal proximityTolerance = 3.0;
    qreal minDist = distance2(position, points_[0], points_[1]);
    int minIndex = 0;

    for (int i = 1; i < points_.size() - 1; ++i) {
        const qreal dist = distance2(QPointF(position), QPointF(points_.at(i)), QPointF(points_.at(i + 1)));
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
int WeatherFront::hitPoint(const QPointF &position) const
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

void WeatherFront::mousePress(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
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
            contextMenu.addAction(remove_);
            contextMenu.addAction(split_);
            // Add a merge action if there is more than one item selected.
            // This also needs to check if both fronts have points under
            // the cursor.
            if (items->size() > 1)
                contextMenu.addAction(merge_);
            QAction *action = contextMenu.exec(event->globalPos(), remove_);
            if (action == remove_)
                remove(repaintNeeded, items);
            else if (action == split_)
                split(event->pos(), repaintNeeded, undoCommands, items);
            else if (action == merge_)
                merge(event->pos(), repaintNeeded, undoCommands, items);
        }
    }
}

void WeatherFront::mouseMove(QMouseEvent *event, bool &repaintNeeded)
{
    if (moving_) {
        move(event->pos());
        repaintNeeded = true;
    } else if (resizing_) {
        resize(event->pos());
        repaintNeeded = true;
    }
}

void WeatherFront::mouseHover(QMouseEvent *event, bool &repaintNeeded)
{
    hoveredCtrlPointIndex_ = hitControlPoint(event->pos());
    repaintNeeded = true;
}

void WeatherFront::incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
{
    Q_UNUSED(complete);
    Q_UNUSED(aborted);

    if (event->button() == Qt::LeftButton) {

        if (!placementPos_)
            placementPos_ = new QPointF(event->pos());
        points_.append(QPointF(event->pos()));
        updateControlPoints();
        repaintNeeded = true;

    } else if (event->button() == Qt::RightButton) {

        if (placementPos_) { // no longer needed, since placement will now either be aborted or completed
            delete placementPos_;
            placementPos_ = 0;
        }

        if (points_.size() < 1) {
            // at least one point is required to form a valid multiline
            aborted = true;
            repaintNeeded = true;
        } else {
            points_.append(QPointF(event->pos())); // the current mouse pos forms the last point
            updateControlPoints();
            complete = true; // causes repaint
        }
    }
}

void WeatherFront::incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded)
{
    if (placementPos_) {
        *placementPos_ = event->pos();
        repaintNeeded = true;
    }
}

void WeatherFront::incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted)
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

void WeatherFront::moveBy(const QPointF &pos)
{
    baseMousePos_ = QPointF();
    basePoints_ = points_;
    move(pos);
}

// Returns the index (>= 0)  of the control point hit by \a pos, or -1 if no
// control point was hit.
int WeatherFront::hitControlPoint(const QPointF &pos) const
{
    for (int i = 0; i < controlPoints_.size(); ++i)
        if (controlPoints_.at(i).contains(pos))
            return i;
    return -1;
}

void WeatherFront::move(const QPointF &pos)
{
    const QPointF delta = pos - baseMousePos_;
    Q_ASSERT(basePoints_.size() == points_.size());
    for (int i = 0; i < points_.size(); ++i)
        points_[i] = basePoints_.at(i) + delta;
    updateControlPoints();
}

void WeatherFront::resize(const QPointF &pos)
{
    const QPointF delta = pos - baseMousePos_;
    Q_ASSERT(pressedCtrlPointIndex_ >= 0);
    Q_ASSERT(pressedCtrlPointIndex_ < controlPoints_.size());
    Q_ASSERT(basePoints_.size() == points_.size());
    points_[pressedCtrlPointIndex_] = basePoints_.at(pressedCtrlPointIndex_) + delta;
    updateControlPoints();
}

void WeatherFront::updateControlPoints()
{
    controlPoints_.clear();
    const int size = 10, size_2 = size / 2;
    foreach (QPointF p, points_)
        controlPoints_.append(QRectF(p.x() - size_2, p.y() - size_2, size, size));
}

void WeatherFront::remove(bool &repaintNeeded, QSet<EditItemBase *> *items)
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

// Splits all items of type Rectangle in two, including this item.
void WeatherFront::split(const QPointF &position, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                         QSet<EditItemBase *> *items)
{
    // Check for a split at a point.
    int index = hitPoint(position);
    
    // Don't split at the ends of the front.
    if (index == 0 || index == points_.size() - 1)
        return;
    
    WeatherFront *newFront = new WeatherFront();
    QList<QPointF> newFront_points;
    
    bool atPoint = false;
    int beforeIndex;
    int afterIndex;

    if (index == -1) {
        // Check for a split on a line.
        index = hitLine(position);
        
        // If no lines were hit then return.
        if (index == -1)
            return;

        // Since we split a line then insert the point where the split occurred.
        newFront_points.append(position);
        beforeIndex = index;
        afterIndex = index + 1;

    } else {
        atPoint = true;
        beforeIndex = index;
        afterIndex = index;
    }

    repaintNeeded = true;

    // Create a list of points that precede the split.
    QList<QPointF> new_points = points_.mid(0, beforeIndex + 1);
    // If the split occurred on a line then include the click position.
    if (!atPoint)
        new_points.append(position);

    // Add the points following the split to the new front.
    for (int i = afterIndex; i < points_.size(); ++i)
        newFront_points.append(points_.at(i));

    newFront->setGeometry(newFront_points);
    items->insert(newFront);

    undoCommands->append(new SetGeometryCommand(this, points_, new_points));
}

// Merges items of type Rectangle into one by
//   1) resizing this item to the bounding box of all items of type Rectangle, and
//   2) removing all items of type Rectangle but this item
void WeatherFront::merge(const QPointF &position, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
                         QSet<EditItemBase *> *items)
{
    // FOR NOW:
    Q_UNUSED(repaintNeeded);
    Q_UNUSED(undoCommands);
    Q_UNUSED(items);

    int index = hitPoint(position);

    // Exit if no points in this weather front are close to the cursor position.
    if (index != 0 && index != points_.size() - 1)
        return;

    // Find another front with a matching point.
    foreach (EditItemBase *item, *items) {

        WeatherFront *front = qobject_cast<WeatherFront *>(item);

        if (front && front != this) {
            int otherIndex = front->hitPoint(position);

            // Only connect to the end of the other front.
            QList<QPointF> new_points = points_;

            if (otherIndex == 0) {
                for (int i = 1; i < front->points_.size(); ++i) {
                    if (index == 0)
                        new_points.insert(0, front->points_.at(i));
                    else
                        new_points.append(front->points_.at(i));
                }
            } else if (otherIndex == front->points_.size() - 1) {
                for (int i = front->points_.size() - 2; i >= 0 ; --i) {
                    if (index == 0)
                        new_points.insert(0, front->points_.at(i));
                    else
                        new_points.append(front->points_.at(i));
                }
            } else
                continue;

            // Remove the other front.
            items->remove(item);

            // Set the geometry of the merged front.
            undoCommands->append(new SetGeometryCommand(this, points_, new_points));
            return;
        }
    }
}

void WeatherFront::setGeometry(const QList<QPointF> &points)
{
    points_ = points;
    updateControlPoints();
}

// to be used by split()
QList<QPointF> WeatherFront::firstSegment(int ctrlPointIndex) const
{
    Q_UNUSED(ctrlPointIndex);
    return QList<QPointF>();
}

// to be used by split()
QList<QPointF> WeatherFront::secondSegment(int ctrlPointIndex) const
{
    Q_UNUSED(ctrlPointIndex);
    return QList<QPointF>();
}

// Returns the distance between \a p and the multiline (i.e. the mimimum distance between \a p and any of the line segments).
// If the multiline contains fewer than two points, the function returns -1.
qreal WeatherFront::distance(const QPointF &p) const
{
    if (points_.size() < 2)
        return -1;

    qreal minDist = -1;
    for (int i = 1; i < points_.size(); ++i) {
        const qreal dist = distance2(QPointF(p), QPointF(points_.at(i - 1)), QPointF(points_.at(i)));
        minDist = (i == 1) ? dist : qMin(minDist, dist);
    }
    Q_ASSERT(minDist >= 0);
    return minDist;
}

void WeatherFront::draw(DrawModes modes, bool incomplete)
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
    glBegin(GL_LINE_STRIP);
    glColor3ub(color_.red(), color_.green(), color_.blue());
    foreach (QPointF p, points_)
        glVertex2i(p.x(), p.y());
    glEnd();

    // Draw the decorations on the line.
    switch (type) {
    case Cold:
        drawFront(Cold);
        break;
    case Warm:
        drawFront(Warm);
        break;
    case Occluded:
        drawFront(Occluded);
        break;
    case Stationary:
        drawFront(Stationary);
        break;
    default:
        ;
    }

    // draw control points if we're selected
    if (modes & Selected)
        drawControlPoints();

    // draw highlighting if we're hovered
    if (modes & Hovered)
        drawHoverHighlighting(incomplete);
}

/**
 * Draws the front specified by the given type.
 */
void WeatherFront::drawFront(frontType type)
{
  float r = 2;          // FIXME
  int end = s_length;
  int ncount = 0;

  float xstart,ystart,xend,yend,xtop,ytop,dxs = 0.0,dys = 0.0,fraction;
  float xstart1 = 0.0,ystart1 = 0.0,xend1 = 0.0,yend1 = 0.0;
  float s,slim,sprev = 0.0, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim = r * 0.75;
  s = 0.;
  i = 1;

  while (i < end) {

    while (s < slim && i < end) {
      sprev = s;
      dxs = x_s[i] - x_s[i-1];
      dys = y_s[i] - y_s[i-1];
      s += sqrtf(dxs*dxs + dys*dys);
      i++;
    }
    if (s < slim) break;

    i--;
    istart = i - 1;
    fraction = (slim-sprev)/(s-sprev);
    xstart = x_s[i-1] + dxs * fraction;
    ystart = y_s[i-1] + dys * fraction;
    s = 0.;
    slim = r*2.;

    while (s < slim && i < end) {
      dxs = x_s[i] - xstart;
      dys = y_s[i] - ystart;
      sprev = s;
      s = sqrtf(dxs*dxs + dys*dys);
      i++;
    }
    if (s < slim) break;

    i--;
    if (istart == i - 1) {
      fraction = slim/s;
      xend = xstart + dxs * fraction;
      yend = ystart + dys * fraction;
    } else {
      x1 = x_s[i-1];
      y1 = y_s[i-1];
      s1 = sqrtf((x1-xstart)*(x1-xstart) + (y1-ystart)*(y1-ystart));
      x2 = x_s[i];
      y2 = y_s[i];
      for (j = 0; j < 10; j++) {
        xm = (x1 + x2) * 0.5;
        ym = (y1 + y2) * 0.5;
        sm = sqrtf((xm-xstart)*(xm-xstart) + (ym-ystart)*(ym-ystart));
        if ((s1-slim) * (sm-slim) <= 0.) {
          x2 = xm;
          y2 = ym;
        } else {
          x1 = xm;
          y1 = ym;
          s1 = sm;
        }
      }
      xend = (x1 + x2) * 0.5;
      yend = (y1 + y2) * 0.5;
    }

    ndrawflag++;

    if (ndrawflag == 1 || type != Occluded) {
      xstart1 = xstart;
      ystart1 = ystart;
      xend1 =   xend;
      yend1 =   yend;
    }

    if (ndrawflag != 1 || type != Occluded) {

      dxs = xend1 - xstart1;
      dys = yend1 - ystart1;

      if (ncount % 2 == 0) {

        // Draw a warm symbol in warm and occluded fronts, and for every other symbol
        // in stationary fronts.
        if (type == Warm || type == Occluded || (type == Stationary && (ncount % 4 == 2))) {
          xm = (xstart1 + xend1) * 0.5;
          ym = (ystart1 + yend1) * 0.5;
          if (type == Stationary) glColor3f(1.,0.,0.);
          DRAW_WARM

        // Draw a cold symbol in cold fronts, and for every other symbol in stationary
        // fronts.
        } else if (type == Cold || (type == Stationary && (ncount % 4 == 0))) {
          xtop = (xstart1 + xend1) * 0.5;
          ytop = (ystart1 + yend1) * 0.5;
          if (type == Stationary) {
            xtop += dys*0.6;
            ytop -= dxs*0.6;
            glColor3f(0.,0.,1.);
          } else {
            xtop -= dys*0.6;
            ytop += dxs*0.6;
          }
          DRAW_COLD
        }

        // Always draw a cold symbol after a warm symbol in an occluded front.
        if (type == Occluded) {
          dxs= xend - xstart;
          dys= yend - ystart;
          xtop= (xstart + xend) * 0.5 - dys*0.6;
          ytop= (ystart + yend) * 0.5 + dxs*0.6;

          DRAW_COLD
        }
      }
      ncount++;

      ndrawflag=0;
    }

    dxs = xend - x_s[i-1];
    dys = yend - y_s[i-1];
    slim = sqrtf(dxs*dxs + dys*dys);
    s = 0.;
    
    if (ndrawflag == 0)
      slim += r*1.5;
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void WeatherFront::drawControlPoints()
{
    glColor3ub(0, 0, 0);
    foreach (QRectF c, controlPoints_) {
        glBegin(GL_POLYGON);
        glVertex3i(c.left(),  c.bottom(), 1);
        glVertex3i(c.right(), c.bottom(), 1);
        glVertex3i(c.right(), c.top(),    1);
        glVertex3i(c.left(),  c.top(),    1);
        glEnd();
    }
}

void WeatherFront::drawHoverHighlighting(bool incomplete)
{
    const int pad = 1;
    if (incomplete)
        glColor3ub(0, 200, 0);
    else
        glColor3ub(255, 0, 0);

    if (hoveredCtrlPointIndex_ >= 0) {
        // highlight a control point
        const QRectF *r = &controlPoints_.at(hoveredCtrlPointIndex_);
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
        glBegin(GL_LINE_STRIP);
        foreach (QPointF p, points_)
            glVertex3i(p.x(), p.y(), 1);
        glEnd();
        glPopAttrib();
    }
}

} // namespace EditItem_WeatherFront
