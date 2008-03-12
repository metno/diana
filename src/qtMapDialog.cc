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

#include <qtMapDialog.h>
#include <qtUtility.h>
#include <qtToggleButton.h>

#include <miString.h>
#include <stdio.h>
#include <iostream>

#define HEIGHTLB 105
#define HEIGHTLBSMALL 70
#define HEIGHTLBXSMALL 50


/*********************************************/
MapDialog::MapDialog( QWidget* parent, Controller* llctrl )
  : QDialog(parent)
{
#ifdef dMapDlg 
  cerr<<"MapDialog::MapDialog called"<<endl;
#endif
  m_ctrl=llctrl;
  ConstructorCernel( llctrl->initMapDialog() );
}


/*********************************************/
MapDialog::MapDialog(  QWidget* parent, const MapDialogInfo& mdi )
  : QDialog(parent)
{
#ifdef dMapDlg 
  cerr<<"MapDialog::MapDialog called"<<endl;
#endif
  ConstructorCernel( mdi );
}


/*********************************************/
void MapDialog::ConstructorCernel( const MapDialogInfo mdi ){
#ifdef dMapDlg 
  cerr<<"MapDialog::ConstructorCernel called"<<endl;
#endif
  
  setCaption(tr("Map and Area"));
    
  // all defined maps etc.
  m_MapDI=mdi;
 
  numMaps= m_MapDI.maps.size();
  activemap= -1;

  int i;

  // linetypes
  linetypes = Linetype::getLinetypeNames();

  // colour 
  cInfo = Colour::getColourInfo();

  // zorders
  zorders.push_back(tr("lowest").latin1());  // nederst
  zorders.push_back(tr("auto").latin1());    // auto
  zorders.push_back(tr("highest").latin1()); // øverst

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

  // ==================================================
  // arealabel
  arealabel= TitleLabel( tr("Area/Projection"), this);

  // areabox
  areabox= new QListWidget(this);
  areabox->setMinimumHeight(HEIGHTLBSMALL);
  areabox->setMaximumHeight(HEIGHTLB);

  int nr_area = m_MapDI.areas.size();
  int area_defIndex=0;
  for( int i=0; i<nr_area; i++ ){
    areabox->addItem(QString(m_MapDI.areas[i].c_str()));
    if( m_MapDI.areas[i]==m_MapDI.default_area )
      area_defIndex=i;
  }
  // select default area
  areabox->setCurrentRow(area_defIndex);

  // ==================================================
  // maplabel
  maplabel= TitleLabel( tr("Maps"), this);
  // selectedMaplabel
  selectedMaplabel= TitleLabel( tr("Selected maps"), this);

  // mapbox
  mapbox= new QListWidget(this);
  mapbox->setMinimumHeight(HEIGHTLBSMALL);
  mapbox->setMaximumHeight(HEIGHTLB);
	   
  for( int i=0; i<numMaps; i++ ){
    mapbox->addItem(QString(m_MapDI.maps[i].name.c_str()));
  }

  mapbox->setSelectionMode(QAbstractItemView::MultiSelection);

  // select default maps
  for( i=0; i<numMaps; i++ ){
    for( int k=0; k<m_MapDI.default_maps.size(); k++ ){
      if( m_MapDI.default_maps[k]==m_MapDI.maps[i].name ){
	mapbox->item(i)->setSelected(true);
      }
    }
  }

  connect( mapbox, SIGNAL(itemSelectionChanged () ),
	   SLOT( mapboxChanged() ) );
   
  // ==================================================
  // delete buttons
  mapdelete= new QPushButton(tr("Delete"), this);
  QToolTip::add( mapdelete,
		 tr("Remove selected map from the list \"Selected maps\"") );
  connect( mapdelete, SIGNAL(clicked()), SLOT(mapdeleteClicked()));

  mapalldelete= new QPushButton(tr("Delete all"), this);
  QToolTip::add( mapalldelete,
		 tr("Clear list of selected maps") );
  connect( mapalldelete, SIGNAL(clicked()), SLOT(mapalldeleteClicked()));


  // ==================================================
  // selectedMapbox
  selectedMapbox= new QListWidget(this);
  selectedMapbox->setMinimumHeight(HEIGHTLBXSMALL);
  selectedMapbox->setMaximumHeight(HEIGHTLBSMALL); // 50

  connect( selectedMapbox, SIGNAL( itemClicked(QListWidgetItem *)  ), 
	   SLOT( selectedMapboxClicked( QListWidgetItem*) ) );


  int m_backIndex = getIndex( cInfo, m_MapDI.backcolour );

  // ==================================================
  // --- Contourlines options ------------------------------
  QFrame* cont_frame= new QFrame(this,"cont_frame");
  cont_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  cont_label= TitleLabel(tr("Contour lines"),cont_frame);

  // checkbox
  contours= new QCheckBox("", cont_frame, "contours");
  QToolTip::add( contours,
		 tr("Draw contour lines (mandatory for maps without filled land)") );
  connect( contours, SIGNAL( toggled(bool) ), 
	   SLOT( cont_checkboxActivated(bool) ) );
  // linecbox
  cont_linelabel= new QLabel(tr("Line thickness"),cont_frame);
  cont_linecbox= LinewidthBox( cont_frame, false);
  connect( cont_linecbox, SIGNAL( activated(int) ), 
	   SLOT( cont_linecboxActivated(int) ) );
  // linetypecbox
  cont_linetypelabel= new QLabel( tr("Line type"), cont_frame );
  cont_linetypebox=  LinetypeBox( cont_frame,false);
  connect( cont_linetypebox, SIGNAL( activated(int) ), 
	   SLOT( cont_linetypeboxActivated(int) ) );
  // colorcbox
  cont_colorlabel= new QLabel(tr("Colour"), cont_frame);
  cont_colorcbox = ColourBox( cont_frame, cInfo, false, 0 );  // last one index
  connect( cont_colorcbox, SIGNAL( activated(int) ), 
	   SLOT( cont_colorcboxActivated(int) ) );
  // zorder
  cont_zorderlabel= new QLabel(tr("Plot position"), cont_frame);
  cont_zorder= ComboBox( cont_frame, zorders, false, 0 );
  connect( cont_zorder, SIGNAL( activated(int) ), 
	   SLOT( cont_zordercboxActivated(int) ) );

  // ==================================================
  // --- Filled land options ------------------------------
  QFrame* land_frame= new QFrame(this,"land_frame");
  land_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  land_label= TitleLabel(tr("Filled land"),land_frame);

  // checkbox
  filledland= new QCheckBox("", land_frame, "land");
  QToolTip::add( filledland,
		 tr("Draw land with separate colour (only available for selected maps)") );
  connect( filledland, SIGNAL( toggled(bool) ), 
	   SLOT( land_checkboxActivated(bool) ) );
  // colorcbox
  land_colorlabel= new QLabel(tr("Colour"), land_frame);
  land_colorcbox = ColourBox( land_frame, cInfo, false, 0 );  // last one index
  connect( land_colorcbox, SIGNAL( activated(int) ), 
	   SLOT( land_colorcboxActivated(int) ) );
  // zorder
  land_zorderlabel= new QLabel(tr("Plot position"), land_frame);
  land_zorder= ComboBox( land_frame, zorders, false, 0 );
  connect( land_zorder, SIGNAL( activated(int) ), 
	   SLOT( land_zordercboxActivated(int) ) );

  // ==================================================
  // --- Latlon options ------------------------------
  latlonb= false;
  latlonc= "black";
  latlonlw= "1";
  latlonlt="solid";
  latlonz=2;
  latlond=10.0;
  int ll_line= 0;
  int ll_linetype= 0;
  int ll_col= 0;
  int ll_z= 2;
  int ll_dens= 0;

  // --- Frame options ------------------------------
  framec= "black";
  framelw= "1";
  int ff_line= 0;
  int ff_linetype= 0;
  int ff_col= 0;
  int ff_z= 2;

  int nm= m_MapDI.maps.size();
  if (nm>0){
    nm--;
    latlonb=  m_MapDI.maps[nm].latlon.ison;
    latlonc=  m_MapDI.maps[nm].latlon.linecolour;
    latlonlw= m_MapDI.maps[nm].latlon.linewidth;
    latlonlt= m_MapDI.maps[nm].latlon.linetype;
    latlond=  m_MapDI.maps[nm].latlon.density;
    latlonz=  m_MapDI.maps[nm].latlon.zorder;
    ll_line= atoi(latlonlw.cStr())-1;
    ll_linetype= getIndex( linetypes, latlonlt );
    ll_col= getIndex( cInfo, latlonc );
    ll_z= latlonz;
    ll_dens= 0;
    int dens= int(latlond*1000);
    for (int k=0; k<densities.size(); k++){
      if (int(atof(densities[k].c_str())*1000)==dens){
	ll_dens= k;
	break;
      }
    }
    // frame
    framec=  m_MapDI.maps[nm].frame.linecolour;
    framelw= m_MapDI.maps[nm].frame.linewidth;
    framelt= m_MapDI.maps[nm].frame.linetype;
    framez=  m_MapDI.maps[nm].frame.zorder;
		ff_line= atoi(framelw.cStr())-1;
    ff_linetype= getIndex( linetypes, framelt );
    ff_col= getIndex( cInfo, framec );
    ff_z= framez;
  }

  QFrame* ll_frame= new QFrame(this,"ll_frame");
  ll_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // label
  ll_label= TitleLabel(tr("Lat/lon lines"),ll_frame);

  // checkbox
  latlon= new QCheckBox("", ll_frame, "llours");
  QToolTip::add( latlon,
		 tr("Show lat/lon-lines on the map") );
  connect( latlon, SIGNAL( toggled(bool) ), 
	   SLOT( ll_checkboxActivated(bool) ) );
  // linecbox
  ll_linelabel= new QLabel(tr("Line thickness"),ll_frame);
  ll_linecbox= LinewidthBox( ll_frame, false,12,ll_line);
  //ComboBox( ll_frame, line, true, ll_line );
  connect( ll_linecbox, SIGNAL( activated(int) ), 
	   SLOT( ll_linecboxActivated(int) ) );
  // linetypebox
  ll_linetypelabel= new QLabel( tr("Line type"), ll_frame );
  ll_linetypebox=  LinetypeBox( ll_frame,false,ll_linetype );
  connect( ll_linetypebox, SIGNAL( activated(int) ), 
	   SLOT( ll_linetypeboxActivated(int) ) );
  // colorcbox
  ll_colorlabel= new QLabel(tr("Colour"), ll_frame);
  ll_colorcbox = ColourBox( ll_frame, cInfo, false, 0 );  // last one index
  connect( ll_colorcbox, SIGNAL( activated(int) ), 
	   SLOT( ll_colorcboxActivated(int) ) );
  // density
  ll_densitylabel= new QLabel(tr("Density"), ll_frame);
  ll_density= ComboBox( ll_frame, densities, true, ll_dens );
  connect( ll_density, SIGNAL( activated(int) ),
	   SLOT( ll_densitycboxActivated(int) ) );
  // zorder
  ll_zorderlabel= new QLabel(tr("Plot position"), ll_frame);
  ll_zorder= ComboBox( ll_frame, zorders, true, ll_z );
  connect( ll_zorder, SIGNAL( activated(int) ), 
	   SLOT( ll_zordercboxActivated(int) ) );

  latlon->setChecked(latlonb);

  // =============================================================

  // frame on map...
  QFrame* ff_frame= new QFrame(this,"ff_frame");
  ff_frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  // frame checkbox
  frameb= false;
  framelabel= TitleLabel(tr("Show frame"),ff_frame);
  showframe= new QCheckBox("", ff_frame, "showframe");
  QToolTip::add( showframe,
		 tr("Draw boundary of selected area") );
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
  ff_linetypebox=  LinetypeBox( ff_frame,false,ff_linetype );
  connect( ff_linetypebox, SIGNAL( activated(int) ), 
	   SLOT( ff_linetypeboxActivated(int) ) );
  // colorcbox
  ff_colorlabel= new QLabel(tr("Colour"), ff_frame);
  ff_colorcbox = ColourBox( ff_frame, cInfo, false, 0 );  // last one index
  connect( ff_colorcbox, SIGNAL( activated(int) ), 
	   SLOT( ff_colorcboxActivated(int) ) );
  // zorder
  ff_zorderlabel= new QLabel(tr("Plot position"), ff_frame);
  ff_zorder= ComboBox( ff_frame, zorders, true, ff_z );
  connect( ff_zorder, SIGNAL( activated(int) ), 
	   SLOT( ff_zordercboxActivated(int) ) );

  // =============================================================

  // Background colour
  backcolorlabel= TitleLabel( tr("Background colour"), this);
  backcolorcbox= ColourBox( this, cInfo, true, 0 );  // last one index
  connect( backcolorcbox, SIGNAL( activated(int) ), 
	   SLOT( backcolorcboxActivated(int) ) );


  // ==================================================
  // Buttons
  savefavourite = new QPushButton(tr("Save favourite"), this);
  usefavourite = new QPushButton(tr("Use favourite"), this);
  mapapply = new QPushButton(tr("Apply"), this);
  mapapplyhide = new QPushButton(tr("Apply+Hide"), this);
  maphide = new QPushButton(tr("Hide"), this);
  maphelp = new QPushButton(tr("Help"), this);
     
  QToolTip::add( savefavourite,
		 tr("Save this layout as your favourite") );
  QToolTip::add( usefavourite,
		 tr("Use saved favourite layout") );


  connect( savefavourite, SIGNAL(clicked()), SLOT(savefavouriteClicked()));
  connect( usefavourite, SIGNAL(clicked()), SLOT(usefavouriteClicked()));
  connect( mapapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));
  connect( mapapply, SIGNAL(clicked()), SIGNAL(MapApply()));
  connect( maphide, SIGNAL(clicked()), SIGNAL(MapHide()));
  connect( maphelp, SIGNAL(clicked()),SLOT(helpClicked())); 

  const int border= 5;
  const int space= 5;

  // ==================================================
  // Area and backgroundcolour
  // ==================================================
  QVBoxLayout* v1layout = new QVBoxLayout( 5 );
  v1layout->addWidget( arealabel );
  v1layout->addWidget( areabox ); 
 
  // background layout
  QHBoxLayout* backlayout = new QHBoxLayout( 5 );
  backlayout->addWidget( backcolorlabel );
  backlayout->addWidget( backcolorcbox );
  backlayout->addStretch();
  v1layout->addLayout( backlayout );

  // ==================================================
  // Latlon-lines and frame
  // ==================================================
  QHBoxLayout* h2layout = new QHBoxLayout( 5 );
  
  // Latlon layouts ---------
  QHBoxLayout* latlonlayout = new QHBoxLayout( 5 );
  latlonlayout->addWidget(ll_frame);
  
  QVBoxLayout* h2h1v1layout= new QVBoxLayout(ll_frame, border, space);
  QHBoxLayout* ll_h3vhlayout= new QHBoxLayout(h2h1v1layout,5);
  ll_h3vhlayout->addWidget(latlon);
  ll_h3vhlayout->addWidget(ll_label);
  ll_h3vhlayout->addStretch();
  
  QGridLayout* ll_h3vglayout= new QGridLayout(5,2,space);
  ll_h3vglayout->setColStretch(0,10);
  ll_h3vglayout->addWidget(ll_colorlabel,0,0);
  ll_h3vglayout->addWidget(ll_colorcbox ,0,1);
  ll_h3vglayout->addWidget(ll_linelabel ,1,0);
  ll_h3vglayout->addWidget(ll_linecbox  ,1,1);
  ll_h3vglayout->addWidget(ll_linetypelabel ,2,0);
  ll_h3vglayout->addWidget(ll_linetypebox  ,2,1);
  ll_h3vglayout->addWidget(ll_densitylabel,3,0);
  ll_h3vglayout->addWidget(ll_density  ,   3,1);
  ll_h3vglayout->addWidget(ll_zorderlabel, 4,0);
  ll_h3vglayout->addWidget(ll_zorder      ,4,1);

  h2h1v1layout->addLayout(ll_h3vglayout);

  h2layout->addLayout( latlonlayout );


  // frame layouts ---------
  QHBoxLayout* framelayout = new QHBoxLayout( 5 );
  framelayout->addWidget(ff_frame);
  
  QVBoxLayout* h2h2v1layout= new QVBoxLayout(ff_frame, border, space);
  QHBoxLayout* framehlayout= new QHBoxLayout(h2h2v1layout,5);
  framehlayout->addWidget( showframe );
  framehlayout->addWidget(framelabel);
  framehlayout->addStretch();

  QGridLayout* ff_h3vglayout= new QGridLayout(4,2,space);
  ff_h3vglayout->setColStretch(0,10);
  ff_h3vglayout->addWidget(ff_colorlabel,0,0);
  ff_h3vglayout->addWidget(ff_colorcbox ,0,1);
  ff_h3vglayout->addWidget(ff_linelabel ,1,0);
  ff_h3vglayout->addWidget(ff_linecbox  ,1,1);
  ff_h3vglayout->addWidget(ff_linetypelabel ,2,0);
  ff_h3vglayout->addWidget(ff_linetypebox  ,2,1);
  ff_h3vglayout->addWidget(ff_zorderlabel, 3,0);
  ff_h3vglayout->addWidget(ff_zorder      ,3,1);

  h2h2v1layout->addLayout(ff_h3vglayout);
  h2h2v1layout->addStretch( );

  h2layout->addLayout( framelayout );

  // ==================================================
  // Maps, selected maps with buttons and options
  // ==================================================

  QHBoxLayout* h3layout = new QHBoxLayout( 5 );
  // maps layout
  QVBoxLayout* mapsvlayout = new QVBoxLayout( 5 );
  mapsvlayout->addWidget( maplabel );
  mapsvlayout->addWidget( mapbox );
  mapsvlayout->addWidget( selectedMaplabel );
  mapsvlayout->addWidget( selectedMapbox );  
  
  h3layout->addLayout( mapsvlayout );
  
  // map buttons
  QHBoxLayout* h4layout = new QHBoxLayout( 5 );
  h4layout->addWidget(mapdelete);
  h4layout->addWidget(mapalldelete);


  // contours and filled maps
  QHBoxLayout* h5layout = new QHBoxLayout( 5 );
  
  // Contours layouts ---------
  QHBoxLayout* cont_h3layout = new QHBoxLayout( 5 );
  cont_h3layout->addWidget(cont_frame);
  
  QVBoxLayout* h3vlayout= new QVBoxLayout(cont_frame, border, space);
  QHBoxLayout* h3vhlayout= new QHBoxLayout(h3vlayout,5);
  h3vhlayout->addWidget(contours);
  h3vhlayout->addWidget(cont_label);
  h3vhlayout->addStretch();

  QGridLayout* h3vglayout= new QGridLayout(4,2,space);
  h3vglayout->setColStretch(0,10);
  h3vglayout->addWidget(cont_colorlabel,0,0);
  h3vglayout->addWidget(cont_colorcbox ,0,1);
  h3vglayout->addWidget(cont_linelabel ,1,0);
  h3vglayout->addWidget(cont_linecbox  ,1,1);
  h3vglayout->addWidget(cont_linetypelabel ,2,0);
  h3vglayout->addWidget(cont_linetypebox  ,2,1);
  h3vglayout->addWidget(cont_zorderlabel,3,0);
  h3vglayout->addWidget(cont_zorder  ,3,1);

  h3vlayout->addLayout(h3vglayout);

  h5layout->addLayout( cont_h3layout );

  // Land layouts ---------
  QHBoxLayout* land_h3layout = new QHBoxLayout( 5 );
  land_h3layout->addWidget(land_frame);
  
  QVBoxLayout* land_h3vlayout= new QVBoxLayout(land_frame, border, space);
  QHBoxLayout* land_h3vhlayout= new QHBoxLayout(land_h3vlayout,5);
  land_h3vhlayout->addWidget(filledland);
  land_h3vhlayout->addWidget(land_label);
  land_h3vhlayout->addStretch();
  
  QGridLayout* land_h3vglayout= new QGridLayout(2,2,space);
  land_h3vglayout->setColStretch(0,10);
  land_h3vglayout->addWidget(land_colorlabel,0,0);
  land_h3vglayout->addWidget(land_colorcbox ,0,1);
  land_h3vglayout->addWidget(land_zorderlabel,1,0);
  land_h3vglayout->addWidget(land_zorder  ,1,1);

  land_h3vlayout->addLayout(land_h3vglayout);
  land_h3vlayout->addStretch();

  h5layout->addLayout( land_h3layout );


  // buttons layout
  QGridLayout* buttonlayout= new QGridLayout(2, 3, space);
  buttonlayout->addWidget( maphelp,      0, 0 );
  buttonlayout->addWidget( savefavourite,0, 1 );
  buttonlayout->addWidget( usefavourite, 0, 2 );

  buttonlayout->addWidget( maphide,      1, 0 );
  buttonlayout->addWidget( mapapplyhide, 1, 1 );
  buttonlayout->addWidget( mapapply,     1, 2 ); 

  // vlayout
  QVBoxLayout* vlayout = new QVBoxLayout( this, 5, 10 );
  vlayout->addLayout( v1layout );
  vlayout->addLayout( h2layout );
  vlayout->addLayout( h3layout );
  vlayout->addLayout( h4layout );
  vlayout->addLayout( h5layout );
  vlayout->addLayout( buttonlayout );
  vlayout->activate(); 

  mapboxChanged();// this must be called after selected mapbox is created

  savefavouriteClicked();  //default favourite

}//end constructor MapDialog




// FRAME SLOTS

void MapDialog::showframe_checkboxActivated(bool on)
{
  frameb= on;
  
  if (on){
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

void MapDialog::ff_linecboxActivated( int index ){
  framelw= miString(index+1);
}

void MapDialog::ff_linetypeboxActivated( int index ){
  framelt= linetypes[index];
}


void MapDialog::ff_colorcboxActivated( int index ){
  framec= cInfo[index].name;
}

void MapDialog::ff_zordercboxActivated( int index ){
  framez= index;
}


// MAP slots

void MapDialog::mapboxChanged(){
#ifdef dMapDlg 
  cerr<<"MapDialog::mapboxChanged called"<<endl;
#endif
  
  int numselected=selectedmaps.size();
  int current=-1,j;
  // find current new map - if any
  if (numselected > 0){
    for( int i=0; i< numMaps; i++ ){	
      if( mapbox->item(i)->isSelected() ){
	for (j=0; j<numselected; j++)
	  if (selectedmaps[j]==i)
	    break;
	if (j==numselected) current=i; // the new map
      }
    }
  }


  selectedmaps.clear();
  numselected= 0;
  int activeidx=-1;
  bool currmapok= false;

  // identify selected maps - keep index to current clicked or previous
  // selected map
  for (int i=0; i<numMaps; i++){
    if (mapbox->item(i)->isSelected()){
      selectedmaps.push_back(i);
      if (current==i || (!currmapok && activemap==i)){
	currmapok= true;
	activeidx= numselected;
      }
      numselected++;
    }
  }
  if (!currmapok) {
    if (numselected>0){
      activeidx= 0;
    }
  }

  // update listbox of selected maps - stop any signals
  selectedMapbox->blockSignals( true );
  selectedMapbox->clear();
  for (int i=0; i<numselected; i++){
    selectedMapbox->addItem(QString(m_MapDI.maps[selectedmaps[i]].name.c_str()));
  }
  selectedMapbox->blockSignals( false );
  
  if( numselected==0 ){// none is selected
    selectedMapbox->setEnabled( false );
    contours->setEnabled( false );
    cont_colorcbox->setEnabled( false );
    cont_linecbox->setEnabled( false );
    cont_linetypebox->setEnabled( false );
    cont_zorder->setEnabled( false );
    filledland->setEnabled( false );
    land_colorcbox->setEnabled( false );
    land_zorder->setEnabled( false );
  } else {
    // select the active map
    selectedMapbox->setCurrentRow(activeidx);
    selectedMapbox->setEnabled( true );
    selectedMapboxClicked(selectedMapbox->currentItem());
  }
}



void MapDialog::selectedMapboxClicked( QListWidgetItem* item ){
  // new selection in list of selected maps
  // fill all option-widgets for map

  int index = selectedMapbox->currentRow();
  activemap=selectedmaps[index];

  bool island= m_MapDI.maps[activemap].type=="triangles";

  bool ison= m_MapDI.maps[activemap].contour.ison;
  int m_colIndex =  getIndex( cInfo, m_MapDI.maps[activemap].contour.linecolour );
  int m_linewIndex = atoi(m_MapDI.maps[activemap].contour.linewidth.cStr())-1;
  int m_linetIndex = getIndex( linetypes, m_MapDI.maps[activemap].contour.linetype  );
  int m_zIndex = m_MapDI.maps[activemap].contour.zorder;

  if (island) contours->setEnabled( true );
  else contours->setEnabled( false );
  cont_colorcbox->setEnabled( true );
  cont_linecbox->setEnabled( true );
  cont_linetypebox->setEnabled( true );
  cont_zorder->setEnabled( true );
  
  filledland->setEnabled( island );
  land_colorcbox->setEnabled( island );
  land_zorder->setEnabled( island );


  // set contours options
  contours->setChecked(ison);
  cont_colorcbox->setCurrentItem( m_colIndex );
  cont_linecbox->setCurrentItem( m_linewIndex );
  cont_linetypebox->setCurrentItem( m_linetIndex );
  cont_zorder->setCurrentItem( m_zIndex );

  // set filled land options
  ison= m_MapDI.maps[activemap].land.ison;
  m_colIndex =  getIndex( cInfo, m_MapDI.maps[activemap].land.fillcolour );
  m_zIndex = m_MapDI.maps[activemap].land.zorder;

  filledland->setChecked(ison);
  land_colorcbox->setCurrentItem( m_colIndex );
  land_zorder->setCurrentItem( m_zIndex );
}

void MapDialog::mapdeleteClicked(){
  
  if (activemap >= 0 && activemap < numMaps){
    mapbox->item(activemap)->setSelected(false);
  }

  mapboxChanged();
}

void MapDialog::mapalldeleteClicked(){
  // deselect all maps
  mapbox->clearSelection();

  mapboxChanged();
}

// CONTOUR SLOTS

void MapDialog::cont_checkboxActivated(bool on)
{
  if (activemap < 0) {
    cerr << "checkboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].contour.ison= on;

  if (on){
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

void MapDialog::cont_linecboxActivated( int index ){
  if (activemap < 0) {
    cerr << "linecboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].contour.linewidth= miString(index+1);
}

void MapDialog::cont_linetypeboxActivated( int index ){
  if (activemap < 0) {
    cerr << "linetypeboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].contour.linetype= linetypes[index];
}


void MapDialog::cont_colorcboxActivated( int index ){
  if (activemap < 0) {
    cerr << "colorcboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].contour.linecolour= cInfo[index].name;
}

void MapDialog::cont_zordercboxActivated( int index ){
  if (activemap < 0) {
    cerr << "zordercboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].contour.zorder= index;
}


// LAND SLOTS


void MapDialog::land_checkboxActivated(bool on)
{
  if (activemap < 0) {
    cerr << "checkboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].land.ison= on;

  if (on){
    land_colorcbox->setEnabled(true);
    land_zorder->setEnabled(true);
  } else {
    land_colorcbox->setEnabled(false);
    land_zorder->setEnabled(false);
  }
}

void MapDialog::land_colorcboxActivated( int index ){
  if (activemap < 0) {
    cerr << "colorcboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].land.fillcolour= cInfo[index].name;
}

void MapDialog::land_zordercboxActivated( int index ){
  if (activemap < 0) {
    cerr << "zordercboxactivated::Catastrophic: activemap < 0" << endl;
    return;
  }
  m_MapDI.maps[activemap].land.zorder= index;
}


// LATLON SLOTS

void MapDialog::ll_checkboxActivated(bool on)
{
  latlonb= on;

  if (on){
    ll_linecbox->setEnabled(true);
    ll_linetypebox->setEnabled(true);
    ll_colorcbox->setEnabled(true);
    ll_density->setEnabled(true);
    ll_zorder->setEnabled(true);
  } else {
    ll_linecbox->setEnabled(false);
    ll_linetypebox->setEnabled(false);
    ll_colorcbox->setEnabled(false);
    ll_density->setEnabled(false);
    ll_zorder->setEnabled(false);
  }
}

void MapDialog::ll_linecboxActivated( int index ){
  latlonlw= miString(index+1);
}

void MapDialog::ll_linetypeboxActivated( int index ){
  latlonlt= linetypes[index];
}


void MapDialog::ll_colorcboxActivated( int index ){
  latlonc= cInfo[index].name;
}

void MapDialog::ll_densitycboxActivated( int index ){
  latlond= atof(densities[index].c_str());
}

void MapDialog::ll_zordercboxActivated( int index ){
  latlonz= index;
}

// -----------------

void MapDialog::backcolorcboxActivated(int index){
#ifdef dMapDlg
  cerr<<"MapDialog::backcolorcboxActivated called"<<endl;
#endif
}

/*
  BUTTONS
*/

void MapDialog::savefavouriteClicked(){
  // save current status as favourite
  favourite= getOKString();

  // enable this button
  usefavourite->setEnabled(true);
}

void MapDialog::useFavourite()
{
  putOKString(favourite);
}

void MapDialog::usefavouriteClicked(){
  useFavourite();
}

void MapDialog::applyhideClicked(){
  emit MapApply();
  emit MapHide();
}

void  MapDialog::helpClicked(){
  emit showdoc("ug_mapdialogue.html"); 
}

bool MapDialog::close(bool alsoDelete){
  emit MapHide();
  return true;
}

// -------------------------------------------------------
// get and put OK strings
// -------------------------------------------------------


/*
  GetOKString
*/

vector<miString> MapDialog::getOKString(){
#ifdef dMapDlg
  cerr<<"MapDialog::getOKString called"<<endl;
#endif 
  vector<miString> vstr;
  
  int lindex;
  int numselected= selectedmaps.size();

  if (numselected==0){ // no maps selected
    int backc=backcolorcbox->currentItem();
    ostringstream ostr;
    ostr << "MAP";
    // qt4 fix: added .toStdString()
    ostr << " area=" << areabox->currentItem()->text().toStdString()
	 << " backcolour="
	 << (backc>=0 && backc<cInfo.size() ? cInfo[backc].name : "white");

    // set latlon options
    MapInfo mi;
    mi.contour.ison= false;
    mi.land.ison= false;
    mi.latlon.ison= latlonb;
    mi.latlon.linecolour= latlonc;
    mi.latlon.linewidth= latlonlw;
    mi.latlon.linetype= latlonlt;
    mi.latlon.zorder= latlonz;
    mi.latlon.density= latlond;

    mi.frame.ison= frameb;
    mi.frame.linecolour= framec;
    mi.frame.linewidth= framelw;
    mi.frame.linetype= framelt;
    
    miString mstr;
    m_ctrl->MapInfoParser(mstr,mi,true);
    ostr << " " << mstr;
    
    vstr.push_back(ostr.str());
    
    // clear log-list
    logmaps.clear();

  } else {
  
    vector<int> lmaps; // check for logging

    for( int i=0; i<numselected; i++ ){
      lindex= selectedmaps[i];
      // check if ok to log
      if (m_MapDI.maps[lindex].logok) lmaps.push_back(lindex);
      ostringstream ostr;
      ostr << "MAP";
      if (i==numselected-1) { // common options for last map only
	int backc=backcolorcbox->currentItem();
    // qt4 fix: added .toStdString()
	ostr << " area=" << areabox->currentItem()->text().toStdString()
	     << " backcolour="
	     << (backc>=0 && backc<cInfo.size() ? cInfo[backc].name : "white");

	// set latlon options
	m_MapDI.maps[lindex].latlon.ison= latlonb;
	m_MapDI.maps[lindex].latlon.linecolour= latlonc;
	m_MapDI.maps[lindex].latlon.linewidth= latlonlw;
	m_MapDI.maps[lindex].latlon.linetype= latlonlt;
	m_MapDI.maps[lindex].latlon.zorder= latlonz;
	m_MapDI.maps[lindex].latlon.density= latlond;

	// set frame options
	m_MapDI.maps[lindex].frame.ison= frameb;
	m_MapDI.maps[lindex].frame.linecolour= framec;
	m_MapDI.maps[lindex].frame.linewidth= framelw;
	m_MapDI.maps[lindex].frame.linetype= framelt;

      } else {
	m_MapDI.maps[lindex].latlon.ison= false; 
	m_MapDI.maps[lindex].frame.ison= false;
      }
      
      miString mstr;
      m_ctrl->MapInfoParser(mstr,m_MapDI.maps[lindex],true);
      ostr << " " << mstr;
      
      vstr.push_back(ostr.str());
    }
    if (lmaps.size() > 0) logmaps= lmaps;
  }

  return vstr;
}

/*
  PutOKString
*/

void MapDialog::putOKString(const vector<miString>& vstr)
{
  int n= vstr.size();
  vector<int> themaps;
  vector<miString> tokens,stokens;
  miString bgcolour,area;
  int lastmap= -1;

  int iline;
  for (iline=0; iline<n; iline++){
    miString str= vstr[iline];
    str.trim();
    if (!str.exists()) continue;
    if (str[0]=='#') continue;
    miString themap= "";
    tokens= str.split(" ");
    int m= tokens.size();
    if (m>0 && tokens[0].upcase()!="MAP") continue;
    for (int j=0; j<m; j++){
      stokens= tokens[j].split('=');
      if (stokens.size()==2){
	if (stokens[0].upcase()=="BACKCOLOUR")
	  bgcolour= stokens[1];
	else if (stokens[0].upcase()=="AREA")
	  area= stokens[1];
	else if (stokens[0].upcase()=="MAP")
	  themap= stokens[1];
      }
    }
    // find the logged map in full list
    if (themap.exists()){
      int idx=-1;
      for (int k=0; k<numMaps; k++)
	if (m_MapDI.maps[k].name == themap){
	  idx= k;
	  lastmap= idx;
	  themaps.push_back(k); // keep list of selected maps
	  break;
	}
      // update options
      if (idx>=0){
	m_ctrl->MapInfoParser(str,m_MapDI.maps[idx],false);
      }
    }
  }

  // reselect area
  int area_index=0;
  for (int i=0; i<m_MapDI.areas.size(); i++)
    if (m_MapDI.areas[i] == area){
      area_index=i;
      break;
    }
  areabox->setCurrentRow(area_index);
  
  // deselect all maps
  mapbox->clearSelection();

  int nm= themaps.size();
  // reselect maps
  for( int i=0; i<nm; i++ ){
    mapbox->item(themaps[i])->setSelected(true);
  }

  if (lastmap>0){
    // set latlon options
    latlonb= m_MapDI.maps[lastmap].latlon.ison;
    latlonc= m_MapDI.maps[lastmap].latlon.linecolour;
    latlonlw= m_MapDI.maps[lastmap].latlon.linewidth;
    latlonlt= m_MapDI.maps[lastmap].latlon.linetype;
    latlonz= m_MapDI.maps[lastmap].latlon.zorder;
    latlond= m_MapDI.maps[lastmap].latlon.density;
    
    int m_colIndex =  getIndex( cInfo, latlonc );
    int m_linewIndex = atoi(latlonlw.cStr())-1;
    int m_linetIndex = getIndex( linetypes, latlonlt );
    // find density index
    int m_densIndex= 0;
    int dens= int(latlond*1000);
    for (int k=0; k<densities.size(); k++)
      if (int(atof(densities[k].c_str())*1000)==dens){
	m_densIndex= k;
	break;
      }
    int m_zIndex = latlonz;
    latlon->setChecked(latlonb);
    if (m_colIndex >=0)
      ll_colorcbox->setCurrentItem( m_colIndex );
    if ( m_linewIndex >= 0)
      ll_linecbox->setCurrentItem( m_linewIndex );
    if ( m_linetIndex >= 0)
      ll_linetypebox->setCurrentItem( m_linetIndex );
    if ( m_densIndex >= 0)
      ll_density->setCurrentItem( m_densIndex );
    if ( m_zIndex >= 0)
      ll_zorder->setCurrentItem( m_zIndex );


    // set frame options
    frameb=  m_MapDI.maps[lastmap].frame.ison;
    framec=  m_MapDI.maps[lastmap].frame.linecolour;
    framelw= m_MapDI.maps[lastmap].frame.linewidth;
    framelt= m_MapDI.maps[lastmap].frame.linetype;
    framez=  m_MapDI.maps[lastmap].frame.zorder;
    
    m_colIndex =  getIndex( cInfo, framec );
    m_linewIndex = atoi(framelw.cStr())-1;
    m_linetIndex = getIndex( linetypes, framelt );
    m_zIndex = framez;

    showframe->setChecked(frameb);
    if (m_colIndex >=0)
      ff_colorcbox->setCurrentItem( m_colIndex );
    if ( m_linewIndex >= 0)
      ff_linecbox->setCurrentItem( m_linewIndex );
    if ( m_linetIndex >= 0)
      ff_linetypebox->setCurrentItem( m_linetIndex );
    if ( m_zIndex >= 0)
      ff_zorder->setCurrentItem( m_zIndex );
  }

  // set background
  int m_colIndex= getIndex( cInfo, bgcolour );
  if (m_colIndex >= 0)
    backcolorcbox->setCurrentItem( m_colIndex );
}



miString MapDialog::getShortname()
{
  miString name;

  name= areabox->item(areabox->currentRow())->text().latin1() + miString(" ");
  
  int numselected= selectedmaps.size();
  for( int i=0; i<numselected; i++){
    int lindex= selectedmaps[i];
    name+= m_MapDI.maps[lindex].name + miString(" ");
  }
  
  name= "<font color=\"#009900\">" + name + "</font>";
  return name;
}


// ------------------------------------------------
// LOG-FILE read/write methods
// ------------------------------------------------

vector<miString> MapDialog::writeLog()
{
  vector<miString> vstr;
  
  // first: write all map-specifications
  int n= m_MapDI.maps.size();
  for( int i=0; i<n; i++ ){
    ostringstream ostr;
    if (i==n-1) { // common options for last map only
      // qt4 fix: added .toStdString()
      ostr << "area=" << areabox->item(areabox->currentRow())->text().toStdString()
	   << " backcolour=" << cInfo[backcolorcbox->currentItem()].name
	   << " ";
      // set latlon options
      m_MapDI.maps[i].latlon.ison= latlonb;
      m_MapDI.maps[i].latlon.linecolour= latlonc;
      m_MapDI.maps[i].latlon.linewidth= latlonlw;
      m_MapDI.maps[i].latlon.linetype= latlonlt;
      m_MapDI.maps[i].latlon.zorder= latlonz;
      m_MapDI.maps[i].latlon.density= latlond;
      // set frame options
      m_MapDI.maps[i].frame.ison= frameb;
      m_MapDI.maps[i].frame.linecolour= framec;
      m_MapDI.maps[i].frame.linewidth= framelw;
      m_MapDI.maps[i].frame.linetype= framelt;
    } else {
      m_MapDI.maps[i].latlon.ison= false;
      m_MapDI.maps[i].frame.ison= false;
    }
    
    miString mstr;
    m_ctrl->MapInfoParser(mstr,m_MapDI.maps[i],true);
    ostr << mstr;

    vstr.push_back(ostr.str());
    vstr.push_back("-------------------");
  }
  
  // end of complete map-list
  vstr.push_back("=================== Selected maps =========");

  // write name of current selected (and legal) maps
  int lindex;
  int numselected= logmaps.size();
  for( int i=0; i<numselected; i++ ){
    lindex= logmaps[i];
    vstr.push_back(m_MapDI.maps[lindex].name);
  }

  vstr.push_back("=================== Favourites ============");
  // write favourite options
  for( int i=0; i<favourite.size(); i++ ){
    vstr.push_back(favourite[i]);
  }

  vstr.push_back("===========================================");

  return vstr;
}

void MapDialog::readLog(const vector<miString>& vstr,
			const miString& thisVersion,
			const miString& logVersion)
{
  // version-check
  //bool oldversion= (thisVersion!=logVersion && logVersion < "2001-08-25");

  int n=vstr.size();
  vector<int> themaps;
  vector<miString> tokens,stokens;
  miString bgcolour,area;
  int lastmap= -1;

  int iline;
  for (iline=0; iline<n; iline++){
    miString str= vstr[iline];
    str.trim();
    if (str.exists() && str[0]=='-') continue;
    if (str.exists() && str[0]=='=') break;
    miString themap= "";
    tokens= str.split(" ");
    int m= tokens.size();
    for (int j=0; j<m; j++){
      stokens= tokens[j].split('=');
      if (stokens.size()==2){
	if (stokens[0].upcase()=="BACKCOLOUR")
	  bgcolour= stokens[1];
	else if (stokens[0].upcase()=="AREA")
	  area= stokens[1];
	else if (stokens[0].upcase()=="MAP")
	  themap= stokens[1];
      }
    }
    // find the logged map in full list
    if (themap.exists()){
      int idx=-1;
      for (int k=0; k<numMaps; k++)
	if (m_MapDI.maps[k].name == themap){
	  idx= k;
	  lastmap= idx;
	  //	  themaps.push_back(k); // just for old log-files
	  break;
	}
      // update options
      if (idx>=0)
	m_ctrl->MapInfoParser(str,m_MapDI.maps[idx],false);
    }
  }
  
  // read previous selected maps
  if (iline < n && vstr[iline][0]=='='){
    themaps.clear();
    iline++;
    for (; iline < n; iline++){
      miString themap= vstr[iline];
      themap.trim();
      if (!themap.exists()) continue;
      if (themap[0]=='=') break;
      for (int k=0; k<numMaps; k++)
	if (m_MapDI.maps[k].name == themap){
	  themaps.push_back(k);
	  break;
	}
    }
  }

  // read favourite
  favourite.clear();
  if (iline < n && vstr[iline][0]=='='){
    iline++;
    for (; iline < n; iline++){
      miString str=vstr[iline];
      str.trim();
      if (!str.exists()) continue;
      if (str[0]=='=') break;
      favourite.push_back(str);
    }
  }
  usefavourite->setEnabled(favourite.size()>0);

  // reselect area
  int area_index=0;
  for (int i=0; i<m_MapDI.areas.size(); i++)
    if (m_MapDI.areas[i] == area){
      area_index=i;
      break;
    }
  areabox->setCurrentRow(area_index);

  // deselect all maps
  mapbox->clearSelection();

  int nm= themaps.size();
  // reselect maps
  for( int i=0; i<nm; i++ ){
    mapbox->item(themaps[i])->setSelected(true);
  }
  // keep for logging
  logmaps= themaps;

  if (lastmap>0){
    // set latlon options
    latlonb= m_MapDI.maps[lastmap].latlon.ison;
    latlonc= m_MapDI.maps[lastmap].latlon.linecolour;
    latlonlw= m_MapDI.maps[lastmap].latlon.linewidth;
    latlonlt= m_MapDI.maps[lastmap].latlon.linetype;
    latlonz= m_MapDI.maps[lastmap].latlon.zorder;
    latlond= m_MapDI.maps[lastmap].latlon.density;
    
    int m_colIndex =  getIndex( cInfo, latlonc );
    int m_linewIndex = atoi(latlonlw.cStr())-1;
    int m_linetIndex = getIndex( linetypes, latlonlt );
    // find density index
    int m_densIndex= 0;
    int dens= int(latlond*1000);
    for (int k=0; k<densities.size(); k++)
      if (int(atof(densities[k].c_str())*1000)==dens){
	m_densIndex= k;
	break;
      }
    int m_zIndex = latlonz;
    latlon->setChecked(latlonb);
    if (m_colIndex >=0)
      ll_colorcbox->setCurrentItem( m_colIndex );
    if ( m_linewIndex >= 0)
      ll_linecbox->setCurrentItem( m_linewIndex );
    if ( m_linetIndex >= 0)
      ll_linetypebox->setCurrentItem( m_linetIndex );
    if ( m_densIndex >= 0)
      ll_density->setCurrentItem( m_densIndex );
    if ( m_zIndex >= 0)
      ll_zorder->setCurrentItem( m_zIndex );

    // set frame options
    frameb=   m_MapDI.maps[lastmap].frame.ison;
    framec=   m_MapDI.maps[lastmap].frame.linecolour;
    framelw=  m_MapDI.maps[lastmap].frame.linewidth;
    framelt=  m_MapDI.maps[lastmap].frame.linetype;
    framez=   m_MapDI.maps[lastmap].frame.zorder;
    
    m_colIndex =  getIndex( cInfo, framec );
    m_linewIndex = atoi(framelw.cStr()) - 1;
    m_linetIndex = getIndex( linetypes, framelt );
    m_zIndex = latlonz;

    showframe->setChecked(frameb);
    if (m_colIndex >=0)
      ff_colorcbox->setCurrentItem( m_colIndex );
    if ( m_linewIndex >= 0)
      ff_linecbox->setCurrentItem( m_linewIndex );
    if ( m_linetIndex >= 0)
      ff_linetypebox->setCurrentItem( m_linetIndex );
    if ( m_zIndex >= 0)
      ff_zorder->setCurrentItem( m_zIndex );
  }

  // set background
  int m_colIndex= getIndex( cInfo, bgcolour );
  if (m_colIndex >= 0)
    backcolorcbox->setCurrentItem( m_colIndex );
}


