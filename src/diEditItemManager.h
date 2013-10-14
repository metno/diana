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

#include <QDialog>
#include <QGridLayout>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QUndoCommand>
#include <QUndoView>
#include <QLineEdit>

class AddOrRemoveItemsCommand;
class DrawingManager;
class EditItemBase;
class QKeyEvent;
class QMouseEvent;
class QTextEdit;
class QUndoStack;

class TextEditor : public QDialog
{
public:
  TextEditor(const QString &text);
  virtual ~TextEditor();

  QString text() const;

private:
  QTextEdit *textEdit_;
};

// ### move this class declaration to a private header file?
class SpecialLineEdit : public QLineEdit
{
  Q_OBJECT
public:
  SpecialLineEdit(const QString &);
private:
  QString propertyName_;
  QString propertyName() const;
  void contextMenuEvent(QContextMenuEvent *);
private slots:
  void openTextEdit();
};

class VarMapEditor : public QDialog
{
public:
  static VarMapEditor *instance();
  QVariantMap edit(const QVariantMap &values);

private:
  VarMapEditor();

  static VarMapEditor *instance_;
  QGridLayout *glayout_;
};

class EditItemManager : public QObject
{
    Q_OBJECT

public:
    EditItemManager();
    virtual ~EditItemManager();

    // Registers a new item with the manager.
    // \a incomplete is true iff the item is considered in the process of being completed (i.e. during manual placement of a new item).
    void addItem(EditItemBase *item, bool incomplete = false);
    void removeItem(EditItemBase *item);

    // Returns the undo stack.
    QUndoStack *undoStack();

    bool canRedo() const;
    bool canUndo() const;
    bool hasIncompleteItem() const;
    bool needsRepaint() const;

    QSet<EditItemBase *> getItems() const;
    QSet<EditItemBase *> getSelectedItems() const;
    QSet<EditItemBase *> findHitItems(const QPointF &) const;

    void storeItems(const QSet<EditItemBase *> &);
    void retrieveItems(const QSet<EditItemBase *> &);
    QList<QPointF> PhysToGeo(const QList<QPointF> &points);
    QList<QPointF> GeoToPhys(const QList<QPointF> &points);

    static EditItemManager *instance() { return self; }

    void editItemProperties(const QSet<EditItemBase *> &);

    QUndoView *getUndoView();

public slots:
    void abortEditing();
    void completeEditing();
    void copyObjects();
    void deselectItem(EditItemBase *);
    void draw();
    void keyPress(QKeyEvent *);
    void keyRelease(QKeyEvent *);
    void mouseDoubleClick(QMouseEvent *);
    void mouseMove(QMouseEvent *);
    void mousePress(QMouseEvent *, QSet<EditItemBase *> * = 0, QSet<EditItemBase *> * = 0);
    void mouseRelease(QMouseEvent *);
    void pasteObjects();
    void redo();
    void repaint();
    void reset();
    void selectItem(EditItemBase *);
    void undo();

signals:
    void selectionChanged();
    void paintDone();
    void repaintNeeded();
    void canUndoChanged(bool);
    void canRedoChanged(bool);
    void incompleteEditing(bool);
    void itemAdded(EditItemBase *);
    void itemChanged(EditItemBase *);
    void itemRemoved(EditItemBase *);

private:
    QSet<EditItemBase *> items_;
    QSet<EditItemBase *> selItems_;
    QSet<EditItemBase *> copiedItems_;
    EditItemBase *hoverItem_;
    EditItemBase *incompleteItem_; // item in the process of being completed (e.g. having its control points manually placed)
    bool repaintNeeded_;
    bool skipRepaint_;
    QUndoStack undoStack_;
    QUndoView *undoView_;
    DrawingManager *drawingManager_;

    void addItem_(EditItemBase *);
    void incompleteMousePress(QMouseEvent *);
    void incompleteMouseRelease(QMouseEvent *);
    void incompleteMouseMove(QMouseEvent *);
    void incompleteMouseDoubleClick(QMouseEvent *);
    void incompleteKeyPress(QKeyEvent *);
    void incompleteKeyRelease(QKeyEvent *);
    void pushCommands(QSet<EditItemBase *> addedItems,
                      QSet<EditItemBase *> removedItems,
                      QList<QUndoCommand *> undoCommands);
    void removeItem_(EditItemBase *item);

    static EditItemManager *self;   // singleton instance pointer
};

class EditItemCommand : public QUndoCommand
{
public:
    EditItemCommand(const QString &text, QUndoCommand *parent = 0);
    EditItemCommand() {}
    virtual ~EditItemCommand() {}
};

class AddOrRemoveItemsCommand : public EditItemCommand
{
public:
    AddOrRemoveItemsCommand(const QSet<EditItemBase *> &, const QSet<EditItemBase *> &);
    virtual ~AddOrRemoveItemsCommand() {}

private:
    QSet<EditItemBase *> addedItems_;
    QSet<EditItemBase *> removedItems_;
    virtual void undo();
    virtual void redo();
};

class SetGeometryCommand : public EditItemCommand
{
public:
    SetGeometryCommand(EditItemBase *, const QList<QPointF> &, const QList<QPointF> &);
    virtual ~SetGeometryCommand() {}

private:
    EditItemBase *item_;
    QList<QPointF> oldGeometry_;
    QList<QPointF> newGeometry_;
    QList<QPointF> oldLatLonPoints_;
    QList<QPointF> newLatLonPoints_;
    virtual void undo();
    virtual void redo();
};

#endif // EDITITEMMANAGER_H
