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
class UndoView;

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

  void addItem(const QSharedPointer<DrawingItemBase> &, bool = false, bool = false);
  void editItem(const QSharedPointer<DrawingItemBase> &item);
  void editItem(DrawingItemBase *item);
  void removeItem(const QSharedPointer<DrawingItemBase> &);

  virtual DrawingItemBase *createItem(const QString &type);
  virtual QSharedPointer<DrawingItemBase> createItemFromVarMap(const QVariantMap &vmap, QString *error);

  // Returns the undo stack.
  QUndoStack *undoStack();

  bool canRedo() const;
  bool canUndo() const;
  bool hasIncompleteItem() const;
  bool needsRepaint() const;

  QList<QSharedPointer<DrawingItemBase> > findHitItems(
      const QPointF &, QList<QSharedPointer<DrawingItemBase> > * = 0) const;

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

  void enableItemChangeNotification(bool = true);
  void setItemChangeFilter(const QString &);
  void emitItemChanged() const;

  void emitLoadFile(const QString &) const;

  void setItemsVisibilityForced(bool);

  virtual QString plotElementTag() const;

  void updateJoins(bool = false);

  virtual bool parseSetup();

public slots:
  void abortEditing();
  void completeEditing();
  void copySelectedItems();
  void cutSelectedItems();
  void deselectItem(const QSharedPointer<DrawingItemBase> &, bool = true);
  void deselectAllItems(bool = true);
  void editProperties();
  void editStyle();
  void setStyleType();
  void keyPress(QKeyEvent *);
  void mouseDoubleClick(QMouseEvent *);
  void mouseMove(QMouseEvent *);
  void mousePress(QMouseEvent *);
  void mouseRelease(QMouseEvent *);
  void pasteItems();
  void joinSelectedItems();
  void unjoinSelectedItems();
  void toggleReversedForSelectedItems();
  void redo();
  void repaint();
  void reset();
  bool selectItem(const QSharedPointer<DrawingItemBase> &, bool = false, bool = true);
  bool selectItem(int, bool = false, bool = true);
  void setSelectMode();
  void startStopEditing(bool start);
  void undo();

  void update();

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
  void itemChanged(const QVariantMap &) const;
  void itemRemoved(int) const;
  void timesUpdated();
  void setWorkAreaCursor(const QCursor &);
  void unsetWorkAreaCursor();
  void editing(bool);
  void loadFile(const QString &) const;

protected:
  virtual void addItem_(const QSharedPointer<DrawingItemBase> &, bool = true, bool = false);
  virtual void removeItem_(const QSharedPointer<DrawingItemBase> &, bool = true);

private slots:
  void initNewItem(DrawingItemBase *item);

private:
  bool selectingOnly_;
  QList<QSharedPointer<DrawingItemBase> > hitItems_;
  QSharedPointer<DrawingItemBase> incompleteItem_; // item in the process of being completed (e.g. having its control points manually placed)
  bool repaintNeeded_;
  bool skipRepaint_;
  quint32 hitOffset_;
  QPoint lastHoverPos_;
  QUndoStack undoStack_;
  UndoView *undoView_;

  QAction* copyAction_;
  QAction* cutAction_;
  QAction* pasteAction_;
  QAction* joinAction_;
  QAction* unjoinAction_;
  QAction* toggleReversedAction_;
  QAction* editPropertiesAction_;
  QAction* editStyleAction_;
  QAction* undoAction_;
  QAction* redoAction_;
  QAction* selectAction_;
  QAction* createPolyLineAction_;
  QAction* createSymbolAction_;
  QAction* createTextAction_;
  QAction* createCompositeAction_;

  enum Mode {
    SelectMode, CreatePolyLineMode, CreateSymbolMode, CreateTextMode, CreateCompositeMode
  } mode_;

  void incompleteMousePress(QMouseEvent *);
  void incompleteMouseRelease(QMouseEvent *);
  void incompleteMouseMove(QMouseEvent *);
  void incompleteMouseDoubleClick(QMouseEvent *);
  void incompleteKeyPress(QKeyEvent *);

  void copyItems(const QSet<QSharedPointer<DrawingItemBase> > &);

  void updateActions();
  void updateTimes();
  void updateActionsAndTimes();

  QString itemChangeFilter_;
  bool itemChangeNotificationEnabled_;
  bool itemsVisibilityForced_;
  bool itemPropsDirectlyEditable_;

  bool cycleHitOrder(QKeyEvent *);

  QList<QList<QSharedPointer<DrawingItemBase> > > oldItemStates_;
  void saveItemStates();
  void pushModifyItemsCommand();

  void adjustSelectedJoinPoints();

  QSharedPointer<DrawingItemBase> hitItem_; // current hit item
  QHash<DrawingItemBase *, QList<QPointF> > oldGeoms_; // original geometries

  static EditItemManager *self_;   // singleton instance pointer
};


class ModifyItemsCommand : public QUndoCommand
{
public:
  ModifyItemsCommand(const QList<QList<QSharedPointer<DrawingItemBase> > > &, const QList<QList<QSharedPointer<DrawingItemBase> > > &, const QString &);
  virtual ~ModifyItemsCommand() {}
private:
  QList<QList<QSharedPointer<DrawingItemBase> > > oldItemStates_;
  QList<QList<QSharedPointer<DrawingItemBase> > > newItemStates_;
  virtual void undo();
  virtual void redo();
};

#endif // EDITITEMMANAGER_H
