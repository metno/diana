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

#ifndef WEATHERAREA_H
#define WEATHERAREA_H

#include <QtGui>
#include "drawingweatherarea.h"
#include "edititembase.h"
#include <diCommonTypes.h>

#define nwarmflag 19

namespace EditItem_WeatherArea {

class WeatherArea : public EditItemBase, public DrawingItem_WeatherArea::WeatherArea
{
    Q_OBJECT
    friend class SetGeometryCommand;
public:
    WeatherArea();
    WeatherArea(const QVariantMap &, QString *);
    virtual ~WeatherArea();

    EditItemBase *copy() const;
    
    // Sets the points for the area and updates any control points.
    void setPoints(const QList<QPointF> &points);

    void draw(DrawModes, bool);

private:
    virtual bool hit(const QPointF &, bool) const;
    virtual bool hit(const QRectF &) const;

    void init();

    virtual void mouseHover(QMouseEvent *, bool &);
    virtual void mousePress(
        QMouseEvent *, bool &, QList<QUndoCommand *> *, QSet<DrawingItemBase *> *, QSet<DrawingItemBase *> *,
        QSet<DrawingItemBase *> *, const QSet<DrawingItemBase *> *, bool *);
    virtual void mouseMove(QMouseEvent *, bool &);

    virtual void incompleteMousePress(QMouseEvent *, bool &, bool &, bool &);
    virtual void incompleteMouseHover(QMouseEvent *, bool &);
    virtual void incompleteKeyPress(QKeyEvent *, bool &, bool &, bool &);

    virtual void moveBy(const QPointF &);

    virtual QString infoString() const { return QString("%1 type=%2 npoints=%3").arg(DrawingItemBase::infoString()).arg(metaObject()->className()).arg(points_.size()); }

    bool saveAsSimpleAreas(QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems, QString *error);
    bool saveAsVAACGroup(QSet<DrawingItemBase *> *items, QString *error);

    virtual QVariantMap clipboardVarMap() const;
    virtual QString clipboardPlainText() const;

    void drawControlPoints();
    void drawHoverHighlighting(bool);

    int hitControlPoint(const QPointF &) const;
    void move(const QPointF &);
    void resize(const QPointF &);
    void updateControlPoints();

    void addPoint(bool &repaintNeeded, int index, const QPointF &point);
    void remove(bool &repaintNeeded, QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems);
    void removePoint(bool &repaintNeeded, int index, QSet<DrawingItemBase *> *items, const QSet<DrawingItemBase *> *selItems);

    QList<QPointF> geometry() const { return points_; }
    void setGeometry(const QList<QPointF> &);
    virtual QList<QPointF> baseGeometry() const;
    virtual QList<QPointF> getBasePoints() const;
    qreal distance(const QPointF &) const;
    int hitLine(const QPointF &) const;

    QList<QRectF> controlPoints_;
    QList<QPointF> basePoints_;

    QPointF baseMousePos_;
    int pressedCtrlPointIndex_;
    int hoveredCtrlPointIndex_;

    QPointF *placementPos_;

    QAction *addPoint_;
    QAction *remove_;
    QAction *removePoint_;
    QAction *copyItems_;
    QAction *editItems_;
};

} // namespace EditItem_WeatherArea

#endif // WEATHERAREA_H
