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

#ifndef EDITITEM_SYMBOL_H
#define EDITITEM_SYMBOL_H

#include <QtGui>
#include "edititembase.h"
#include "drawingsymbol.h"

namespace EditItem_Symbol {

class Symbol : public EditItemBase, public DrawingItem_Symbol::Symbol
{
  Q_OBJECT
  friend class SetGeometryCommand;

public:
  Symbol(int = -1);
  virtual ~Symbol();

protected:
  virtual void draw();
  virtual void drawHoverHighlightingBG(bool, bool) const;
  virtual void drawHoverHighlighting(bool, bool) const;

private:
  virtual DrawingItemBase *cloneSpecial(bool) const;

  virtual bool hit(const QPointF &, bool) const;

  virtual void mousePress(QMouseEvent *, bool &, bool *);
  virtual void mouseRelease(QMouseEvent *event, bool &repaintNeeded);

  virtual void incompleteMousePress(QMouseEvent *, bool &, bool &, bool &);
  virtual void incompleteKeyPress(QKeyEvent *, bool &, bool &, bool &);

  virtual QString infoString() const { return QString("%1 type=%2").arg(DrawingItemBase::infoString()).arg(metaObject()->className()); }

  virtual void resize(const QPointF &);
  virtual void updateControlPoints();
  virtual void setPoints(const QList<QPointF> &);
};

} // namespace EditItem_Symbol

#endif // EDITITEM_SYMBOL_H
