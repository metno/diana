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

#ifndef MODIFYTEXTCOMMAND_H
#define MODIFYTEXTCOMMAND_H

#include <EditItems/drawingtext.h>

#include <QUndoCommand>
#include <QString>
#include <QStringList>
#include <QSharedPointer>

namespace EditItems {

class ModifyTextCommand : public QUndoCommand
{
public:
  ModifyTextCommand(
      const QString &,
      const QSharedPointer<DrawingItem_Text::Text> &,
      const QStringList &,
      const QStringList &,
      QUndoCommand * = 0);
  virtual ~ModifyTextCommand() {}
private:
  QSharedPointer<DrawingItem_Text::Text> item_;
  QStringList oldText_;
  QStringList newText_;
  virtual void undo();
  virtual void redo();
};

} // namespace

#endif // MODIFYTEXTCOMMAND_H
