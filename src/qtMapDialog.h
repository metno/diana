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

#include <diController.h>
#include <QDialog>
#include <vector>

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class ToggleButton;
class QCheckBox;

/**
   \brief Map selection dialogue

   Dialogue for selections of maps, predefined projections/areas and
   various map-settings
*/
class MapDialog : public QDialog {
Q_OBJECT
public:

  MapDialog(QWidget* parent, const MapDialogInfo& mdi);
  MapDialog(QWidget* parent, Controller* llctrl);

  /// the plot info strings
  std::vector<std::string> getOKString();
  /// set the dialogue elements from a plot info string
  void putOKString(const std::vector<std::string>& vstr);
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
  void closeEvent(QCloseEvent*);

private:

  void ConstructorCernel(const MapDialogInfo mdi);

private slots:
  //  void areaboxSelected( QListWidgetItem* item );
  void mapboxChanged();
  void selectedMapboxClicked(QListWidgetItem* item);
  void mapdeleteClicked();
  void mapalldeleteClicked();

  void lon_checkboxActivated(bool);
  void lon_linecboxActivated(int index);
  void lon_linetypeboxActivated(int index);
  void lon_densitycboxActivated(int index);
  void lon_zordercboxActivated(int index);
  void lon_showValueActivated(bool);

  void lat_checkboxActivated(bool);
  void lat_linecboxActivated(int index);
  void lat_linetypeboxActivated(int index);
  void lat_densitycboxActivated(int index);
  void lat_zordercboxActivated(int index);
  void lat_showValueActivated(bool);

  void showframe_checkboxActivated(bool);
  void ff_linecboxActivated(int index);
  void ff_linetypeboxActivated(int index);
  void ff_zordercboxActivated(int index);

  void cont_checkboxActivated(bool);
  void cont_linecboxActivated(int index);
  void cont_linetypeboxActivated(int index);
  void cont_colorcboxActivated(int index);
  void cont_zordercboxActivated(int index);

  void land_checkboxActivated(bool);
  void land_colorcboxActivated(int index);
  void land_zordercboxActivated(int index);

  void helpClicked();
  void applyhideClicked();
  void saveFavoriteClicked();
  void useFavoriteClicked();

  signals:
  void MapApply();
  void MapHide();
  void showsource(const std::string, const std::string = "");

private:
  MapDialogInfo m_MapDI; // all maps and areas
  std::vector<std::string> favorite; // favorite options
  int numMaps; // number of maps
  std::vector<int> selectedmaps; // maps selected
  int activemap; // active selected map
  std::vector<int> logmaps; // selected maps ready for logging

  std::vector<Colour::ColourInfo> cInfo; // all defined colours
  std::vector<std::string> linetypes; // all defined linetypes
  std::vector<std::string> zorders; // all defined zorders
  std::vector<std::string> densities; // latlon densities (degrees)
  std::vector<std::string> positions; // all defined positions
  std::map<std::string,int> positions_map; // all defined positions
  Controller* m_ctrl;

  // areas
  QLabel* arealabel;
  QListWidget* areabox;

  // latlon options
  QLabel* lon_label;
  QLabel* lon_linelabel;
  QLabel* lon_linetypelabel;
  QLabel* lon_colorlabel;
  QLabel* lon_zorderlabel;
  QLabel* lon_densitylabel;
  QCheckBox* showlon;
  QComboBox* lon_linecbox;
  QComboBox* lon_linetypebox;
  QComboBox* lon_colorcbox;
  QComboBox* lon_zorder;
  QComboBox* lon_density;
  QCheckBox* lon_showvalue;
  QComboBox* lon_valuepos;
  bool lonb;
  std::string lonlw;
  std::string lonlt;
  int lonz;
  float lond;
  bool lonshowvalue;

  QLabel* lat_label;
  QLabel* lat_linelabel;
  QLabel* lat_linetypelabel;
  QLabel* lat_colorlabel;
  QLabel* lat_zorderlabel;
  QLabel* lat_densitylabel;
  QCheckBox* showlat;
  QComboBox* lat_linecbox;
  QComboBox* lat_linetypebox;
  QComboBox* lat_colorcbox;
  QComboBox* lat_zorder;
  QComboBox* lat_density;
  QCheckBox* lat_showvalue;
  QComboBox* lat_valuepos;
  bool latb;
  std::string latlw;
  std::string latlt;
  int latz;
  float latd;
  bool latshowvalue;

  // backgroundcolour
  QLabel* backcolorlabel;
  QComboBox* backcolorcbox;

  // frame
  QCheckBox* showframe;
  bool frameb;
  QLabel* framelabel;
  QLabel* ff_linelabel;
  QLabel* ff_linetypelabel;
  QLabel* ff_colorlabel;
  QLabel* ff_zorderlabel;
  QComboBox* ff_linecbox;
  QComboBox* ff_linetypebox;
  QComboBox* ff_colorcbox;
  QComboBox* ff_zorder;
  std::string framelw;
  std::string framelt;
  int framez;

  // maps and selected maps
  QLabel* maplabel;
  QListWidget* mapbox;
  QLabel* selectedMaplabel;
  QListWidget* selectedMapbox;
  QPushButton* mapdelete;
  QPushButton* mapalldelete;

  // contourlines options
  QLabel* cont_label;
  QLabel* cont_linelabel;
  QLabel* cont_linetypelabel;
  QLabel* cont_colorlabel;
  QLabel* cont_zorderlabel;
  QCheckBox* contours;
  QComboBox* cont_linecbox;
  QComboBox* cont_linetypebox;
  QComboBox* cont_colorcbox;
  QComboBox* cont_zorder;

  // filled land options
  QLabel* land_label;
  QLabel* land_colorlabel;
  QLabel* land_zorderlabel;
  QCheckBox* filledland;
  QComboBox* land_colorcbox;
  QComboBox* land_zorder;

  // buttons
  QPushButton* savefavorite;
  QPushButton* usefavorite;
  QPushButton* mapapply;
  QPushButton* mapapplyhide;
  QPushButton* maphide;
  QPushButton* maphelp;

  QColor* pixcolor;

};

#endif
