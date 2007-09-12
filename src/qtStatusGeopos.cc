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
#include <qtStatusGeopos.h>
#include <qlayout.h>
#include <qlabel.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>

StatusGeopos::StatusGeopos(QWidget* parent, const char* name)
  : QWidget(parent,name) {

  geographicMode= true;
  decimalMode= false;
  
  // Create horisontal layout manager
  QHBoxLayout* thlayout = new QHBoxLayout( this, 0, 0, "thlayout");
  thlayout->setMargin(1);
  thlayout->setSpacing(5);

  sxlabel= new QLabel(tr("Lat:"),this,"sxlabel");
  sxlabel->setMinimumSize(sxlabel->sizeHint());
  thlayout->addWidget(sxlabel,0);

  latlabel= new QLabel(" 00°00'N",this,"xlabel");
  latlabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
//   latlabel->setPalette( QPalette( QColor(0, 54, 125) ) );
  latlabel->setMinimumSize(latlabel->sizeHint());
  thlayout->addWidget(latlabel,0);

  thlayout->addSpacing(10);
  
  sylabel= new QLabel(tr("Lon:"),this,"sylabel");
  sylabel->setMinimumSize(sylabel->sizeHint());
  thlayout->addWidget(sylabel,0);
  
  lonlabel= new QLabel("000°00'W",this,"ylabel");
  lonlabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  lonlabel->setMinimumSize(lonlabel->sizeHint());
  thlayout->addWidget(lonlabel,0);

  // Start the geometry management
  thlayout->activate();
}


void StatusGeopos::changeMode() {
  geographicMode= !geographicMode;
  if (geographicMode) {
    sxlabel->setText(tr("Lat:"));
    sylabel->setText(tr("Lon:"));
  } else {
    sxlabel->setText("  X:");
    sylabel->setText("  Y:");
  }
  latlabel->clear();
  lonlabel->clear();
}

void StatusGeopos::changeLatLonMode() {
  decimalMode= !decimalMode;
}


void StatusGeopos::setPosition(float lat, float lon){

  ostringstream slat, slon;

  if (geographicMode) {

    if (decimalMode) {

      slat << setw(6) << setprecision(5) << lat;
      slon << setw(6) << setprecision(5) <<lon;
    
    } else {

      int deg, min;

      min= int(fabsf(lat)*60.+0.5);
      deg= min/60;
      min= min%60;
      if (lat>=0.0)
	slat << setw(2) << deg << "°" << setw(2) << setfill('0') << min << "'N";
      else
	slat << setw(2) << deg << "°" << setw(2) << setfill('0') << min << "'S";
      
      min= int(fabsf(lon)*60.+0.5);
      deg= min/60;
      min= min%60;
      if (lon>=0.0)
	slon << setw(3) << deg << "°" << setw(2) << setfill('0') << min << "'E";
      else
	slon << setw(3) << deg << "°" << setw(2) << setfill('0') << min << "'W";
    }
    
  } else {
      
    float x= lat + 1.;  // fortran indexing
    float y= lon + 1.;
    slat << setw(6) << x;
    slon << setw(6) << y;

  }

  latlabel->setText(slat.str().c_str());
  lonlabel->setText(slon.str().c_str());
}

