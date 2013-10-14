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
#include "diEditItemManager.h"

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

    // Returns true iff the item is hit at \a pos.
    // The item is considered selected iff \a selected is true (a selected item may typically be hit at
    // control points as well).
    virtual bool hit(const QPointF &pos, bool selected) const = 0;

    // Returns true iff the item is considered to be hit by \a rect.
    // Whether this means that the item's shape is partially or fully inside \a rect is
    // up to the item itself.
    virtual bool hit(const QRectF &bbox) const = 0;

    virtual void mousePress(
        QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
        QSet<EditItemBase *> *itemsToCopy = 0, QSet<EditItemBase *> *itemsToEdit = 0,
        QSet<EditItemBase *> *items = 0, const QSet<EditItemBase *> *selItems = 0, bool *multiItemOp = 0);

    virtual void incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);

    // Handles other events for an item in its normal state.
    // See mousePress() for the documentation of the arguments.
    virtual void mouseRelease(QMouseEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands);
    virtual void mouseMove(QMouseEvent *event, bool &repaintNeeded);
    virtual void mouseHover(QMouseEvent *event, bool &repaintNeeded);
    virtual void mouseDoubleClick(QMouseEvent *event, bool &repaintNeeded);
    virtual void keyPress(
        QKeyEvent *event, bool &repaintNeeded, QList<QUndoCommand *> *undoCommands,
        QSet<EditItemBase *> *items = 0, const QSet<EditItemBase *> *selItems = 0);
    virtual void keyRelease(QKeyEvent *event, bool &repaintNeeded);

    // Handles other events for an item in the process of being completed (i.e. during manual placement of a new item).
    // See incompleteMousePress() for the documentation of the arguments.
    virtual void incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
    virtual void incompleteMouseMove(QMouseEvent *event, bool &repaintNeeded);
    virtual void incompleteMouseHover(QMouseEvent *event, bool &repaintNeeded);
    virtual void incompleteMouseDoubleClick(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted) ;
    virtual void incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
    virtual void incompleteKeyRelease(QKeyEvent *event, bool &repaintNeeded);

    // Moves the item by the specified amount (i.e. \a pos is relative to the item's current position).
    virtual void moveBy(const QPointF &pos);

    // Draws the item.
    // \a modes indicates whether the item is selected, hovered, both, or neither.
    // \a incomplete is true iff the item is in the process of being completed (i.e. during manual placement of a new item).
    virtual void draw(DrawModes modes, bool incomplete) = 0;

    // Returns the item's globally unique ID.
    int id() const;

    // Returns the item's group ID if set, or -1 otherwise.
    int groupId() const;

    // Emits the repaintNeeded() signal.
    void repaint();

    // Returns a duplicate of the item.
    virtual EditItemBase *copy() const = 0;

    // Returns the item's points.
    virtual QList<QPointF> getPoints() const = 0;
    // Sets the item's points.
    virtual void setPoints(const QList<QPointF> &points) = 0;

    // Returns the item's geographic points.
    virtual QList<QPointF> getLatLonPoints() const;
    // Sets the item's geographic points.
    virtual void setLatLonPoints(const QList<QPointF> &points);

    // Returns the item's properties.
    QVariantMap properties() const;
    QVariantMap &propertiesRef();
    // sets the item's properties.
    void setProperties(const QVariantMap &);

    virtual QString infoString() const { return QString("addr=%1 id=%2").arg((ulong)this, 0, 16).arg(id()); }

    virtual QVariantMap clipboardVarMap() const;
    virtual QString clipboardPlainText() const;

    static EditItemBase *createItemFromVarMap(const QVariantMap &, QString *);

protected:
    EditItemBase();

    // Returns the item's base points (representing the start of a move operation etc.).
    virtual QList<QPointF> getBasePoints() const = 0;

    bool moving_;
    bool resizing_;
    QVariantMap properties_;

private:
    int id_;
    static int nextId_;
    int nextId();
    QList<QPointF> latLonPoints;

signals:
    void repaintNeeded();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EditItemBase::DrawModes)

#endif // EDITITEMBASE_H
