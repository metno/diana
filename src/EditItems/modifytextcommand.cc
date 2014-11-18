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

#include <EditItems/modifytextcommand.h>
#include <diEditItemManager.h>

namespace EditItems {

ModifyTextCommand::ModifyTextCommand(
    const QString &text,
    const QSharedPointer<DrawingItem_Text::Text> &item,
    const QStringList &oldText,
    const QStringList &newText,
    QUndoCommand *parent)
  : QUndoCommand(text, parent)
  , item_(item)
  , oldText_(oldText)
  , newText_(newText)
{
}

void ModifyTextCommand::undo()
{
  item_->setText(oldText_);
}

void ModifyTextCommand::redo()
{
  item_->setText(newText_);
}

} // namespace
