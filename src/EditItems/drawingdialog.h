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

#ifndef DRAWINGDIALOG_H
#define DRAWINGDIALOG_H

#include "qtDataDialog.h"
#include <EditItems/layer.h>

class DrawingManager;
class LayerManager;

namespace EditItems {

class LayerGroupsPane;
class DrawingLayersPane;

class DrawingDialog : public DataDialog
{
  Q_OBJECT

public:
  DrawingDialog(QWidget *, Controller *);

  virtual std::string name() const;
  virtual void updateDialog();
  virtual std::vector<std::string> getOKString();
  virtual void putOKString(const std::vector<std::string> &);

private:
  DrawingManager *drawm_;
  LayerGroupsPane *layerGroupsPane_;
  DrawingLayersPane *layersPane_; // keeps the active layers
  QHash<QString, QString> fileMap_;

private slots:
  virtual void updateTimes();
  void makeProduct();
  void handleDialogUpdated();

  // ### FOR TESTING:
  void dumpStructure();
  void showInfo(bool);

signals:
  void newEditLayerRequested(const QSharedPointer<Layer> &);
};

} // namespace

#endif // DRAWINGDIALOG_H
