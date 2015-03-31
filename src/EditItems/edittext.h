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

#ifndef EDITTEXT_H
#define EDITTEXT_H

#include "drawingtext.h"
#include "edititembase.h"

namespace EditItem_Text {

class Text : public EditItemBase, public DrawingItem_Text::Text
{
  Q_OBJECT

public:
  Text(int = -1);
  virtual ~Text();

private:

  virtual void mousePress(QMouseEvent *event, bool &repaintNeeded, bool *multiItemOp = 0);
  virtual void mouseDoubleClick(QMouseEvent *, bool &);
  virtual void incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
  virtual void incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);

  virtual QList<QAction *> actions(const QPoint &) const;

protected:
  virtual void drawHoverHighlighting(bool, bool) const;
  virtual void drawIncomplete() const;

  virtual void resize(const QPointF &);
  virtual void updateControlPoints();
  virtual void setPoints(const QList<QPointF> &points);

private slots:
  bool editText();

private:
  virtual DrawingItemBase *cloneSpecial(bool) const;

  QAction *editAction;
};

} // namespace EditItem_Text

#endif // EDITTEXT_H
