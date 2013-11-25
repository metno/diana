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
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QUndoCommand>
#include <QUndoView>
#include <QLineEdit>
#include "diDrawingManager.h"

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
  QWidget *formWidget_;
};

class EditItemManager : public DrawingManager
{
    Q_OBJECT

public:
    enum Action {
      Cut, Copy, Paste, Edit, Load, Save, Undo, Redo, Select, CreatePolyLine, CreateSymbol
    };

    EditItemManager();
    virtual ~EditItemManager();

    // Registers a new item with the manager.
    // \a incomplete is true iff the item is considered in the process of being completed (i.e. during manual placement of a new item).
    void addItem(DrawingItemBase *item, bool incomplete = false, bool skipRepaint = false);
    void addItem(EditItemBase *item, bool incomplete = false, bool skipRepaint = false);
    void removeItem(DrawingItemBase *item);

    virtual DrawingItemBase *createItemFromVarMap(const QVariantMap &vmap, QString *error);

    // Returns the undo stack.
    QUndoStack *undoStack();

    bool canRedo() const;
    bool canUndo() const;
    bool hasIncompleteItem() const;
    bool needsRepaint() const;

    QSet<DrawingItemBase *> getSelectedItems() const;
    QSet<DrawingItemBase *> findHitItems(const QPointF &) const;

    bool changeProjection(const Area& newArea);
    void plot(bool under, bool over);
    void storeItems(const QSet<DrawingItemBase *> &);
    void retrieveItems(const QSet<DrawingItemBase *> &);

    static EditItemManager *instance();
    virtual bool isEnabled() const;

    void sendMouseEvent(QMouseEvent* event, EventResult& res);
    void sendKeyboardEvent(QKeyEvent* event, EventResult& res);

    void editItemProperties(const QSet<DrawingItemBase *> &);

    QHash<Action, QAction*> actions();
    QUndoView *getUndoView();

public slots:
    void abortEditing();
    void completeEditing();
    void copySelectedItems();
    void cutSelectedItems();
    void deselectItem(DrawingItemBase *);
    void editItems();
    void keyPress(QKeyEvent *);
    void keyRelease(QKeyEvent *);
    void mouseDoubleClick(QMouseEvent *);
    void mouseMove(QMouseEvent *);
    void mousePress(QMouseEvent *, QSet<DrawingItemBase *> * = 0, QSet<DrawingItemBase *> * = 0);
    void mouseRelease(QMouseEvent *);
    void pasteItems();
    void redo();
    void repaint();
    void reset();
    void selectItem(DrawingItemBase *);
    void undo();
    void updateActions();
    void saveItemsToFile();
    void setSelectMode();

private slots:
    void loadItemsFromFile();
    void setCreatePolyLineMode();
    void setCreateSymbolMode();

signals:
    void selectionChanged();
    void paintDone();
    void repaintNeeded();
    void canUndoChanged(bool);
    void canRedoChanged(bool);
    void incompleteEditing(bool);
    void itemAdded(DrawingItemBase *);
    void itemChanged(DrawingItemBase *);
    void itemRemoved(DrawingItemBase *);
    void timesUpdated();

protected:
    virtual void addItem_(DrawingItemBase *);
    virtual void removeItem_(DrawingItemBase *item);

private slots:
    void initNewItem(DrawingItemBase *item);

private:
    QSet<DrawingItemBase *> selItems_;
    EditItemBase *hoverItem_;
    EditItemBase *incompleteItem_; // item in the process of being completed (e.g. having its control points manually placed)
    bool repaintNeeded_;
    bool skipRepaint_;
    QUndoStack undoStack_;
    QUndoView *undoView_;

    QAction* cutAction;
    QAction* copyAction;
    QAction* pasteAction;
    QAction* editAction;
    QAction* loadAction;
    QAction* saveAction;
    QAction* undoAction;
    QAction* redoAction;
    QAction* selectAction;
    QAction* createPolyLineAction;
    QAction* createSymbolAction;

    enum Mode {
      SelectMode, CreatePolyLineMode, CreateSymbolMode
    } mode_;

    void incompleteMousePress(QMouseEvent *);
    void incompleteMouseRelease(QMouseEvent *);
    void incompleteMouseMove(QMouseEvent *);
    void incompleteMouseDoubleClick(QMouseEvent *);
    void incompleteKeyPress(QKeyEvent *);
    void incompleteKeyRelease(QKeyEvent *);
    void pushCommands(QSet<DrawingItemBase *> addedItems,
                      QSet<DrawingItemBase *> removedItems,
                      QList<QUndoCommand *> undoCommands);

    // Clipboard operations
    void copyItems(const QSet<DrawingItemBase *> &);

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
    AddOrRemoveItemsCommand(const QSet<DrawingItemBase *> &, const QSet<DrawingItemBase *> &);
    virtual ~AddOrRemoveItemsCommand() {}

private:
    QSet<DrawingItemBase *> addedItems_;
    QSet<DrawingItemBase *> removedItems_;
    virtual void undo();
    virtual void redo();
};

class SetGeometryCommand : public EditItemCommand
{
public:
    SetGeometryCommand(EditItemBase *, const QList<QPointF> &, const QList<QPointF> &);
    virtual ~SetGeometryCommand() {}
    virtual int id() const;
    virtual bool mergeWith(const QUndoCommand *command);

    EditItemBase *item() const;
    QList<QPointF> newLatLonPoints() const;

private:
    EditItemBase *item_;
    QList<QPointF> oldLatLonPoints_;
    QList<QPointF> newLatLonPoints_;
    virtual void undo();
    virtual void redo();
};

#endif // EDITITEMMANAGER_H
