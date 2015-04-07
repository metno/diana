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

#ifndef EDITITEMBASE_H
#define EDITITEMBASE_H

#include <QtGui>
#include "drawingitembase.h"
#include "diEditItemManager.h"

#define Editing(i) dynamic_cast<EditItemBase *>(i)

// This is the abstract base class for editable items.
class EditItemBase : public QObject
{
    Q_OBJECT

public:
    virtual ~EditItemBase() {}
    EditItemBase(const EditItemBase &other) {}

    enum DrawMode {
        Normal = 0x0,   // the item is neither selected nor hovered
        Selected = 0x1, // the item is selected
        Hovered = 0x2   // the item is hovered
    };
    Q_DECLARE_FLAGS(DrawModes, DrawMode)

    virtual void mousePress(QMouseEvent *event, bool &repaintNeeded, bool *multiItemOp = 0);

    virtual void incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);


    // Handles other events for an item in its normal state.
    // See mousePress() for the documentation of the arguments.
    virtual void mouseRelease(QMouseEvent *event, bool &repaintNeeded);
    virtual void mouseMove(QMouseEvent *event, bool &repaintNeeded);
    virtual void mouseHover(QMouseEvent *event, bool &repaintNeeded, bool selectingOnly = false);
    virtual void mouseDoubleClick(QMouseEvent *event, bool &repaintNeeded);
    virtual void keyPress(QKeyEvent *event, bool &repaintNeeded);
    virtual void keyRelease(QKeyEvent *event, bool &repaintNeeded);


    // Handles other events for an item in the process of being completed (i.e. during manual placement of a new item).
    // See incompleteMousePress() for the documentation of the arguments.
    virtual void incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
    virtual void incompleteMouseMove(QMouseEvent *event, bool &repaintNeeded);
    virtual void incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded);
    virtual void incompleteMouseDoubleClick(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted) ;
    virtual void incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
    virtual void incompleteKeyRelease(QKeyEvent *event, bool &repaintNeeded);

    void draw(DrawModes, bool, bool = false);
    void draw();

    QVariantMap clipboardVarMap() const;
    QString clipboardPlainText() const;

    // Returns actions that are applicable to this item at screen position \a pos. Examples include adding or deleting control points.
    virtual QList<QAction *> actions(const QPoint &pos) const;

    virtual void updateHoverPos(const QPoint &);
    QPointF hoverPos() const;
    virtual void movePointTo(int, const QPointF &);

protected:
    EditItemBase();

    void init();
    void copyBaseData(EditItemBase *) const;

    QList<QPointF> geometry() const;
    void setGeometry(const QList<QPointF> &);
    virtual QList<QPointF> baseGeometry() const;

    // Returns the item's base points (representing the start of a move operation etc.).
    virtual QList<QPointF> getBasePoints() const;

    int hitControlPoint(const QPointF &) const;
    void moveBy(const QPointF &);
    virtual void move(const QPointF &);
    virtual void resize(const QPointF &) = 0;
    virtual void updateControlPoints() = 0;

    // Draws graphics to indicate the incomplete state of the item (if applicable).
    virtual void drawIncomplete() const {}

    virtual void drawHoverHighlightingBG(bool, bool) const {};
    virtual void drawHoverHighlighting(bool, bool) const = 0;

    void drawControlPoints(const QColor & = QColor(0, 0, 0, 255), const QColor & = QColor(0, 200, 200, 255), int = 0) const;
    void drawHoveredControlPoint(const QColor & = QColor(0, 0, 0, 255), int = 0) const;

    bool moving_;
    bool resizing_;

    QList<QRectF> controlPoints_;
    QList<QPointF> basePoints_;

    QPointF baseMousePos_;
    int pressedCtrlPointIndex_;
    int hoverCtrlPointIndex_;
    QPoint hoverPos_;

    static int controlPointSize() { return 8; }

private:
    void nudge(QKeyEvent *, bool &);
    void remove(QKeyEvent *);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EditItemBase::DrawModes)

#endif // EDITITEMBASE_H
