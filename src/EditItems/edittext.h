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
public:
  Text();
  virtual ~Text();

  virtual bool hit(const QPointF &pos, bool selected) const;
  virtual bool hit(const QRectF &bbox) const;

  virtual void mousePress(QMouseEvent *event, bool &repaintNeeded,
                          QList<QUndoCommand *> *undoCommands,
                          QSet<DrawingItemBase *> *items = 0,
                          const QSet<DrawingItemBase *> *selItems = 0,
                          bool *multiItemOp = 0);

  virtual void incompleteMousePress(QMouseEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);
  virtual void incompleteKeyPress(QKeyEvent *event, bool &repaintNeeded, bool &complete, bool &aborted);

protected:
  virtual void resize(const QPointF &);
  virtual void updateControlPoints();
  virtual void drawHoverHighlighting(bool) const;
  virtual void drawIncomplete() const;

private:
  QSizeF getStringSize() const;

  int cursor_;
};

} // namespace EditItem_Text

#endif // EDITTEXT_H
