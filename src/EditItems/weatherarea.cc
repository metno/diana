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
#include <QDomDocument>
#include <diTesselation.h>

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

WeatherArea::WeatherArea()
{
    init();
    updateControlPoints();
    color_.setRed(0);
    color_.setGreen(0);
    color_.setBlue(0);
}

WeatherArea::WeatherArea(const QVariantMap &vmap, QString *error)
{
  init();

  *error = QString();

  // verify type
  if (vmap.value("type").toString() != "EditItem_WeatherArea::WeatherArea") {
    *error = QString("invalid type: %1, expected %2")
        .arg(vmap.value("type").toString()).arg("EditItem_WeatherArea::WeatherArea");
    return;
  }

  // set any properties
  if (vmap.contains("properties")) {
    if (!vmap.value("properties").canConvert(QVariant::Map)) {
      *error = QString("invalid properties type: %1, expected %2")
          .arg(vmap.value("properties").typeName()).arg("QVariantMap");
      return;
    }
    setProperties(vmap.value("properties").toMap());
  }

  // set points
  if (!vmap.contains("points")) {
    *error = QString("no points found");
    return;
  }
  if (!vmap.value("points").canConvert(QVariant::List)) {
    *error = QString("invalid points type: %1, expected %2")
        .arg(vmap.value("points").typeName()).arg("QVariantList");
    return;
  }
  const QVariantList vpoints = vmap.value("points").toList();
  QList<QPointF> points;
  foreach (QVariant vpoint, vpoints)
    points.append(vpoint.toPointF());
  setPoints(EditItemManager::instance()->GeoToPhys(points));
}

WeatherArea::~WeatherArea()
{
}

void WeatherArea::init()
{
  moving_ = false;
  resizing_ = false;
  pressedCtrlPointIndex_ = -1;
  hoveredCtrlPointIndex_ = -1;
  placementPos_ = 0;
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
    setGeometry(points);
}

QList<QPoint> WeatherArea::baseGeometry() const
{
    return basePoints_;
}

QList<QPoint> WeatherArea::getBasePoints() const
{
    return baseGeometry();
}

bool WeatherArea::hit(const QPoint &pos, bool selected) const
{
    const qreal proximityTolerance = 3.0;
    const bool hitEdge = ((points_.size() >= 2) && (distance(pos) < proximityTolerance)) || (selected && (hitControlPoint(pos) >= 0));
    const QPolygon polygon(points_.toVector());
    const bool hitInterior = polygon.containsPoint(pos, Qt::OddEvenFill);
    return hitEdge || hitInterior;
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

void WeatherArea::mousePress(
    QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
    QSet<EditItemBase *> *itemsToCopy, QSet<EditItemBase *> *itemsToEdit,
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
            QAction addPoint_act(tr("&Add point"), 0);
            QAction remove_act(tr("&Remove"), 0);
            QAction removePoint_act(tr("Remove &point"), 0);
            QAction copyItems_act(tr("&Copy"), 0);
            QAction editItems_act(tr("P&roperties"), 0);

            // Add actions, checking for a click on a line or a point.
            const int lineIndex = hitLine(position);
            const int pointIndex = hitControlPoint(position);
            if (pointIndex != -1) {
                if (points_.size() <= 3)
                    return; // an area needs at least three points
                contextMenu.addAction(&removePoint_act);
            } else if (lineIndex != -1) {
                contextMenu.addAction(&addPoint_act);
            }
            contextMenu.addAction(&remove_act);
            if (itemsToCopy)
              contextMenu.addAction(&copyItems_act);
            if (itemsToEdit)
              contextMenu.addAction(&editItems_act);
            QAction *action = contextMenu.exec(event->globalPos(), &remove_act);
            if (action == &remove_act)
                remove(repaintNeeded, items);
            else if (action == &removePoint_act)
                removePoint(repaintNeeded, pointIndex, items);
            else if (action == &addPoint_act)
                addPoint(repaintNeeded, lineIndex, position, items);
            else if (action == &copyItems_act) {
                Q_ASSERT(itemsToCopy);
                QSet<EditItemBase *>::const_iterator it;
                for (it = items->begin(); it != items->end(); ++it) {
                    WeatherArea *weatherArea = qobject_cast<WeatherArea *>(*it);
                    if (weatherArea)
                        itemsToCopy->insert(weatherArea);
                }
            } else if (action == &editItems_act) {
                Q_ASSERT(itemsToEdit);
                //Q_ASSERT(items->contains(this));
                itemsToEdit->insert(this);
            }
        }
    }
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

void WeatherArea::moveBy(const QPoint &pos)
{
    baseMousePos_ = QPoint();
    basePoints_ = points_;
    move(pos);
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

    // draw the interior
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
    GLdouble *gldata = new GLdouble[points_.size() * 3];
    for (int i = 0; i < points_.size(); ++i) {
        const QPoint p = points_.at(i);
        gldata[3 * i] = p.x();
        gldata[3 * i + 1] = p.y();
        gldata[3 * i + 2] = 0.0;
    }
    glColor4ub(128, 128, 128, 50);
    beginTesselation();
    int npoints = points_.size();
    tesselation(gldata, 1, &npoints);
    endTesselation();
    delete[] gldata;

    // draw the outline
    glBegin(GL_LINE_LOOP);
    glColor3ub(color_.red(), color_.green(), color_.blue());
    foreach (QPoint p, points_)
        glVertex2i(p.x(), p.y());
    glEnd();

    // draw highlighting if hovered
    if (modes & Hovered)
        drawHoverHighlighting(incomplete);

    // draw control points if selected
    if (modes & Selected)
        drawControlPoints();
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

// Attempts to create new WeatherArea objects from the KML structure in \a data.
// If successful, the function returns a list of pointers to the new objects.
// Otherwise, the function returns an empty list and passes an explanation in \a error.
QList<WeatherArea *> WeatherArea::createFromKML(const QByteArray &data, QString *error)
{
  *error = QString();

  int line;
  int col;
  QString err;
  QDomDocument doc;
  if (doc.setContent(data, &err, &line, &col) == false) {
    *error = QString("parse error at line=%1, column=%2: %3").arg(line).arg(col).arg(err);
    return QList<WeatherArea *>();
  }

  const QDomNodeList folderNodes = doc.elementsByTagName("Folder");
  if (folderNodes.size() == 0) {
    *error = QString("no Folder elements found");
    return QList<WeatherArea *>();
  }

  QList<WeatherArea *> areas;

  // loop over folders
  for (int i = 0; i < folderNodes.size(); ++i) {
    const QDomElement folderElement = folderNodes.item(i).toElement();

    // folder name
    const QDomElement folderNameElement = folderElement.firstChildElement("name");
    if (folderNameElement.isNull()) {
      *error = QString("no folder name element found");
      break;
    }
    const QString folderName = folderNameElement.firstChild().nodeValue();

    // placemarks
    const QDomNodeList placemarkNodes = folderElement.elementsByTagName("Placemark");
    if (placemarkNodes.size() == 0) {
      *error = QString("no Placemark elements found in Folder element %1").arg(folderName);
      break;
    }

    // loop over placemarks
    for (int j = 0; j < placemarkNodes.size(); ++j) {
      const QDomElement placemarkElement = placemarkNodes.item(j).toElement();

      // placemark name
      const QDomElement placemarkNameElement = placemarkElement.firstChildElement("name");
      if (placemarkNameElement.isNull()) {
        *error = QString("no name element found in Placemark element %1 in Folder element %2")
            .arg(j).arg(folderName);
        break;
      }
      const QString placemarkName = placemarkNameElement.firstChild().nodeValue();

      // placemark coordinates
      const QDomNodeList coordinatesNodes = placemarkElement.elementsByTagName("coordinates");
      if (coordinatesNodes.size() != 1) {
        *error = QString("exactly one coordinates element expected in Placemark element %1 in Folder element %2, found %3")
            .arg(placemarkName).arg(folderName).arg(coordinatesNodes.size());
        break;
      }
      if (coordinatesNodes.item(0).childNodes().size() != 1) {
        *error = QString("one child expected in coordinates element in Placemark element %1 in Folder element %2, found %3")
            .arg(placemarkName).arg(folderName).arg(coordinatesNodes.item(0).childNodes().size());
        break;
      }
      const QString coords = coordinatesNodes.item(0).firstChild().nodeValue();
      QList<QPointF> points;
      foreach (QString coord, coords.split(QRegExp("\\s+"), QString::SkipEmptyParts)) {
        const QStringList coordComps = coord.split(",", QString::SkipEmptyParts);
        if (coordComps.size() < 2) {
          *error = QString("expected at least two components (i.e. lat, lon) in coordinate in Placemark element %1 in Folder element %2, found %3: %4")
              .arg(placemarkName).arg(folderName).arg(coordComps.size()).arg(coord);
          break;
        }
        bool ok;
        const double lon = coordComps.at(1).toDouble(&ok);
        if (!ok) {
          *error = QString("failed to convert longitude string to double value in Placemark element %1 in Folder element %2: %3")
              .arg(placemarkName).arg(folderName).arg(coordComps.at(1));
          break;
        }
        const double lat = coordComps.at(0).toDouble(&ok);
        if (!ok) {
          *error = QString("failed to convert latitude string to double value in Placemark element %1 in Folder element %2: %3")
              .arg(placemarkName).arg(folderName).arg(coordComps.at(0));
          break;
        }
        points.append(QPointF(lon, lat));
      }

      if (!error->isEmpty())
        break;

      WeatherArea *area = new WeatherArea();
      area->setLatLonPoints(points);
      area->propertiesRef().insert("origKML", data);
      area->propertiesRef().insert("folderName", folderName);
      area->propertiesRef().insert("placemarkName", placemarkName);
      areas.append(area);

    } // placemarks

    if (!error->isEmpty())
      break;

  } // folders

  if (!error->isEmpty()) {
    foreach (WeatherArea *area, areas)
      delete area;
    areas.clear();
  }

  return areas;
}

QVariantMap WeatherArea::clipboardVarMap() const
{
  QVariantMap vmap = EditItemBase::clipboardVarMap();
  QVariantList vpoints;
  foreach (QPointF p, EditItemManager::instance()->PhysToGeo(getPoints()))
    vpoints.append(p);
  vmap.insert("points", vpoints);
  vmap.insert("properties", properties());
  return vmap;
}

QString WeatherArea::clipboardPlainText() const
{
  QString s = EditItemBase::clipboardPlainText();
  foreach (QPointF p, EditItemManager::instance()->PhysToGeo(getPoints()))
    s.append(QString("(%1, %2) ").arg(p.x()).arg(p.y()));
  return s;
}

} // namespace EditItem_WeatherArea
