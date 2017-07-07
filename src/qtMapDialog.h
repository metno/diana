/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2013 met.no

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
#ifndef _mapdialog_h
#define _mapdialog_h

#include "diController.h"
#include "diMapInfo.h"
#include <QWidget>
#include <vector>

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class ToggleButton;
class QCheckBox;

class Ui_MapDialog;

/**
   \brief Map selection dialogue

   Dialogue for selections of maps, predefined projections/areas and
   various map-settings
*/
class MapDialog : public QWidget {
  Q_OBJECT

public:
  MapDialog(QWidget* parent, Controller* llctrl);
  ~MapDialog();

  /// the plot info strings
  PlotCommand_cpv getOKString();
  /// set the dialogue elements from a plot info string
  void putOKString(const PlotCommand_cpv& vstr);
  /// creates a short name for the current settings (used in quick menues)
  std::string getShortname();
  /// returns all settings in logfile format
  std::vector<std::string> writeLog();
  /// set the dialogue elements from logfile settings
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
  /// choose the favorite settings
  void useFavorite();

protected:
  void closeEvent(QCloseEvent*) override;

private Q_SLOTS:
  void mapboxChanged();
  void selectedMapboxClicked(QListWidgetItem* item);
  void mapdeleteClicked();
  void mapalldeleteClicked();

  void lon_checkboxActivated(bool);
  void lat_checkboxActivated(bool);
  void showframe_checkboxActivated(bool);

  void cont_checkboxActivated(bool);
  void cont_linestyleActivated();
  void cont_zordercboxActivated(int index);

  void land_checkboxActivated(bool);
  void land_colorcboxActivated(int index);
  void land_zordercboxActivated(int index);

  void helpClicked();
  void applyhideClicked();
  void saveFavoriteClicked();
  void useFavoriteClicked();

Q_SIGNALS:
  void MapApply();
  void MapHide();
  void showsource(const std::string, const std::string = "");

private:
  void getMapInfoFromUi(MapInfo& mi);
  void getLonLatValuePos(QComboBox* combo, bool& show, std::string& value_pos);
  void setMapInfoToUi(const MapInfo& mi);
  void setLonLatValuePos(QComboBox* combo, bool show, const std::string& value_pos);

private:
  MapDialogInfo m_MapDI; // all maps and areas
  PlotCommand_cpv favorite; // favorite options
  int numMaps; // number of maps
  std::vector<int> selectedmaps; // maps selected
  int activemap; // active selected map
  std::vector<int> logmaps; // selected maps ready for logging

  std::vector<int> densities; // latlon densities (degrees, multiplied with 1000)
  std::vector<std::string> positions; // all defined positions
  std::map<std::string,int> positions_map; // all defined positions
  Controller* m_ctrl;

  Ui_MapDialog* ui;

  QColor* pixcolor;
};

#endif
