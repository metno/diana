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
#include "edititembase.h"
#include <diCommonTypes.h>

#define nwarmflag 19

namespace EditItem_WeatherArea {

class WeatherArea : public EditItemBase
{
    Q_OBJECT
    friend class SetGeometryCommand;
public:
    WeatherArea();
    virtual ~WeatherArea();

    EditItemBase *copy() const;

    void setType(frontType type);
    QList<QPoint> getPoints() const;
    void setPoints(const QList<QPoint> &points);

private:
    virtual bool hit(const QPoint &, bool) const;
    virtual bool hit(const QRect &) const;

    void init();

    virtual void mouseHover(QMouseEvent *, bool &);
    virtual void mousePress(QMouseEvent *, bool &, QList<QUndoCommand *> *, QSet<EditItemBase *> *, bool *);
    virtual void mouseMove(QMouseEvent *, bool &);
    virtual void mouseRelease(QMouseEvent *, bool &, QList<QUndoCommand *> *);
    virtual void keyPress(QKeyEvent *, bool &, QList<QUndoCommand *> *, QSet<EditItemBase *> *);

    virtual void incompleteMousePress(QMouseEvent *, bool &, bool &, bool &);
    virtual void incompleteMouseHover(QMouseEvent *, bool &);
    virtual void incompleteKeyPress(QKeyEvent *, bool &, bool &, bool &);

    virtual void draw(DrawModes, bool);
    void drawControlPoints();
    void drawHoverHighlighting(bool);

    int hitControlPoint(const QPoint &) const;
    void move(const QPoint &);
    void resize(const QPoint &);
    void updateControlPoints();

    void addPoint(bool &repaintNeeded, int index, const QPoint &point, QSet<EditItemBase *> *items);
    void remove(bool &, QSet<EditItemBase *> *);
    void removePoint(bool &repaintNeeded, int index, QSet<EditItemBase *> *items);

    QList<QPoint> geometry() const { return points_; }
    void setGeometry(const QList<QPoint> &);
    QList<QPoint> baseGeometry() const { return basePoints_; }
    qreal distance(const QPoint &) const;
    int hitLine(const QPoint &) const;
    int hitPoint(const QPoint &) const;

    QList<QPoint> points_;
    QList<QRect> controlPoints_;
    QList<QPoint> basePoints_;

    bool moving_;
    bool resizing_;
    QPoint baseMousePos_;
    int pressedCtrlPointIndex_;
    int hoveredCtrlPointIndex_;

    QPoint *placementPos_;

    QAction *addPoint_;
    QAction *remove_;
    QAction *removePoint_;

    QColor color_;
    frontType type;
    float *x,*y,*x_s,*y_s; // arrays for holding smooth line
    int s_length;          // nr of smooth line points
    float xwarmflag[nwarmflag];
    float ywarmflag[nwarmflag];
};

} // namespace EditItem_WeatherArea

#endif // WEATHERAREA_H
