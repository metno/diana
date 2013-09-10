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

#ifndef EDITITEMMANAGER_H
#define EDITITEMMANAGER_H

#include <QObject>
#include <QPoint>
#include <QSet>
#include <QUndoCommand>
#include <QUndoView>

class AddOrRemoveItemsCommand;
class DrawingManager;
class EditItemBase;
class QKeyEvent;
class QMouseEvent;
class QUndoStack;

class EditItemManager : public QObject
{
    Q_OBJECT
    friend class AddOrRemoveItemsCommand;

public:
    EditItemManager(DrawingManager *drawm);
    virtual ~EditItemManager();

    // Registers a new item with the manager.
    // \a incomplete is true iff the item is considered in the process of being completed (i.e. during manual placement of a new item).
    void addItem(EditItemBase *item, bool incomplete = false);

    // Returns the undo stack.
    QUndoStack *undoStack();

    bool canRedo() const;
    bool canUndo() const;
    bool hasIncompleteItem() const;
    bool needsRepaint() const;

    QSet<EditItemBase *> getItems() const;
    QSet<EditItemBase *> getSelectedItems() const;
    QSet<EditItemBase *> findHitItems(const QPoint &) const;

    void createUndoView();

public slots:
    void abortEditing();
    void completeEditing();
    void copyObjects();
    void draw();
    void keyPress(QKeyEvent *);
    void keyRelease(QKeyEvent *);
    void mouseDoubleClick(QMouseEvent *);
    void mouseMove(QMouseEvent *);
    void mousePress(QMouseEvent *, QSet<EditItemBase *> * = 0);
    void mouseRelease(QMouseEvent *);
    void pasteObjects();
    void redo();
    void repaint();
    void reset();
    void undo();

signals:
    void paintDone();
    void repaintNeeded();
    void canUndoChanged(bool);
    void canRedoChanged(bool);
    void incompleteEditing(bool);

private:
    QSet<EditItemBase *> items_;
    QSet<EditItemBase *> selItems_;
    QSet<EditItemBase *> copiedItems_;
    EditItemBase *hoverItem_;
    EditItemBase *incompleteItem_; // item in the process of being completed (e.g. having its control points manually placed)
    bool repaintNeeded_;
    bool skipRepaint_;
    QUndoStack undoStack_;
    DrawingManager *drawingManager_;

    void addItem_(EditItemBase *);
    void retrieveItems(const QSet<EditItemBase *> &);
    void incompleteMousePress(QMouseEvent *);
    void incompleteMouseRelease(QMouseEvent *);
    void incompleteMouseMove(QMouseEvent *);
    void incompleteMouseDoubleClick(QMouseEvent *);
    void incompleteKeyPress(QKeyEvent *);
    void incompleteKeyRelease(QKeyEvent *);
    void pushCommands(QSet<EditItemBase *> addedItems,
                      QSet<EditItemBase *> removedItems,
                      QList<QUndoCommand *> undoCommands);
    void removeItem(EditItemBase *item);
    void storeItems(const QSet<EditItemBase *> &);
};

class AddOrRemoveItemsCommand : public QUndoCommand
{
public:
    AddOrRemoveItemsCommand(EditItemManager *, const QSet<EditItemBase *> &, const QSet<EditItemBase *> &);
    virtual ~AddOrRemoveItemsCommand() {}

private:
    EditItemManager *eim_;
    QSet<EditItemBase *> addedItems_;
    QSet<EditItemBase *> removedItems_;
    virtual void undo();
    virtual void redo();
};

class SetGeometryCommand : public QUndoCommand
{
public:
    SetGeometryCommand(EditItemBase *, const QList<QPoint> &, const QList<QPoint> &);
private:
    EditItemBase *item_;
    QList<QPoint> oldGeometry_;
    QList<QPoint> newGeometry_;
    virtual void undo();
    virtual void redo();
};

#endif // EDITITEMMANAGER_H
