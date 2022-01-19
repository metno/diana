/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2021 met.no

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

#ifndef WebMapDialog_h
#define WebMapDialog_h 1

#include "qtDataDialog.h"
#include "diKVListPlotCommand.h"

#include <memory>

class QSortFilterProxyModel;
class QStringListModel;
class Ui_WebMapDialog;

class WebMapLayer;
class WebMapService;

extern const std::string WEBMAP_SERVICE;
extern const std::string WEBMAP_LAYER;
extern const std::string WEBMAP_ZORDER;
extern const std::string WEBMAP_TIME_TOLERANCE;
extern const std::string WEBMAP_TIME_OFFSET;

class WebMapDialog : public DataDialog
{
  Q_OBJECT

public:
  WebMapDialog(QWidget *parent, Controller *ctrl);
  ~WebMapDialog();

  std::string name() const override;
  void updateDialog() override;
  PlotCommand_cpv getOKString() override;
  void putOKString(const PlotCommand_cpv& vstr) override;

  size_t plotCommandCount() const
    { return mOk.size(); }

  KVListPlotCommand_cp plotCommand(size_t idx) const;

public /*Q_SLOTS*/:
  void updateTimes() override;

private Q_SLOTS:
  void onServiceRefreshStarting();
  void onServiceRefreshFinished();

  void onAddServicesFilter(const QString&);
  void onAddLayersFilter(const QString&);

  void checkAddComplete();
  void onAddNext();
  void onAddBack();
  void onAddRestart();

  void onModifyApply();
  void onModifyReset();

  void onModifyLayerSelected();
  void onModifyLayerRemove();

  void onModifyStyleChanged();
  void onModifyGreyToggled();
  void onModifyAlphaScaleChanged();
  void onModifyAlphaOffsetChanged();
  void onModifyPlotOrderChanged();
  void onModifyTimeToleranceChanged();
  void onModifyTimeOffsetChanged();
  void onModifyDimensionValuesChanged();

private:
  void setupUi();

  void initializeAddServicePage(bool forward);
  bool isAddServiceComplete();
  WebMapService* selectedAddService() const;

  void initializeAddLayerPage(bool forward);
  bool isAddLayerComplete();
  const WebMapLayer* selectedAddLayer() const;

  void updateAddLayers();
  void addSelectedLayer();

  void replaceCommandKV(const miutil::KeyValue& kv);
  void replaceCommandKV(const miutil::KeyValue& kv, size_t idx);

private:
  enum { AddServicePage, AddLayerPage };

  std::unique_ptr<Ui_WebMapDialog> ui;

  QStringListModel* mServicesModel;
  QSortFilterProxyModel* mServicesFilter;

  QStringListModel* mLayersModel;
  QSortFilterProxyModel* mLayersFilter;

  WebMapService* mAddSelectedService;

  PlotCommand_cpv mOk;
};

#endif // WebMapDialog_h
