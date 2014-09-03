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

#ifndef EDITDRAWINGDIALOG_H
#define EDITDRAWINGDIALOG_H

#include "qtDataDialog.h"
#include <EditItems/layer.h>

class EditItemManager;

namespace EditItems {

class LayerGroupsPane;
class EditDrawingLayersPane;

class EditDrawingDialog : public DataDialog
{
  Q_OBJECT

public:
  EditDrawingDialog(QWidget *, Controller *);

  virtual std::string name() const;
  virtual void updateDialog() {} // n/a
  virtual std::vector<std::string> getOKString() { return std::vector<std::string>(); } // n/a
  virtual void putOKString(const std::vector<std::string> &) {} // n/a

public slots:
  void handleNewEditLayerRequested(const QSharedPointer<Layer> &);

private:
  EditItemManager *editm_;
  EditDrawingLayersPane *layersPane_;

private slots:
  virtual void updateTimes() {} // n/a

  // ### FOR TESTING:
  void dumpStructure();
  void showInfo(bool);
  void showUndoStack(bool);

  void setItemsVisibilityForced(bool);
};

} // namespace

#endif // EDITDRAWINGDIALOG_H
