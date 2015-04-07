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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtMapDialog.h"
#include "qtUtility.h"
#include "qtToggleButton.h"
#include "diLinetype.h"

#include <puTools/miStringFunctions.h>

#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QFrame>
#include <QCheckBox>
#include <QToolTip>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.MapDialog"
#include <miLogger/miLogging.h>

using namespace std;

#define HEIGHTLB 105
#define HEIGHTLBSMALL 70
#define HEIGHTLBXSMALL 50

/*********************************************/
MapDialog::MapDialog(QWidget* parent, Controller* llctrl) :
  QDialog(parent)
  {
#ifdef dMapDlg
  METLIBS_LOG_DEBUG("MapDialog::MapDialog called");
#endif
  m_ctrl = llctrl;
  ConstructorCernel(llctrl->initMapDialog());
  }

/*********************************************/
MapDialog::MapDialog(QWidget* parent, const MapDialogInfo& mdi) :
  QDialog(parent)
  {
#ifdef dMapDlg
  METLIBS_LOG_DEBUG("MapDialog::MapDialog called");
#endif
  ConstructorCernel(mdi);
  }

/*********************************************/
void MapDialog::ConstructorCernel(const MapDialogInfo mdi)
{
#ifdef dMapDlg
  METLIBS_LOG_DEBUG("MapDialog::ConstructorCernel called");
#endif

  setWindowTitle(tr("Map and Area"));

  // all defined maps etc.
  m_MapDI = mdi;

  numMaps = m_MapDI.maps.size();
  activemap = -1;

  int i;

  // linetypes
  linetypes = Linetype::getLinetypeNames();

  // colour
  cInfo = Colour::getColourInfo();

  // zorders
  zorders.push_back(tr("lowest").toStdString());
  zorders.push_back(tr("auto").toStdString());
  zorders.push_back(tr("highest").toStdString());

  // latlon densities (degrees)
  densities.push_back("0.5");
  densities.push_back("1");
  densities.push_back("2");
  densities.push_back("2.5");
  densities.push_back("3");
  densities.push_back("4");
  densities.push_back("5");
  densities.push_back("10");
  densities.push_back("15");
  densities.push_back("30");
  densities.push_back("90");
  densities.push_back("180");

  // positions
  std::vector<std::string> positions_tr; // all defined positions
  positions_tr.push_back(tr("left").toStdString());
  positions_tr.push_back(tr("bottom").toStdString());
  positions_tr.push_back(tr("both").toStdString());
  positions.push_back("left");
  positions.push_back("bottom");
  positions.push_back("both");
  positions_map["left"]=0;;
  positions_map["bottom"]=1;;
  positions_map["both"]=2;;
//obsolete syntax
  positions_map["0"]=0;;
  positions_map["1"]=1;;
  positions_map["2"]=2;;

  // ==================================================
  // arealabel
  arealabel = TitleLabel(tr("Area/Projection"), this);

  // areabox
  areabox = new QListWidget(this);

  int nr_area = m_MapDI.areas.size();
  int area_defIndex = 0;
  for (int i = 0; i < nr_area; i++) {
    areabox->addItem(QString(m_MapDI.areas[i].c_str()));
    if (m_MapDI.areas[i] == m_MapDI.default_area)
      area_defIndex = i;
  }
  // select default area
  areabox->setCurrentRow(area_defIndex);

  // ==================================================
  // maplabel
  maplabel = TitleLabel(tr("Maps"), this);
  // selectedMaplabel
  selectedMaplabel = TitleLabel(tr("Selected maps"), this);

  // mapbox
  mapbox = new QListWidget(this);

  for (int i = 0; i < numMaps; i++) {
    mapbox->addItem(QString(m_MapDI.maps[i].name.c_str()));
  }

  mapbox->setSelectionMode(QAbstractItemView::MultiSelection);

  // select default maps
  for (i = 0; i < numMaps; i++) {
    for (unsigned int k = 0; k < m_MapDI.default_maps.size(); k++) {
      if (m_MapDI.default_maps[k] == m_MapDI.maps[i].name) {
        mapbox->item(i)->setSelected(true);
      }
    }
  }

  connect(mapbox, SIGNAL(itemSelectionChanged () ),
      SLOT( mapboxChanged() ) );

  // delete buttons
  mapdelete= new QPushButton(tr("Delete"), this);
  mapdelete->setToolTip(tr("Remove selected map from the list \"Selected maps\"") );
  connect( mapdelete, SIGNAL(clicked()), SLOT(mapdeleteClicked()));

  mapalldelete= new QPushButton(tr("Delete all"), this);
  mapalldelete->setToolTip(tr("Clear list of selected maps") );
  connect( mapalldelete, SIGNAL(clicked()), SLOT(mapalldeleteClicked()));

  // ==================================================
  // selectedMapbox
  selectedMapbox= new QListWidget(this);

  connect( selectedMapbox, SIGNAL( itemClicked(QListWidgetItem *) ),
      SLOT( selectedMapboxClicked( QListWidgetItem*) ) );

  // --- Contourlines options ------------------------------
  QFrame* cont_frame= new QFrame(this);
  cont_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  cont_label= TitleLabel(tr("Contour lines"),cont_frame);

  // checkbox
  contours= new QCheckBox("", cont_frame);
  contours->setToolTip(tr("Draw contour lines (mandatory for maps without filled land)") );
  connect( contours, SIGNAL( toggled(bool) ),
      SLOT( cont_checkboxActivated(bool) ) );
  // linecbox
  cont_linelabel= new QLabel(tr("Line thickness"),cont_frame);
  cont_linecbox= LinewidthBox( cont_frame, false);
  connect( cont_linecbox, SIGNAL( activated(int) ),
      SLOT( cont_linecboxActivated(int) ) );
  // linetypecbox
  cont_linetypelabel= new QLabel( tr("Line type"), cont_frame );
  cont_linetypebox= LinetypeBox( cont_frame,false);
  connect( cont_linetypebox, SIGNAL( activated(int) ),
      SLOT( cont_linetypeboxActivated(int) ) );
  // colorcbox
  cont_colorlabel= new QLabel(tr("Colour"), cont_frame);
  cont_colorcbox = ColourBox( cont_frame, cInfo, false, 0 ); // last one index
  connect( cont_colorcbox, SIGNAL( activated(int) ),
      SLOT( cont_colorcboxActivated(int) ) );
  // zorder
  cont_zorderlabel= new QLabel(tr("Plot position"), cont_frame);
  cont_zorder= ComboBox( cont_frame, zorders, false, 0 );
  connect( cont_zorder, SIGNAL( activated(int) ),
      SLOT( cont_zordercboxActivated(int) ) );

  // --- Filled land options ------------------------------
  QFrame* land_frame= new QFrame(this);
  land_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  land_label= TitleLabel(tr("Filled land"),land_frame);

  // checkbox
  filledland= new QCheckBox("", land_frame);
  filledland->setToolTip(tr("Draw land with separate colour (only available for selected maps)") );
  connect( filledland, SIGNAL( toggled(bool) ),
      SLOT( land_checkboxActivated(bool) ) );
  // colorcbox
  land_colorlabel= new QLabel(tr("Colour"), land_frame);
  land_colorcbox = ColourBox( land_frame, cInfo, false, 0 ); // last one index
  connect( land_colorcbox, SIGNAL( activated(int) ),
      SLOT( land_colorcboxActivated(int) ) );
  // zorder
  land_zorderlabel= new QLabel(tr("Plot position"), land_frame);
  land_zorder= ComboBox( land_frame, zorders, false, 0 );
  connect( land_zorder, SIGNAL( activated(int) ),
      SLOT( land_zordercboxActivated(int) ) );

  // --- Latlon options ------------------------------
  lonb= false;
  lonc= "black";
  lonlw= "1";
  lonlt="solid";
  lonz=2;
  lond=10.0;
  lonshowvalue=false;
  int lon_line= 0;
  int lon_linetype= 0;
  int lon_col= 0;
  int lon_z= 2;
  int lon_dens= 0;

  latb= false;
  latc= "black";
  latlw= "1";
  latlt="solid";
  latz=2;
  latd=10.0;
  latshowvalue=false;
  int lat_line= 0;
  int lat_linetype= 0;
  int lat_col= 0;
  int lat_z= 2;
  int lat_dens= 0;

  // --- Frame options ------------------------------
  framec= "black";
  framelw= "1";
  int ff_line= 0;
  int ff_linetype= 0;
  int ff_col= 0;
  int ff_z= 2;

  int nm= m_MapDI.maps.size();
  if (nm>0) {
    nm--;

    lonb= m_MapDI.maps[nm].lon.ison;
    lonc= m_MapDI.maps[nm].lon.linecolour;
    lonlw= m_MapDI.maps[nm].lon.linewidth;
    lonlt= m_MapDI.maps[nm].lon.linetype;
    lond= m_MapDI.maps[nm].lon.density;
    lonz= m_MapDI.maps[nm].lon.zorder;
    lonshowvalue= m_MapDI.maps[nm].lon.showvalue;
    lon_line= atoi(lonlw.c_str())-1;
    lon_linetype= getIndex( linetypes, lonlt );
    lon_col= getIndex( cInfo, lonc );
    lon_z= lonz;
    lon_dens= 0;
    int dens= int(lond*1000);
    for (unsigned int k=0; k<densities.size(); k++) {
      if (int(atof(densities[k].c_str())*1000)==dens) {
        lon_dens= k;
        break;
      }
    }

    latb= m_MapDI.maps[nm].lat.ison;
    latc= m_MapDI.maps[nm].lat.linecolour;
    latlw= m_MapDI.maps[nm].lat.linewidth;
    latlt= m_MapDI.maps[nm].lat.linetype;
    latd= m_MapDI.maps[nm].lat.density;
    latz= m_MapDI.maps[nm].lat.zorder;
    latshowvalue= m_MapDI.maps[nm].lat.showvalue;
    lat_line= atoi(latlw.c_str())-1;
    lat_linetype= getIndex( linetypes, latlt );
    lat_col= getIndex( cInfo, latc );
    lat_z= latz;
    lat_dens= 0;
    dens= int(latd*1000);
    for (unsigned int k=0; k<densities.size(); k++) {
      if (int(atof(densities[k].c_str())*1000)==dens) {
        lat_dens= k;
        break;
      }
    }

    // frame
    framec= m_MapDI.maps[nm].frame.linecolour;
    framelw= m_MapDI.maps[nm].frame.linewidth;
    framelt= m_MapDI.maps[nm].frame.linetype;
    framez= m_MapDI.maps[nm].frame.zorder;
    ff_line= atoi(framelw.c_str())-1;
    ff_linetype= getIndex( linetypes, framelt );
    ff_col= getIndex( cInfo, framec );
    ff_z= framez;
  }

  // Show longitude lines

  QFrame* lon_frame= new QFrame(this);
  lon_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  lon_label= TitleLabel(tr("Longitude lines"),lon_frame);

  // checkbox
  showlon= new QCheckBox("", lon_frame);
  showlon->setToolTip(tr("Show longitude-lines on the map") );
  connect( showlon, SIGNAL( toggled(bool) ),
      SLOT( lon_checkboxActivated(bool) ) );
  // linecbox
  lon_linelabel= new QLabel(tr("Line thickness"),lon_frame);
  lon_linecbox= LinewidthBox( lon_frame, false,12,lon_line);
  //ComboBox( ll_frame, line, true, ll_line );
  connect( lon_linecbox, SIGNAL( activated(int) ),
      SLOT( lon_linecboxActivated(int) ) );
  // linetypebox
  lon_linetypelabel= new QLabel( tr("Line type"), lon_frame );
  lon_linetypebox= LinetypeBox( lon_frame,false,lon_linetype );
  connect( lon_linetypebox, SIGNAL( activated(int) ),
      SLOT( lon_linetypeboxActivated(int) ) );
  // colorcbox
  lon_colorlabel= new QLabel(tr("Colour"), lon_frame);
  lon_colorcbox = ColourBox( lon_frame, cInfo, false, 0 ); // last one index
  connect( lon_colorcbox, SIGNAL( activated(int) ),
      SLOT( lon_colorcboxActivated(int) ) );
  // density
  lon_densitylabel= new QLabel(tr("Density"), lon_frame);
  lon_density= ComboBox( lon_frame, densities, true, lon_dens );
  connect( lon_density, SIGNAL( activated(int) ),
      SLOT( lon_densitycboxActivated(int) ) );
  // zorder
  lon_zorderlabel= new QLabel(tr("Plot position"), lon_frame);
  lon_zorder= ComboBox( lon_frame, zorders, true, lon_z );
  connect( lon_zorder, SIGNAL( activated(int) ),
      SLOT( lon_zordercboxActivated(int) ) );
  // show value checkbox
  lon_showvalue= new QCheckBox(tr("Show value"), lon_frame);
  lon_showvalue->setToolTip(tr("Show longitude-values") );
  connect( lon_showvalue, SIGNAL( toggled(bool) ),SLOT( lon_showValueActivated(bool) ) );
  lon_showvalue->setChecked(lonshowvalue);
  // value pos
  lon_valuepos= ComboBox( lon_frame, positions_tr, true, 0 );
  showlon->setChecked(lonb);

  // Show latitude lines

  QFrame* lat_frame= new QFrame(this);
  lat_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  lat_label= TitleLabel(tr("Latitude lines"),lat_frame);

  // checkbox
  showlat= new QCheckBox("", lat_frame);
  showlat->setToolTip(tr("Show latitude-lines on the map") );
  connect( showlat, SIGNAL( toggled(bool) ),
      SLOT( lat_checkboxActivated(bool) ) );
  // linecbox
  lat_linelabel= new QLabel(tr("Line thickness"),lat_frame);
  lat_linecbox= LinewidthBox( lat_frame, false,12,lat_line);
  //ComboBox( ll_frame, line, true, ll_line );
  connect( lat_linecbox, SIGNAL( activated(int) ),
      SLOT( lat_linecboxActivated(int) ) );
  // linetypebox
  lat_linetypelabel= new QLabel( tr("Line type"), lat_frame );
  lat_linetypebox= LinetypeBox( lat_frame,false,lat_linetype );
  connect( lat_linetypebox, SIGNAL( activated(int) ),
      SLOT( lat_linetypeboxActivated(int) ) );
  // colorcbox
  lat_colorlabel= new QLabel(tr("Colour"), lat_frame);
  lat_colorcbox = ColourBox( lat_frame, cInfo, false, 0 ); // last one index
  connect( lat_colorcbox, SIGNAL( activated(int) ),
      SLOT( lat_colorcboxActivated(int) ) );
  // density
  lat_densitylabel= new QLabel(tr("Density"), lat_frame);
  lat_density= ComboBox( lat_frame, densities, true, lat_dens );
  connect( lat_density, SIGNAL( activated(int) ),
      SLOT( lat_densitycboxActivated(int) ) );
  // zorder
  lat_zorderlabel= new QLabel(tr("Plot position"), lat_frame);
  lat_zorder= ComboBox( lat_frame, zorders, true, lat_z );
  connect( lat_zorder, SIGNAL( activated(int) ),
      SLOT( lat_zordercboxActivated(int) ) );
  // show value checkbox
  lat_showvalue= new QCheckBox(tr("Show value"), lat_frame);
  lat_showvalue->setToolTip(tr("Show latitude-values") );
  connect( lat_showvalue, SIGNAL( toggled(bool) ),SLOT( lat_showValueActivated(bool) ) );
  lat_showvalue->setChecked(latshowvalue);
  // value pos
  lat_valuepos= ComboBox( lat_frame, positions_tr, true, 1);

  showlat->setChecked(latb);

  // frame on map...
  QFrame* ff_frame= new QFrame(this);
  ff_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // frame checkbox
  frameb= false;
  framelabel= TitleLabel(tr("Show frame"),ff_frame);
  showframe= new QCheckBox("", ff_frame);
  showframe->setToolTip(tr("Draw boundary of selected area") );
  connect( showframe, SIGNAL( toggled(bool) ),
      SLOT( showframe_checkboxActivated(bool) ) );

  // linecbox
  ff_linelabel= new QLabel(tr("Line thickness"),ff_frame);
  ff_linecbox= LinewidthBox( ff_frame, false,12,ff_line);
  //ComboBox( ff_frame, line, true, ff_line );
  connect( ff_linecbox, SIGNAL( activated(int) ),
      SLOT( ff_linecboxActivated(int) ) );
  // linetypecbox
  ff_linetypelabel= new QLabel( tr("Line type"), ff_frame );
  ff_linetypebox= LinetypeBox( ff_frame,false,ff_linetype );
  connect( ff_linetypebox, SIGNAL( activated(int) ),
      SLOT( ff_linetypeboxActivated(int) ) );
  // colorcbox
  ff_colorlabel= new QLabel(tr("Colour"), ff_frame);
  ff_colorcbox = ColourBox( ff_frame, cInfo, false, 0 ); // last one index
  connect( ff_colorcbox, SIGNAL( activated(int) ),
      SLOT( ff_colorcboxActivated(int) ) );
  // zorder
  ff_zorderlabel= new QLabel(tr("Plot position"), ff_frame);
  ff_zorder= ComboBox( ff_frame, zorders, true, ff_z );
  connect( ff_zorder, SIGNAL( activated(int) ),
      SLOT( ff_zordercboxActivated(int) ) );

  // Background colour
  backcolorlabel= TitleLabel( tr("Background colour"), this);
  backcolorcbox= ColourBox( this, cInfo, true, 0 ); // last one index
  backcolorcbox->setCurrentIndex(1);//Default colour white OBS
  connect( backcolorcbox, SIGNAL( activated(int) ),
      SLOT( backcolorcboxActivated(int) ) );

  // =============================================================

  // Buttons
  savefavorite = new QPushButton(tr("Save favorite"), this);
  usefavorite = new QPushButton(tr("Use favorite"), this);
  mapapply = new QPushButton(tr("Apply"), this);
  mapapplyhide = new QPushButton(tr("Apply+Hide"), this);
  maphide = new QPushButton(tr("Hide"), this);
  maphelp = new QPushButton(tr("Help"), this);

  mapapply->setDefault( true );
  savefavorite->setToolTip(tr("Save this layout as your favorite") );
  usefavorite->setToolTip(tr("Use saved favorite layout") );

  connect( savefavorite, SIGNAL(clicked()), SLOT(saveFavoriteClicked()));
  connect( usefavorite, SIGNAL(clicked()), SLOT(useFavoriteClicked()));
  connect( mapapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));
  connect( mapapply, SIGNAL(clicked()), SIGNAL(MapApply()));
  connect( maphide, SIGNAL(clicked()), SIGNAL(MapHide()));
  connect( maphelp, SIGNAL(clicked()),SLOT(helpClicked()));

  // ==================================================
  // Area / Projection
  // ==================================================
  QVBoxLayout* v1layout = new QVBoxLayout();
  v1layout->addWidget( arealabel );
  v1layout->addWidget( areabox );

  // ==================================================
  // Latlon-lines and frame
  // ==================================================
  QHBoxLayout* h2layout = new QHBoxLayout();
  QVBoxLayout * frame2layout = new QVBoxLayout();

  h2layout->addWidget(lon_frame);
  h2layout->addWidget(lat_frame);
  h2layout->addLayout(frame2layout);

  // Latlon layouts ---------
  QVBoxLayout* lonlayout= new QVBoxLayout(lon_frame);
  QHBoxLayout* lon_toplayout= new QHBoxLayout();
  lon_toplayout->addWidget(showlon);
  lon_toplayout->addWidget(lon_label);
  lon_toplayout->addStretch();

  QGridLayout* lon_glayout= new QGridLayout();
  lon_glayout->addWidget(lon_colorlabel,0,0);
  lon_glayout->addWidget(lon_colorcbox ,0,1);
  lon_glayout->addWidget(lon_linelabel ,1,0);
  lon_glayout->addWidget(lon_linecbox ,1,1);
  lon_glayout->addWidget(lon_linetypelabel ,2,0);
  lon_glayout->addWidget(lon_linetypebox ,2,1);
  lon_glayout->addWidget(lon_densitylabel,3,0);
  lon_glayout->addWidget(lon_density , 3,1);
  lon_glayout->addWidget(lon_zorderlabel, 4,0);
  lon_glayout->addWidget(lon_zorder ,4,1);
  lon_glayout->addWidget(lon_showvalue ,5,0);
  lon_glayout->addWidget(lon_valuepos ,5,1);

  lonlayout->addLayout(lon_toplayout);
  lonlayout->addLayout(lon_glayout);

  QVBoxLayout* latlayout= new QVBoxLayout(lat_frame);
  QHBoxLayout* lat_toplayout= new QHBoxLayout();
  lat_toplayout->addWidget(showlat);
  lat_toplayout->addWidget(lat_label);
  lat_toplayout->addStretch();

  QGridLayout* lat_glayout= new QGridLayout();
  lat_glayout->addWidget(lat_colorlabel,0,0);
  lat_glayout->addWidget(lat_colorcbox ,0,1);
  lat_glayout->addWidget(lat_linelabel ,1,0);
  lat_glayout->addWidget(lat_linecbox ,1,1);
  lat_glayout->addWidget(lat_linetypelabel ,2,0);
  lat_glayout->addWidget(lat_linetypebox ,2,1);
  lat_glayout->addWidget(lat_densitylabel,3,0);
  lat_glayout->addWidget(lat_density , 3,1);
  lat_glayout->addWidget(lat_zorderlabel, 4,0);
  lat_glayout->addWidget(lat_zorder ,4,1);
  lat_glayout->addWidget(lat_showvalue ,5,0);
  lat_glayout->addWidget(lat_valuepos ,5,1);

  latlayout->addLayout(lat_toplayout);
  latlayout->addLayout(lat_glayout);

  // frame layouts ---------
  QVBoxLayout* framelayout= new QVBoxLayout(ff_frame);
  QHBoxLayout* framehlayout= new QHBoxLayout();
  framehlayout->addWidget( showframe );
  framehlayout->addWidget(framelabel);
  framehlayout->addStretch();

  QGridLayout* frame_glayout= new QGridLayout();
  frame_glayout->addWidget(ff_colorlabel,0,0);
  frame_glayout->addWidget(ff_colorcbox ,0,1);
  frame_glayout->addWidget(ff_linelabel ,1,0);
  frame_glayout->addWidget(ff_linecbox ,1,1);
  frame_glayout->addWidget(ff_linetypelabel ,2,0);
  frame_glayout->addWidget(ff_linetypebox ,2,1);
  frame_glayout->addWidget(ff_zorderlabel, 3,0);
  frame_glayout->addWidget(ff_zorder ,3,1);

  framelayout->addLayout(framehlayout);
  framelayout->addLayout(frame_glayout);
  framelayout->addStretch( );

  // background layout
  QHBoxLayout* backlayout = new QHBoxLayout();
  backlayout->addWidget( backcolorlabel );
  backlayout->addWidget( backcolorcbox );
  backlayout->addStretch();

  frame2layout->addWidget(ff_frame);
  frame2layout->addLayout(backlayout);
  frame2layout->addStretch();

  // ==================================================
  // Maps, selected maps with buttons and options
  // ==================================================

  QHBoxLayout* h3layout = new QHBoxLayout();
  // maps layout
  QVBoxLayout* mapsvlayout = new QVBoxLayout();
  QVBoxLayout* smapsvlayout = new QVBoxLayout();
  mapsvlayout->addWidget( maplabel );
  mapsvlayout->addWidget( mapbox );
  smapsvlayout->addWidget( selectedMaplabel );
  smapsvlayout->addWidget( selectedMapbox );
  smapsvlayout->addWidget(mapdelete);
  smapsvlayout->addWidget(mapalldelete);

  h3layout->addLayout( mapsvlayout );
  h3layout->addLayout( smapsvlayout );

  // contours and filled maps
  QHBoxLayout* h5layout = new QHBoxLayout();

  // Contours layouts ---------
  QHBoxLayout* cont_h3layout = new QHBoxLayout();
  cont_h3layout->addWidget(cont_frame);

  QVBoxLayout* h3vlayout= new QVBoxLayout(cont_frame);
  QHBoxLayout* h3vhlayout= new QHBoxLayout();
  h3vhlayout->addWidget(contours);
  h3vhlayout->addWidget(cont_label);
  h3vhlayout->addStretch();

  QGridLayout* h3vglayout= new QGridLayout();
  h3vglayout->addWidget(cont_colorlabel,0,0);
  h3vglayout->addWidget(cont_colorcbox ,0,1);
  h3vglayout->addWidget(cont_linelabel ,1,0);
  h3vglayout->addWidget(cont_linecbox ,1,1);
  h3vglayout->addWidget(cont_linetypelabel ,2,0);
  h3vglayout->addWidget(cont_linetypebox ,2,1);
  h3vglayout->addWidget(cont_zorderlabel,3,0);
  h3vglayout->addWidget(cont_zorder ,3,1);

  h3vlayout->addLayout(h3vhlayout);
  h3vlayout->addLayout(h3vglayout);

  h5layout->addLayout( cont_h3layout );

  // Land layouts ---------
  QHBoxLayout* land_h3layout = new QHBoxLayout();
  land_h3layout->addWidget(land_frame);

  QVBoxLayout* land_h3vlayout= new QVBoxLayout(land_frame);
  QHBoxLayout* land_h3vhlayout= new QHBoxLayout();
  land_h3vhlayout->addWidget(filledland);
  land_h3vhlayout->addWidget(land_label);
  land_h3vhlayout->addStretch();

  QGridLayout* land_h3vglayout= new QGridLayout();
  land_h3vglayout->addWidget(land_colorlabel,0,0);
  land_h3vglayout->addWidget(land_colorcbox ,0,1);
  land_h3vglayout->addWidget(land_zorderlabel,1,0);
  land_h3vglayout->addWidget(land_zorder ,1,1);

  land_h3vlayout->addLayout(land_h3vhlayout);
  land_h3vlayout->addLayout(land_h3vglayout);
  land_h3vlayout->addStretch();

  h5layout->addLayout( land_h3layout );

  // buttons layout
  QGridLayout* buttonlayout= new QGridLayout();
  buttonlayout->addWidget( maphelp, 0, 0 );
  buttonlayout->addWidget( savefavorite,0, 1 );
  buttonlayout->addWidget( usefavorite, 0, 2 );

  buttonlayout->addWidget( maphide, 1, 0 );
  buttonlayout->addWidget( mapapplyhide, 1, 1 );
  buttonlayout->addWidget( mapapply, 1, 2 );

  // vlayout
  QVBoxLayout* vlayout = new QVBoxLayout( this );
  vlayout->addLayout( v1layout );
  vlayout->addLayout( h2layout );
  vlayout->addLayout( h3layout );
  //vlayout->addLayout( h4layout );
  vlayout->addLayout( h5layout );
  vlayout->addLayout( buttonlayout );
  //vlayout->activate();

  mapboxChanged();// this must be called after selected mapbox is created

  if (favorite.size()==0) saveFavoriteClicked(); //default favorite

}//end constructor MapDialog


// FRAME SLOTS

void MapDialog::showframe_checkboxActivated(bool on)
{
  frameb = on;

  if (on) {
    ff_linecbox->setEnabled(true);
    ff_linetypebox->setEnabled(true);
    ff_colorcbox->setEnabled(true);
    ff_zorder->setEnabled(true);
  } else {
    ff_linecbox->setEnabled(false);
    ff_linetypebox->setEnabled(false);
    ff_colorcbox->setEnabled(false);
    ff_zorder->setEnabled(false);
  }
}

void MapDialog::ff_linecboxActivated(int index)
{
  framelw = miutil::from_number(index + 1);
}

void MapDialog::ff_linetypeboxActivated(int index)
{
  framelt = linetypes[index];
}

void MapDialog::ff_colorcboxActivated(int index)
{
  framec = cInfo[index].name;
}

void MapDialog::ff_zordercboxActivated(int index)
{
  framez = index;
}

// MAP slots

void MapDialog::mapboxChanged()
{
#ifdef dMapDlg
  METLIBS_LOG_DEBUG("MapDialog::mapboxChanged called");
#endif

  int numselected = selectedmaps.size();
  int current = -1, j;
  // find current new map - if any
  if (numselected > 0) {
    for (int i = 0; i < numMaps; i++) {
      if (mapbox->item(i)->isSelected()) {
        for (j = 0; j < numselected; j++)
          if (selectedmaps[j] == i)
            break;
        if (j == numselected)
          current = i; // the new map
      }
    }
  }

  selectedmaps.clear();
  numselected = 0;
  int activeidx = -1;
  bool currmapok = false;

  // identify selected maps - keep index to current clicked or previous
  // selected map
  for (int i = 0; i < numMaps; i++) {
    if (mapbox->item(i)->isSelected()) {
      selectedmaps.push_back(i);
      if (current == i || (!currmapok && activemap == i)) {
        currmapok = true;
        activeidx = numselected;
      }
      numselected++;
    }
  }
  if (!currmapok) {
    if (numselected > 0) {
      activeidx = 0;
    }
  }

  // update listbox of selected maps - stop any signals
  selectedMapbox->blockSignals(true);
  selectedMapbox->clear();
  for (int i = 0; i < numselected; i++) {
    selectedMapbox->addItem(QString(m_MapDI.maps[selectedmaps[i]].name.c_str()));
  }
  selectedMapbox->blockSignals(false);

  if (numselected == 0) {// none is selected
    selectedMapbox->setEnabled(false);
    contours->setEnabled(false);
    cont_colorcbox->setEnabled(false);
    cont_linecbox->setEnabled(false);
    cont_linetypebox->setEnabled(false);
    cont_zorder->setEnabled(false);
    filledland->setEnabled(false);
    land_colorcbox->setEnabled(false);
    land_zorder->setEnabled(false);
  } else {
    // select the active map
    selectedMapbox->setCurrentRow(activeidx);
    selectedMapbox->setEnabled(true);
    selectedMapboxClicked(selectedMapbox->currentItem());
  }
}

void MapDialog::selectedMapboxClicked(QListWidgetItem* item)
{
  // new selection in list of selected maps
  // fill all option-widgets for map

  int index = selectedMapbox->currentRow();
  activemap = selectedmaps[index];

  bool island = (m_MapDI.maps[activemap].type == "triangles"
    || m_MapDI.maps[activemap].type == "shape");

  bool ison = m_MapDI.maps[activemap].contour.ison;
  int m_colIndex = getIndex(cInfo, m_MapDI.maps[activemap].contour.linecolour);
  int m_linewIndex = atoi(m_MapDI.maps[activemap].contour.linewidth.c_str()) - 1;
  int m_linetIndex = getIndex(linetypes,
      m_MapDI.maps[activemap].contour.linetype);
  int m_zIndex = m_MapDI.maps[activemap].contour.zorder;

  if (island)
    contours->setEnabled(true);
  else
    contours->setEnabled(false);
  cont_colorcbox->setEnabled(true);
  cont_linecbox->setEnabled(true);
  cont_linetypebox->setEnabled(true);
  cont_zorder->setEnabled(true);

  filledland->setEnabled(island);
  land_colorcbox->setEnabled(island);
  land_zorder->setEnabled(island);

  // set contours options
  contours->setChecked(ison);
  cont_colorcbox->setCurrentIndex(m_colIndex);
  cont_linecbox->setCurrentIndex(m_linewIndex);
  cont_linetypebox->setCurrentIndex(m_linetIndex);
  cont_zorder->setCurrentIndex(m_zIndex);

  // set filled land options
  ison = m_MapDI.maps[activemap].land.ison;
  m_colIndex = getIndex(cInfo, m_MapDI.maps[activemap].land.fillcolour);
  m_zIndex = m_MapDI.maps[activemap].land.zorder;

  filledland->setChecked(ison);
  land_colorcbox->setCurrentIndex(m_colIndex);
  land_zorder->setCurrentIndex(m_zIndex);
}

void MapDialog::mapdeleteClicked()
{

  if (activemap >= 0 && activemap < numMaps) {
    mapbox->item(activemap)->setSelected(false);
  }

  mapboxChanged();
}

void MapDialog::mapalldeleteClicked()
{
  // deselect all maps
  mapbox->clearSelection();

  mapboxChanged();
}

// CONTOUR SLOTS

void MapDialog::cont_checkboxActivated(bool on)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("checkboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.ison = on;

  if (on) {
    cont_linecbox->setEnabled(true);
    cont_linetypebox->setEnabled(true);
    cont_colorcbox->setEnabled(true);
    cont_zorder->setEnabled(true);
  } else {
    cont_linecbox->setEnabled(false);
    cont_linetypebox->setEnabled(false);
    cont_colorcbox->setEnabled(false);
    cont_zorder->setEnabled(false);
  }
}

void MapDialog::cont_linecboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("linecboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.linewidth = miutil::from_number(index + 1);
}

void MapDialog::cont_linetypeboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("linetypeboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.linetype = linetypes[index];
}

void MapDialog::cont_colorcboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("colorcboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.linecolour = cInfo[index].name;
}

void MapDialog::cont_zordercboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("zordercboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.zorder = index;
}

// LAND SLOTS


void MapDialog::land_checkboxActivated(bool on)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("checkboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].land.ison = on;

  if (on) {
    land_colorcbox->setEnabled(true);
    land_zorder->setEnabled(true);
  } else {
    land_colorcbox->setEnabled(false);
    land_zorder->setEnabled(false);
  }
}

void MapDialog::land_colorcboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("colorcboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].land.fillcolour = cInfo[index].name;
}

void MapDialog::land_zordercboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("zordercboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].land.zorder = index;
}

// LATLON SLOTS

void MapDialog::lon_checkboxActivated(bool on)
{
  lonb = on;

  lon_linecbox->setEnabled(on);
  lon_linetypebox->setEnabled(on);
  lon_colorcbox->setEnabled(on);
  lon_density->setEnabled(on);
  lon_zorder->setEnabled(on);
  lon_showvalue->setEnabled(on);
  lon_valuepos->setEnabled(on);
}

void MapDialog::lon_linecboxActivated(int index)
{
  lonlw = miutil::from_number(index + 1);
}

void MapDialog::lon_linetypeboxActivated(int index)
{
  lonlt = linetypes[index];
}

void MapDialog::lon_colorcboxActivated(int index)
{
  lonc = cInfo[index].name;
}

void MapDialog::lon_densitycboxActivated(int index)
{
  lond = atof(densities[index].c_str());
}

void MapDialog::lon_zordercboxActivated(int index)
{
  lonz = index;
}

void MapDialog::lon_showValueActivated(bool on)
{
  lonshowvalue = on;
}

void MapDialog::lat_checkboxActivated(bool on)
{
  latb = on;

  lat_linecbox->setEnabled(on);
  lat_linetypebox->setEnabled(on);
  lat_colorcbox->setEnabled(on);
  lat_density->setEnabled(on);
  lat_zorder->setEnabled(on);
  lat_showvalue->setEnabled(on);
  lat_valuepos->setEnabled(on);
}

void MapDialog::lat_linecboxActivated(int index)
{
  latlw = miutil::from_number(index + 1);
}

void MapDialog::lat_linetypeboxActivated(int index)
{
  latlt = linetypes[index];
}

void MapDialog::lat_colorcboxActivated(int index)
{
  latc = cInfo[index].name;
}

void MapDialog::lat_densitycboxActivated(int index)
{
  latd = atof(densities[index].c_str());
}

void MapDialog::lat_zordercboxActivated(int index)
{
  latz = index;
}

void MapDialog::lat_showValueActivated(bool on)
{
  latshowvalue = on;
}

// -----------------

void MapDialog::backcolorcboxActivated(int index)
{
#ifdef dMapDlg
  METLIBS_LOG_DEBUG("MapDialog::backcolorcboxActivated called");
#endif
}

/*
 BUTTONS
 */

void MapDialog::saveFavoriteClicked()
{
  // save current status as favorite
  favorite = getOKString();

  // enable this button
  usefavorite->setEnabled(true);
}

void MapDialog::useFavorite()
{
  /*
  METLIBS_LOG_DEBUG("useFavorite:");
  for (int i=0; i<favorite.size();i++)
    METLIBS_LOG_DEBUG(favorite[i]);
   */
  putOKString(favorite);
}

void MapDialog::useFavoriteClicked()
{
  useFavorite();
}

void MapDialog::applyhideClicked()
{
  emit MapApply();
  emit MapHide();
}

void MapDialog::helpClicked()
{
  emit showsource("ug_mapdialogue.html");
}

void MapDialog::closeEvent(QCloseEvent* e)
{
  emit MapHide();
}

// -------------------------------------------------------
// get and put OK strings
// -------------------------------------------------------


/*
 GetOKString
 */

vector<string> MapDialog::getOKString()
{
#ifdef dMapDlg
  METLIBS_LOG_DEBUG("MapDialog::getOKString called");
#endif
  vector<string> vstr;

//Area string
  ostringstream areastr;

  if (areabox->currentRow() > -1) {
    areastr << "AREA";
    areastr << " name=" << areabox->currentItem()->text().toStdString();
    vstr.push_back(areastr.str());
  }

  //Map strings
  int lindex;
  int numselected = selectedmaps.size();

  if (numselected == 0 && areabox->count()>0 ) { // no maps selected
    int backc = backcolorcbox->currentIndex();

    ostringstream ostr;
    ostr << "MAP"
    << " backcolour="
    << (backc >= 0 && backc < int(cInfo.size()) ? cInfo[backc].name : "white");

    // set latlon options
    MapInfo mi;
    mi.contour.ison = false;
    mi.land.ison = false;

    mi.lon.ison = lonb;
    mi.lon.linecolour = lonc;
    mi.lon.linewidth = lonlw;
    mi.lon.linetype = lonlt;
    mi.lon.zorder = lonz;
    mi.lon.density = lond;
    mi.lon.showvalue = lonshowvalue;
    if(lon_valuepos->currentIndex()>-1 && lon_valuepos->currentIndex()<int(positions.size()))
      mi.lon.value_pos = positions[lon_valuepos->currentIndex()];

    mi.lat.ison = latb;
    mi.lat.linecolour = latc;
    mi.lat.linewidth = latlw;
    mi.lat.linetype = latlt;
    mi.lat.zorder = latz;
    mi.lat.density = latd;
    mi.lat.showvalue = latshowvalue;
    if(lat_valuepos->currentIndex()>-1 && lat_valuepos->currentIndex()<int(positions.size()))
      mi.lat.value_pos = positions[lat_valuepos->currentIndex()];

    mi.frame.ison = frameb;
    mi.frame.linecolour = framec;
    mi.frame.linewidth = framelw;
    mi.frame.linetype = framelt;

    std::string mstr;
    m_ctrl->MapInfoParser(mstr, mi, true);
    ostr << " " << mstr;

    vstr.push_back(ostr.str());

    // clear log-list
    logmaps.clear();

  } else {

    vector<int> lmaps; // check for logging

    for (int i = 0; i < numselected; i++) {
      lindex = selectedmaps[i];
      // check if ok to log
      if (m_MapDI.maps[lindex].logok)
        lmaps.push_back(lindex);
      ostringstream ostr;
      ostr << "MAP";
      if (i == numselected - 1) { // common options for last map only
        int backc = backcolorcbox->currentIndex();
        ostr << " backcolour="
        << (backc >= 0 && backc < int(cInfo.size()) ? cInfo[backc].name
            : "white");

        // set latlon options
        m_MapDI.maps[lindex].lon.ison = lonb;
        m_MapDI.maps[lindex].lon.linecolour = lonc;
        m_MapDI.maps[lindex].lon.linewidth = lonlw;
        m_MapDI.maps[lindex].lon.linetype = lonlt;
        m_MapDI.maps[lindex].lon.zorder = lonz;
        m_MapDI.maps[lindex].lon.density = lond;
        m_MapDI.maps[lindex].lon.showvalue = lonshowvalue;
        if(lon_valuepos->currentIndex()>-1 && lon_valuepos->currentIndex()<int(positions.size()))
        m_MapDI.maps[lindex].lon.value_pos = positions[lon_valuepos->currentIndex()];

        m_MapDI.maps[lindex].lat.ison = latb;
        m_MapDI.maps[lindex].lat.linecolour = latc;
        m_MapDI.maps[lindex].lat.linewidth = latlw;
        m_MapDI.maps[lindex].lat.linetype = latlt;
        m_MapDI.maps[lindex].lat.zorder = latz;
        m_MapDI.maps[lindex].lat.density = latd;
        m_MapDI.maps[lindex].lat.showvalue = latshowvalue;
        if(lat_valuepos->currentIndex()>-1 && lat_valuepos->currentIndex()<int(positions.size()))
          m_MapDI.maps[lindex].lat.value_pos = positions[lat_valuepos->currentIndex()];


        // set frame options
        m_MapDI.maps[lindex].frame.ison = frameb;
        m_MapDI.maps[lindex].frame.linecolour = framec;
        m_MapDI.maps[lindex].frame.linewidth = framelw;
        m_MapDI.maps[lindex].frame.linetype = framelt;

      } else {
        m_MapDI.maps[lindex].lon.ison = false;
        m_MapDI.maps[lindex].lat.ison = false;
        m_MapDI.maps[lindex].frame.ison = false;
      }

      std::string mstr;
      m_ctrl->MapInfoParser(mstr, m_MapDI.maps[lindex], true);
      ostr << " " << mstr;

      vstr.push_back(ostr.str());
    }
    if (lmaps.size() > 0)
      logmaps = lmaps;
  }

  return vstr;
}

/*
 PutOKString
 */

void MapDialog::putOKString(const vector<string>& vstr)
{
  int n = vstr.size();
  vector<int> themaps;
  vector<std::string> tokens, stokens;
  std::string bgcolour, area;
  int lastmap = -1;

  int iline;
  for (iline = 0; iline < n; iline++) {
    std::string str = vstr[iline];
    miutil::trim(str);
    if (str.empty())
      continue;
    if (str[0] == '#')
      continue;
    std::string themap = "";
    tokens = miutil::split(str, " ");
    int m = tokens.size();
    if (m > 0 && (miutil::to_upper(tokens[0]) != "MAP" && miutil::to_upper(tokens[0]) != "AREA"))
      continue;
    for (int j = 0; j < m; j++) {
      stokens = miutil::split(tokens[j], 0, "=");
      if (stokens.size() == 2) {
        if (miutil::to_upper(stokens[0]) == "BACKCOLOUR")
          bgcolour = stokens[1];
        else if (miutil::to_upper(stokens[0]) == "NAME" || miutil::to_upper(stokens[0]) == "AREANAME" || miutil::to_upper(stokens[0]) == "AREA")
          area = stokens[1];
        else if (miutil::to_upper(stokens[0]) == "MAP")
          themap = stokens[1];
      }
    }

    // find the logged map in full list
    if (not themap.empty()) {
      int idx = -1;
      for (int k = 0; k < numMaps; k++)
        if (m_MapDI.maps[k].name == themap) {
          idx = k;
          lastmap = idx;
          themaps.push_back(k); // keep list of selected maps
          break;
        }
      // update options
      if (idx >= 0) {
        m_ctrl->MapInfoParser(str, m_MapDI.maps[idx], false);
      }
    }
  }

  // reselect area
  int area_index = 0;
  for (unsigned int i = 0; i < m_MapDI.areas.size(); i++)
    if (m_MapDI.areas[i] == area) {
      area_index = i;
      break;
    }
  areabox->setCurrentRow(area_index);

  // deselect all maps
  mapbox->clearSelection();

  int nm = themaps.size();
  // reselect maps
  for (int i = 0; i < nm; i++) {
    mapbox->item(themaps[i])->setSelected(true);
  }

  if (lastmap >= 0 && lastmap <int(m_MapDI.maps.size())) {
    // set latlon options
    lonb = m_MapDI.maps[lastmap].lon.ison;
    lonc = m_MapDI.maps[lastmap].lon.linecolour;
    lonlw = m_MapDI.maps[lastmap].lon.linewidth;
    lonlt = m_MapDI.maps[lastmap].lon.linetype;
    lonz = m_MapDI.maps[lastmap].lon.zorder;
    lond = m_MapDI.maps[lastmap].lon.density;
    lonshowvalue = m_MapDI.maps[lastmap].lon.showvalue;

    int m_colIndex = getIndex(cInfo, lonc);
    int m_linewIndex = atoi(lonlw.c_str()) - 1;
    int m_linetIndex = getIndex(linetypes, lonlt);
    // find density index
    int m_densIndex = 0;
    int dens = int(lond * 1000);
    for (unsigned int k = 0; k < densities.size(); k++)
      if (int(atof(densities[k].c_str()) * 1000) == dens) {
        m_densIndex = k;
        break;
      }
    int m_zIndex = lonz;
    showlon->setChecked(lonb);
    if (m_colIndex >= 0)
      lon_colorcbox->setCurrentIndex(m_colIndex);
    if (m_linewIndex >= 0)
      lon_linecbox->setCurrentIndex(m_linewIndex);
    if (m_linetIndex >= 0)
      lon_linetypebox->setCurrentIndex(m_linetIndex);
    if (m_densIndex >= 0)
      lon_density->setCurrentIndex(m_densIndex);
    if (m_zIndex >= 0)
      lon_zorder->setCurrentIndex(m_zIndex);
    lon_showvalue->setChecked(lonshowvalue);
    if (positions_map.count(m_MapDI.maps[lastmap].lon.value_pos)) {
      lon_valuepos->setCurrentIndex(positions_map[m_MapDI.maps[lastmap].lon.value_pos]);
    }
    latb = m_MapDI.maps[lastmap].lat.ison;
    latc = m_MapDI.maps[lastmap].lat.linecolour;
    latlw = m_MapDI.maps[lastmap].lat.linewidth;
    latlt = m_MapDI.maps[lastmap].lat.linetype;
    latz = m_MapDI.maps[lastmap].lat.zorder;
    latd = m_MapDI.maps[lastmap].lat.density;
    latshowvalue = m_MapDI.maps[lastmap].lat.showvalue;

    m_colIndex = getIndex(cInfo, latc);
    m_linewIndex = atoi(latlw.c_str()) - 1;
    m_linetIndex = getIndex(linetypes, latlt);
    // find density index
    m_densIndex = 0;
    dens = int(latd * 1000);
    for (unsigned int k = 0; k < densities.size(); k++)
      if (int(atof(densities[k].c_str()) * 1000) == dens) {
        m_densIndex = k;
        break;
      }
    m_zIndex = latz;
    showlat->setChecked(latb);
    if (m_colIndex >= 0)
      lat_colorcbox->setCurrentIndex(m_colIndex);
    if (m_linewIndex >= 0)
      lat_linecbox->setCurrentIndex(m_linewIndex);
    if (m_linetIndex >= 0)
      lat_linetypebox->setCurrentIndex(m_linetIndex);
    if (m_densIndex >= 0)
      lat_density->setCurrentIndex(m_densIndex);
    if (m_zIndex >= 0)
      lat_zorder->setCurrentIndex(m_zIndex);
    lat_showvalue->setChecked(latshowvalue);
    if (positions_map.count(m_MapDI.maps[lastmap].lat.value_pos)) {
      lat_valuepos->setCurrentIndex(positions_map[m_MapDI.maps[lastmap].lat.value_pos]);
    }

    // set frame options
    frameb = m_MapDI.maps[lastmap].frame.ison;
    framec = m_MapDI.maps[lastmap].frame.linecolour;
    framelw = m_MapDI.maps[lastmap].frame.linewidth;
    framelt = m_MapDI.maps[lastmap].frame.linetype;
    framez = m_MapDI.maps[lastmap].frame.zorder;

    m_colIndex = getIndex(cInfo, framec);
    m_linewIndex = atoi(framelw.c_str()) - 1;
    m_linetIndex = getIndex(linetypes, framelt);
    m_zIndex = framez;

    showframe->setChecked(frameb);
    if (m_colIndex >= 0)
      ff_colorcbox->setCurrentIndex(m_colIndex);
    if (m_linewIndex >= 0)
      ff_linecbox->setCurrentIndex(m_linewIndex);
    if (m_linetIndex >= 0)
      ff_linetypebox->setCurrentIndex(m_linetIndex);
    if (m_zIndex >= 0)
      ff_zorder->setCurrentIndex(m_zIndex);
  }

  // set background
  int m_colIndex = getIndex(cInfo, bgcolour);
  if (m_colIndex >= 0)
    backcolorcbox->setCurrentIndex(m_colIndex);

}

std::string MapDialog::getShortname()
{
  std::string name;

  if ( areabox->count()== 0 ) {
    return name;
  }

  name = areabox->item(areabox->currentRow())->text().toStdString() + std::string(
      " ");

  int numselected = selectedmaps.size();
  for (int i = 0; i < numselected; i++) {
    int lindex = selectedmaps[i];
    name += m_MapDI.maps[lindex].name + std::string(" ");
  }

  name = "<font color=\"#009900\">" + name + "</font>";
  return name;
}

// ------------------------------------------------
// LOG-FILE read/write methods
// ------------------------------------------------

vector<string> MapDialog::writeLog()
{
  vector<string> vstr;

  // first: write all map-specifications
  int n = m_MapDI.maps.size();
  for (int i = 0; i < n; i++) {
    ostringstream ostr;
    if (i == n - 1) { // common options for last map only
      // qt4 fix: added .toStdString()
      if ( areabox->currentRow() >= 0 ) {
        ostr << "area="
            << areabox->item(areabox->currentRow())->text().toStdString()
            << " backcolour=" << cInfo[backcolorcbox->currentIndex()].name << " ";
      }
      // set lon options
      m_MapDI.maps[i].lon.ison = lonb;
      m_MapDI.maps[i].lon.linecolour = lonc;
      m_MapDI.maps[i].lon.linewidth = lonlw;
      m_MapDI.maps[i].lon.linetype = lonlt;
      m_MapDI.maps[i].lon.zorder = lonz;
      m_MapDI.maps[i].lon.density = lond;
      m_MapDI.maps[i].lon.showvalue = lonshowvalue;
      if(lon_valuepos->currentIndex()>-1 && lon_valuepos->currentIndex()<int(positions.size())){
        m_MapDI.maps[i].lon.value_pos = positions[lon_valuepos->currentIndex()];
      }

      m_MapDI.maps[i].lat.ison = latb;
      m_MapDI.maps[i].lat.linecolour = latc;
      m_MapDI.maps[i].lat.linewidth = latlw;
      m_MapDI.maps[i].lat.linetype = latlt;
      m_MapDI.maps[i].lat.zorder = latz;
      m_MapDI.maps[i].lat.density = latd;
      m_MapDI.maps[i].lat.showvalue = latshowvalue;
      if(lat_valuepos->currentIndex()>-1 && lat_valuepos->currentIndex()<int(positions.size())){
        m_MapDI.maps[i].lat.value_pos = positions[lat_valuepos->currentIndex()];
      }


      // set frame options
      m_MapDI.maps[i].frame.ison = frameb;
      m_MapDI.maps[i].frame.linecolour = framec;
      m_MapDI.maps[i].frame.linewidth = framelw;
      m_MapDI.maps[i].frame.linetype = framelt;
    } else {
      m_MapDI.maps[i].lon.ison = false;
      m_MapDI.maps[i].lat.ison = false;
      m_MapDI.maps[i].frame.ison = false;
    }

    std::string mstr;
    m_ctrl->MapInfoParser(mstr, m_MapDI.maps[i], true);
    ostr << mstr;

    vstr.push_back(ostr.str());
    vstr.push_back("-------------------");
  }

  // end of complete map-list
  vstr.push_back("=================== Selected maps =========");

  // write name of current selected (and legal) maps
  int lindex;
  int numselected = logmaps.size();
  for (int i = 0; i < numselected; i++) {
    lindex = logmaps[i];
    vstr.push_back(m_MapDI.maps[lindex].name);
  }

  vstr.push_back("=================== Favorites ============");
  // write favorite options
  for (unsigned int i = 0; i < favorite.size(); i++) {
    vstr.push_back(favorite[i]);
  }

  vstr.push_back("===========================================");

  return vstr;
}

void MapDialog::readLog(const vector<string>& vstr,
    const string& thisVersion, const string& logVersion)
{
  // version-check
  //bool oldversion= (thisVersion!=logVersion && logVersion < "2001-08-25");

  int n = vstr.size();
  vector<int> themaps;
  vector<std::string> tokens, stokens;
  std::string bgcolour, area;
  int lastmap = -1;

  int iline;
  for (iline = 0; iline < n; iline++) {
    std::string str = vstr[iline];
    miutil::trim(str);
    if (not str.empty() && str[0] == '-')
      continue;
    if (not str.empty() && str[0] == '=')
      break;
    std::string themap = "";
    tokens = miutil::split(str, " ");
    int m = tokens.size();
    for (int j = 0; j < m; j++) {
      stokens = miutil::split(tokens[j], 0, "=");
      if (stokens.size() == 2) {
        if (miutil::to_upper(stokens[0]) == "BACKCOLOUR")
          bgcolour = stokens[1];
        else if (miutil::to_upper(stokens[0]) == "AREA")
          area = stokens[1];
        else if (miutil::to_upper(stokens[0]) == "MAP")
          themap = stokens[1];
      }
    }
    // find the logged map in full list
    if (not themap.empty()) {
      int idx = -1;
      for (int k = 0; k < numMaps; k++)
        if (m_MapDI.maps[k].name == themap) {
          idx = k;
          lastmap = idx;
          //	  themaps.push_back(k); // just for old log-files
          break;
        }
      // update options
      if (idx >= 0)
        m_ctrl->MapInfoParser(str, m_MapDI.maps[idx], false);
    }
  }

  // read previous selected maps
  if (iline < n && vstr[iline][0] == '=') {
    themaps.clear();
    iline++;
    for (; iline < n; iline++) {
      std::string themap = vstr[iline];
      miutil::trim(themap);
      if (themap.empty())
        continue;
      if (themap[0] == '=')
        break;
      for (int k = 0; k < numMaps; k++)
        if (m_MapDI.maps[k].name == themap) {
          themaps.push_back(k);
          break;
        }
    }
  }

  // read favorite
  favorite.clear();
  if (iline < n && vstr[iline][0] == '=') {
    iline++;
    for (; iline < n; iline++) {
      std::string str = vstr[iline];
      miutil::trim(str);
      if (str.empty())
        continue;
      if (str[0] == '=')
        break;
      favorite.push_back(str);
    }
  }
  usefavorite->setEnabled(favorite.size() > 0);

  // reselect area
  int area_index = 0;
  for (unsigned int i = 0; i < m_MapDI.areas.size(); i++)
    if (m_MapDI.areas[i] == area) {
      area_index = i;
      break;
    }
  areabox->setCurrentRow(area_index);

  // deselect all maps
  mapbox->clearSelection();

  int nm = themaps.size();
  // reselect maps
  for (int i = 0; i < nm; i++) {
    mapbox->item(themaps[i])->setSelected(true);
  }
  // keep for logging
  logmaps = themaps;

  if (lastmap > 0) {
    // set lon options
    lonb = m_MapDI.maps[lastmap].lon.ison;
    lonc = m_MapDI.maps[lastmap].lon.linecolour;
    lonlw = m_MapDI.maps[lastmap].lon.linewidth;
    lonlt = m_MapDI.maps[lastmap].lon.linetype;
    lonz = m_MapDI.maps[lastmap].lon.zorder;
    lond = m_MapDI.maps[lastmap].lon.density;
    lonshowvalue = m_MapDI.maps[lastmap].lon.showvalue;

    int m_colIndex = getIndex(cInfo, lonc);
    int m_linewIndex = atoi(lonlw.c_str()) - 1;
    int m_linetIndex = getIndex(linetypes, lonlt);
    // find density index
    int m_densIndex = 0;
    int dens = int(lond * 1000);
    for (unsigned int k = 0; k < densities.size(); k++)
      if (int(atof(densities[k].c_str()) * 1000) == dens) {
        m_densIndex = k;
        break;
      }
    int m_zIndex = lonz;
    showlon->setChecked(lonb);
    if (m_colIndex >= 0)
      lon_colorcbox->setCurrentIndex(m_colIndex);
    if (m_linewIndex >= 0)
      lon_linecbox->setCurrentIndex(m_linewIndex);
    if (m_linetIndex >= 0)
      lon_linetypebox->setCurrentIndex(m_linetIndex);
    if (m_densIndex >= 0)
      lon_density->setCurrentIndex(m_densIndex);
    if (m_zIndex >= 0)
      lon_zorder->setCurrentIndex(m_zIndex);
    lon_showvalue->setChecked(lonshowvalue);
    if (positions_map.count(m_MapDI.maps[lastmap].lon.value_pos)) {
      lon_valuepos->setCurrentIndex(positions_map[m_MapDI.maps[lastmap].lon.value_pos]);
    }
    // set lat options
    latb = m_MapDI.maps[lastmap].lat.ison;
    latc = m_MapDI.maps[lastmap].lat.linecolour;
    latlw = m_MapDI.maps[lastmap].lat.linewidth;
    latlt = m_MapDI.maps[lastmap].lat.linetype;
    latz = m_MapDI.maps[lastmap].lat.zorder;
    latd = m_MapDI.maps[lastmap].lat.density;
    latshowvalue = m_MapDI.maps[lastmap].lat.showvalue;

    m_colIndex = getIndex(cInfo, latc);
    m_linewIndex = atoi(latlw.c_str()) - 1;
    m_linetIndex = getIndex(linetypes, latlt);
    // find density index
    m_densIndex = 0;
    dens = int(latd * 1000);
    for (unsigned int k = 0; k < densities.size(); k++)
      if (int(atof(densities[k].c_str()) * 1000) == dens) {
        m_densIndex = k;
        break;
      }
    m_zIndex = latz;
    showlat->setChecked(latb);
    if (m_colIndex >= 0)
      lat_colorcbox->setCurrentIndex(m_colIndex);
    if (m_linewIndex >= 0)
      lat_linecbox->setCurrentIndex(m_linewIndex);
    if (m_linetIndex >= 0)
      lat_linetypebox->setCurrentIndex(m_linetIndex);
    if (m_densIndex >= 0)
      lat_density->setCurrentIndex(m_densIndex);
    if (m_zIndex >= 0)
      lat_zorder->setCurrentIndex(m_zIndex);
    lat_showvalue->setChecked(latshowvalue);
//    if (latvaluepos >= 0)
    if (positions_map.count(m_MapDI.maps[lastmap].lat.value_pos)) {
      lat_valuepos->setCurrentIndex(positions_map[m_MapDI.maps[lastmap].lat.value_pos]);
    }

    // set frame options
    frameb = m_MapDI.maps[lastmap].frame.ison;
    framec = m_MapDI.maps[lastmap].frame.linecolour;
    framelw = m_MapDI.maps[lastmap].frame.linewidth;
    framelt = m_MapDI.maps[lastmap].frame.linetype;
    framez = m_MapDI.maps[lastmap].frame.zorder;

    m_colIndex = getIndex(cInfo, framec);
    m_linewIndex = atoi(framelw.c_str()) - 1;
    m_linetIndex = getIndex(linetypes, framelt);
    m_zIndex = lonz;

    showframe->setChecked(frameb);
    if (m_colIndex >= 0)
      ff_colorcbox->setCurrentIndex(m_colIndex);
    if (m_linewIndex >= 0)
      ff_linecbox->setCurrentIndex(m_linewIndex);
    if (m_linetIndex >= 0)
      ff_linetypebox->setCurrentIndex(m_linetIndex);
    if (m_zIndex >= 0)
      ff_zorder->setCurrentIndex(m_zIndex);
  }

  // set background
  int m_colIndex = getIndex(cInfo, bgcolour);
  if (m_colIndex >= 0)
    backcolorcbox->setCurrentIndex(m_colIndex);
}

