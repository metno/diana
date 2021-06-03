/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 MET Norway

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

#ifndef WebMapPlotListModel_h
#define WebMapPlotListModel_h 1

#include <QAbstractListModel>

class WebMapDialog;

class WebMapPlotListModel : public QAbstractListModel
{
  Q_OBJECT

public:
  WebMapPlotListModel(WebMapDialog* parent = 0);

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

public:
  void onPlotsRemoveBegin();
  void onPlotsRemoveEnd();
  void onPlotAddBegin(int idx);
  void onPlotAddEnd();

private:
  WebMapDialog* dialog() const;
};

#endif // WebMapPlotListModel_h
