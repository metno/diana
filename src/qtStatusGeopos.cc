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

#include "qtStatusGeopos.h"

#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>

#include <sstream>
#include <iomanip>
#include <math.h>

using namespace std;

StatusGeopos::StatusGeopos(QWidget* parent) :
  QWidget(parent)
{

  // Create horisontal layout manager
  QHBoxLayout* thlayout = new QHBoxLayout(this);
  thlayout->setMargin(1);
  thlayout->setSpacing(5);

  xybox = new QComboBox(this);
  xybox->setMinimumWidth(xybox->sizeHint().width());
  xybox->addItem(tr("Lat/Lon degree"));
  xybox->addItem(tr("Lat/Lon decimal"));
  xybox->addItem(tr("Proj coordinates"));
  xybox->addItem(tr("Grid coordinates"));
  xybox->addItem(tr("Area (km2)"));
  xybox->addItem(tr("Distance (km)"));
  thlayout->addWidget(xybox);

  sxlabel = new QLabel(tr("Lat:"), this);
  sxlabel->setMinimumSize(sxlabel->sizeHint());
  thlayout->addWidget(sxlabel, 0);

  latlabel = new QLabel(" 00\xB0""00'N", this);
  latlabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  latlabel->setMinimumSize(latlabel->sizeHint());
  thlayout->addWidget(latlabel, 0);

  thlayout->addSpacing(10);

  sylabel = new QLabel(tr("Lon:"), this);
  sylabel->setMinimumSize(sylabel->sizeHint());
  thlayout->addWidget(sylabel, 0);

  lonlabel = new QLabel("000\xB0""00'W", this);
  lonlabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  lonlabel->setMinimumSize(lonlabel->sizeHint());
  thlayout->addWidget(lonlabel, 0);

  // Start the geometry management
  thlayout->activate();
}

bool StatusGeopos::geographicMode()
{
  return (xybox->currentIndex() < 2);
}

bool StatusGeopos::gridMode()
{
  return (xybox->currentIndex() == 3);
}

bool StatusGeopos::areaMode()
{
  return (xybox->currentIndex() == 4);
}

bool StatusGeopos::distMode()
{
  return (xybox->currentIndex() == 5);
}


void StatusGeopos::setPosition(float lat, float lon)
{

  ostringstream slat, slon;

  if (xybox->currentIndex() < 2) {

    sxlabel->setText(tr("Lat:"));
    sylabel->setText(tr("Lon:"));

    if (xybox->currentIndex() == 1) {

      slat << setw(6) << setprecision(5) << lat;
      slon << setw(6) << setprecision(5) << lon;

    } else {

      int deg, min;

      min = int(fabsf(lat) * 60. + 0.5);
      deg = min / 60;
      min = min % 60;
      if (lat >= 0.0)
        slat << setw(2) << deg << "\xB0" << setw(2) << setfill('0') << min << "'N";
      else
        slat << setw(2) << deg << "\xB0" << setw(2) << setfill('0') << min << "'S";

      min = int(fabsf(lon) * 60. + 0.5);
      deg = min / 60;
      min = min % 60;
      if (lon >= 0.0)
        slon << setw(3) << deg << "\xB0" << setw(2) << setfill('0') << min << "'E";
      else
        slon << setw(3) << deg << "\xB0" << setw(2) << setfill('0') << min << "'W";
    }
  } else if (xybox->currentIndex() == 4){
    sxlabel->setText("  Mark:");
    sylabel->setText("Window:");
    slat << lat;
    slon << lon;
  } else if (xybox->currentIndex() == 5){
    sxlabel->setText("  Vert:");
    sylabel->setText(" Horiz:");
    slat << lat;
    slon << lon;
  } else {

    sxlabel->setText("  X:");
    sylabel->setText("  Y:");

    float x = lat;
    float y = lon;
    slat << setw(6) << x;
    slon << setw(6) << y;

  }

  latlabel->setText(slat.str().c_str());
  lonlabel->setText(slon.str().c_str());
}

void StatusGeopos::undefPosition()
{
  latlabel->setText("");
  lonlabel->setText("");
}
