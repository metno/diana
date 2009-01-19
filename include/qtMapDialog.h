/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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


#include <QDialog>

#include <diController.h>
#include <vector>
#include <miString.h>

using namespace std;

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class ToggleButton;
class QCheckBox;

/**
   \brief Map selection dialogue
   
   Dialogue for selections of maps, predefined projections/areas and various map-settings

*/

class MapDialog: public QDialog
{
  Q_OBJECT
public:

  MapDialog( QWidget* parent, const MapDialogInfo& mdi );
  MapDialog( QWidget* parent, Controller* llctrl );

  /// the plot info strings
  vector<miString> getOKString();
  /// set the dialogue elements from a plot info string
  void putOKString(const vector<miString>& vstr);
  /// creates a short name for the current settings (used in quick menues)
  miString getShortname();

  /// returns all settings in logfile format
  vector<miString> writeLog();
  /// set the dialogue elements from logfile settings
  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion, const miString& logVersion);

  /// choose the favourite settings
  void useFavourite();

protected:
  void closeEvent( QCloseEvent* );

private:

  void ConstructorCernel( const MapDialogInfo mdi );

private slots:
//  void areaboxSelected( QListWidgetItem* item );
  void mapboxChanged();
  void selectedMapboxClicked( QListWidgetItem* item );
  void mapdeleteClicked();
  void mapalldeleteClicked();

  void ll_checkboxActivated(bool);
  void ll_linecboxActivated( int index );
  void ll_linetypeboxActivated( int index );
  void ll_colorcboxActivated( int index );
  void ll_densitycboxActivated( int index );
  void ll_zordercboxActivated( int index );

  void backcolorcboxActivated(int index);

  void showframe_checkboxActivated(bool);
  void ff_linecboxActivated( int index );
  void ff_linetypeboxActivated( int index );
  void ff_colorcboxActivated( int index );
  void ff_zordercboxActivated( int index );

  void cont_checkboxActivated(bool);
  void cont_linecboxActivated( int index );
  void cont_linetypeboxActivated( int index );
  void cont_colorcboxActivated( int index );
  void cont_zordercboxActivated( int index );

  void land_checkboxActivated(bool);
  void land_colorcboxActivated( int index );
  void land_zordercboxActivated( int index );

  void helpClicked();
  void applyhideClicked();
  void savefavouriteClicked();
  void usefavouriteClicked();

  signals:
  void MapApply();
  void MapHide();
  void showsource(const miString, const miString="");
 

private:
  MapDialogInfo m_MapDI;      // all maps and areas
  vector<miString> favourite; // favourite options
  int numMaps;                // number of maps
  vector<int> selectedmaps;   // maps selected
  int activemap;              // active selected map
  vector<int> logmaps;        // selected maps ready for logging
  
  vector<Colour::ColourInfo> cInfo; // all defined colours
  vector<miString> linetypes; // all defined linetypes
  vector<miString> zorders;   // all defined zorders
  vector<miString> densities; // latlon densities (degrees)
  Controller* m_ctrl;

  // areas
  QLabel* arealabel;
  QListWidget* areabox;

  // latlon options
  QLabel* ll_label;
  QLabel* ll_linelabel;
  QLabel* ll_linetypelabel;
  QLabel* ll_colorlabel;
  QLabel* ll_zorderlabel;
  QLabel* ll_densitylabel;
  QCheckBox* latlon;
  QComboBox* ll_linecbox;
  QComboBox* ll_linetypebox;
  QComboBox* ll_colorcbox;
  QComboBox* ll_zorder;
  QComboBox* ll_density;
  bool latlonb;
  miString latlonc;
  miString latlonlw;
  miString latlonlt;
  int latlonz;
  float latlond;

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
  miString framec;
  miString framelw;
  miString framelt;
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
  QPushButton* savefavourite;
  QPushButton* usefavourite;
  QPushButton* mapapply;
  QPushButton* mapapplyhide;
  QPushButton* maphide;
  QPushButton* maphelp;

  QColor* pixcolor;

};


#endif
