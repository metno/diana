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

#ifndef WEATHERFRONT_H
#define WEATHERFRONT_H

#include <QtGui>
#include "edititembase.h"
#include <diCommonTypes.h>

#define nwarmflag 19

namespace EditItem_WeatherFront {

class WeatherFront : public EditItemBase
{
    Q_OBJECT
    friend class SetGeometryCommand;
public:
    WeatherFront();
    virtual ~WeatherFront();

    EditItemBase *copy() const;

    void setType(frontType type);
    QList<QPointF> getPoints() const;
    void setPoints(const QList<QPointF> &points);

private:
    virtual bool hit(const QPointF &, bool) const;
    virtual bool hit(const QRectF &) const;

    void init();

    virtual void mouseHover(QMouseEvent *, bool &);
    virtual void mousePress(QMouseEvent *, bool &, QList<QUndoCommand *> *, QSet<EditItemBase *> *, bool *);
    virtual void mouseMove(QMouseEvent *, bool &);

    virtual void incompleteMousePress(QMouseEvent *, bool &, bool &, bool &);
    virtual void incompleteMouseHover(QMouseEvent *, bool &);
    virtual void incompleteKeyPress(QKeyEvent *, bool &, bool &, bool &);

    virtual void moveBy(const QPointF &);

    virtual QString infoString() const { return QString("%1 type=WeatherFront npoints=%2").arg(EditItemBase::infoString()).arg(points_.size()); }

    virtual void draw(DrawModes, bool);
    void drawFront(frontType type);
    void drawControlPoints();
    void drawHoverHighlighting(bool);

    int hitControlPoint(const QPointF &) const;
    void move(const QPointF &);
    void resize(const QPointF &);
    void updateControlPoints();
    void remove(bool &, QSet<EditItemBase *> *);
    void split(const QPointF &, bool &, QList<QUndoCommand *> *, QSet<EditItemBase *> *);
    void merge(const QPointF &, bool &, QList<QUndoCommand *> *, QSet<EditItemBase *> *);
    virtual QList<QPointF> geometry() const { return points_; }
    void setGeometry(const QList<QPointF> &);
    virtual QList<QPointF> baseGeometry() const;
    virtual QList<QPointF> getBasePoints() const;
    QList<QPointF> firstSegment(int) const; // the arg is a control point index
    QList<QPointF> secondSegment(int) const; // ditto
    qreal distance(const QPointF &) const;
    int hitLine(const QPointF &) const;
    int hitPoint(const QPointF &) const;

    QList<QPointF> points_;
    QList<QRectF> controlPoints_;
    QList<QPointF> basePoints_;

    QPointF baseMousePos_;
    int pressedCtrlPointIndex_;
    int hoveredCtrlPointIndex_;

    QPointF *placementPos_;

    QAction *remove_;
    QAction *split_;
    QAction *merge_;
    QMenu *contextMenu_;

    QColor color_;
    frontType type;
    float *x,*y,*x_s,*y_s; // arrays for holding smooth line
    int s_length;          // nr of smooth line points
    float xwarmflag[nwarmflag];
    float ywarmflag[nwarmflag];
};

} // namespace EditItem_WeatherFront

#endif // WEATHERFRONT_H
