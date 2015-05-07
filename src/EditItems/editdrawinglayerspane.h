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

#ifndef EDITDRAWINGLAYERSPANE_H
#define EDITDRAWINGLAYERSPANE_H

#include <EditItems/layerspanebase.h>
#include <EditItems/layergroup.h>

namespace EditItems {

class LayerManager;

class EditDrawingLayersPane : public LayersPaneBase
{
  Q_OBJECT
public:
  EditDrawingLayersPane(const QString &);
  void addDuplicate(const QSharedPointer<Layer> &);
private:
  virtual void updateButtons();
  virtual void addContextMenuActions(QMenu &) const;
  virtual bool handleContextMenuAction(const QAction *, const QList<LayerWidget *> &);
  virtual bool handleKeyPressEvent(QKeyEvent *);
  void add(const QSharedPointer<Layer> &, bool = false);
  void merge(const QList<LayerWidget *> &);
  void duplicate(const QList<LayerWidget *> &);
  void save(const QList<LayerWidget *> &);
  QString scratchLayerName_;
  QToolButton *addEmptyButton_;
  QToolButton *addFromFileButton_;
  QToolButton *refreshSelectedButton_;
  QToolButton *selectAllItemsButton_;
  QToolButton *deselectAllItemsButton_;
  QToolButton *mergeSelectedButton_;
  QToolButton *duplicateSelectedButton_;
  QToolButton *removeSelectedButton_;
  QToolButton *saveSelectedButton_;
  QToolButton *undoButton_;
  QToolButton *redoButton_;
  QAction *refresh_act_;
  QAction *selectAll_act_;
  QAction *deselectAll_act_;
  QAction *merge_act_;
  QAction *duplicate_act_;
  QAction *remove_act_;
  QAction *save_act_;
  QString lastSelSaveFName_;
private slots:
  void selectAll();
  void deselectAll();
  void addEmpty();
  void addFromFile();
  void refreshSelected();
  void mergeSelected();
  void duplicateSelected();
  void removeSelected();
  void saveSelected();
  void undo();
  void redo();
  virtual void handleLayersUpdate();
  void loadFile(const QString &);
};

} // namespace

#endif // EDITDRAWINGLAYERSPANE_H
