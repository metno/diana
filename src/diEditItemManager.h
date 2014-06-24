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

class EditItemManager : public DrawingManager
{
  Q_OBJECT

public:
  enum Action {
    Cut, Copy, Paste, EditProperties, EditStyle, Undo, Redo, Select,
    CreatePolyLine, CreateSymbol, CreateText, CreateComposite
  };

  static EditItemManager *instance();

  EditItemManager();
  virtual ~EditItemManager();

  /// Registers a new item with the manager.
  /// \a incomplete is true iff the item is considered in the process of being completed (i.e. during manual placement of a new item).
  void addItem(const QSharedPointer<DrawingItemBase> &item, bool incomplete = false, bool skipRepaint = false);
  void editItem(const QSharedPointer<DrawingItemBase> &item);
  void editItem(DrawingItemBase *item);
  void removeItem(const QSharedPointer<DrawingItemBase> &item);

  virtual QSharedPointer<DrawingItemBase> createItemFromVarMap(const QVariantMap &vmap, QString *error);

  // Returns the undo stack.
  QUndoStack *undoStack();

  bool canRedo() const;
  bool canUndo() const;
  bool hasIncompleteItem() const;
  bool needsRepaint() const;

  QSet<QSharedPointer<DrawingItemBase> > getSelectedItems() const;
  QSet<QSharedPointer<DrawingItemBase> > findHitItems(const QPointF &) const;

  bool processInput(const std::vector<std::string>& inp);
  void plot(bool under, bool over);
  void storeItems(const QSet<QSharedPointer<DrawingItemBase> > &);
  void retrieveItems(const QSet<QSharedPointer<DrawingItemBase> > &);

  //virtual bool isEnabled() const;
  virtual void setEditing(bool enable);

  void sendMouseEvent(QMouseEvent* event, EventResult& res);
  void sendKeyboardEvent(QKeyEvent* event, EventResult& res);

  QHash<Action, QAction*> actions();
  QUndoView *getUndoView();

public slots:
  void abortEditing();
  void completeEditing();
  void copySelectedItems();
  void cutSelectedItems();
  void deselectItem(const QSharedPointer<DrawingItemBase> &);
  void deselectAllItems();
  void editProperties();
  void editStyle();
  void setStyleType() const;
  void keyPress(QKeyEvent *);
  void keyRelease(QKeyEvent *);
  void mouseDoubleClick(QMouseEvent *);
  void mouseMove(QMouseEvent *);
  void mousePress(QMouseEvent *);
  void mouseRelease(QMouseEvent *);
  void pasteItems();
  void redo();
  void repaint();
  void reset();
  void selectItem(const QSharedPointer<DrawingItemBase> &);
  void setSelectMode();
  void startStopEditing(bool start);
  void undo();

  void handleLayersUpdate();

private slots:
  void setCreatePolyLineMode();
  void setCreateSymbolMode();
  void setCreateTextMode();
  void setCreateCompositeMode();
  void handleSelectionChange();

signals:
  void selectionChanged();
  void repaintNeeded();
  void canUndoChanged(bool);
  void canRedoChanged(bool);
  void incompleteEditing(bool);
  void itemAdded(DrawingItemBase *);
  void itemChanged(DrawingItemBase *); // ### anybody connected to this?
  void itemRemoved(DrawingItemBase *); // ### anybody connected to this?
  void timesUpdated();
  void setWorkAreaCursor(const QCursor &);
  void unsetWorkAreaCursor();
  void editing(bool);

protected:
  virtual void addItem_(const QSharedPointer<DrawingItemBase> &);
  virtual void removeItem_(const QSharedPointer<DrawingItemBase> &);

private slots:
  void initNewItem(DrawingItemBase *item);

private:
  QSharedPointer<DrawingItemBase> hoverItem_;
  QSharedPointer<DrawingItemBase> incompleteItem_; // item in the process of being completed (e.g. having its control points manually placed)
  bool repaintNeeded_;
  bool skipRepaint_;
  QUndoStack undoStack_;
  QUndoView *undoView_;

  QAction* cutAction;
  QAction* copyAction;
  QAction* pasteAction;
  QAction* editPropertiesAction;
  QAction* editStyleAction;
  QAction* undoAction;
  QAction* redoAction;
  QAction* selectAction;
  QAction* createPolyLineAction;
  QAction* createSymbolAction;
  QAction* createTextAction;
  QAction* createCompositeAction;

  enum Mode {
    SelectMode, CreatePolyLineMode, CreateSymbolMode, CreateTextMode, CreateCompositeMode
  } mode_;

  void incompleteMousePress(QMouseEvent *);
  void incompleteMouseRelease(QMouseEvent *);
  void incompleteMouseMove(QMouseEvent *);
  void incompleteMouseDoubleClick(QMouseEvent *);
  void incompleteKeyPress(QKeyEvent *);
  void incompleteKeyRelease(QKeyEvent *);
  void pushCommands(const QSet<QSharedPointer<DrawingItemBase> > &addedItems,
                    const QSet<QSharedPointer<DrawingItemBase> > &removedItems,
                    const QList<QUndoCommand *> &undoCommands);

  // Clipboard operations
  void copyItems(const QSet<QSharedPointer<DrawingItemBase> > &);

  void updateActions();
  void updateTimes();
  void updateActionsAndTimes();

  virtual bool parseSetup() { return true; } // n/a

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
  AddOrRemoveItemsCommand(const QSet<QSharedPointer<DrawingItemBase> > &, const QSet<QSharedPointer<DrawingItemBase> > &);
  virtual ~AddOrRemoveItemsCommand() {}

private:
  QSet<QSharedPointer<DrawingItemBase> > addedItems_;
  QSet<QSharedPointer<DrawingItemBase> > removedItems_;
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
