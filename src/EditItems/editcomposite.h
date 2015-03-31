/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2014 met.no

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

#ifndef EDITCOMPOSITE_H
#define EDITCOMPOSITE_H

#include "drawingcomposite.h"
#include "edititembase.h"

namespace EditItem_Composite {

class Composite : public EditItemBase, public DrawingItem_Composite::Composite
{
  Q_OBJECT

public:
  Composite(int = -1);
  virtual ~Composite();

  virtual void mousePress(QMouseEvent *event, bool &repaintNeeded,
                          QList<QUndoCommand *> *undoCommands,
                          QSet<QSharedPointer<DrawingItemBase> > *items = 0,
                          const QSet<QSharedPointer<DrawingItemBase> > *selItems = 0,
                          bool *multiItemOp = 0);

  virtual void incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
  virtual void incompleteMouseRelease(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);

  virtual QList<QAction *> actions(const QPoint &) const;

  virtual void createElements();
  virtual bool isEditable(DrawingItemBase *element) const;

protected:
  virtual void drawHoverHighlighting(bool, bool) const;
  virtual void drawIncomplete() const;

  virtual void move(const QPointF &pos);
  virtual void resize(const QPointF &);
  virtual void updateControlPoints();

  virtual DrawingItemBase *newCompositeItem() const;
  virtual DrawingItemBase *newPolylineItem() const;
  virtual DrawingItemBase *newSymbolItem() const;
  virtual DrawingItemBase *newTextItem() const;

private slots:
  void editItem();

private:
  virtual DrawingItemBase *cloneSpecial(bool) const;

  QAction *editAction;
};

class CompositeEditor : public QWidget {
  Q_OBJECT

public:
  CompositeEditor(Composite *item);
  virtual ~CompositeEditor();

  void applyChanges();

private slots:
  void updateSymbol(QAction *action);
  void updateText(const QString &text);

private:
  void createElements(const DrawingItemBase::Category &category, const QString &name);

  QStringList objects;
  QStringList values;
  QStringList styles;

  Composite *item;
  QHash<int, QVariantList> changes;
  QList<CompositeEditor *> childEditors;
  QHash<int, QWidget *> editors;
};

} // namespace EditItem_Composite

#endif // EDITCOMPOSITE_H
