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

#include <EditItems/modifypropertiescommand.h>
#include <diEditItemManager.h>

namespace EditItems {

ModifyPropertiesCommand::ModifyPropertiesCommand(
    const QString &text,
    const QList<QSharedPointer<DrawingItemBase> > &items,
    const QList<QVariantMap> &oldProperties,
    const QList<QVariantMap> &newProperties,
    QUndoCommand * parent)
  : QUndoCommand(text, parent)
  , items_(items)
  , oldProperties_(oldProperties)
  , newProperties_(newProperties)
{
}

void ModifyPropertiesCommand::undo()
{
  for (int i = 0; i < items_.size(); ++i)
    items_.at(i)->setProperties(oldProperties_.at(i));
}

void ModifyPropertiesCommand::redo()
{
  for (int i = 0; i < items_.size(); ++i)
    items_.at(i)->setProperties(newProperties_.at(i));
}

} // namespace
