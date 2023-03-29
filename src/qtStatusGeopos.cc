/*
 Diana - A Free Meteorological Visualisation Tool

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

#include "diana_config.h"

#include "qtStatusGeopos.h"

#include "diController.h"
#include "util/qstring_util.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>

#include <sstream>
#include <iomanip>
#include <math.h>


StatusGeopos::StatusGeopos(Controller* c, QWidget* parent)
    : QWidget(parent)
    , controller(c)
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

  latlabel = new QLabel(tr(" 00°0""00'N"), this);
  latlabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  latlabel->setMinimumSize(latlabel->sizeHint());
  thlayout->addWidget(latlabel, 0);

  thlayout->addSpacing(10);

  sylabel = new QLabel(tr("Lon:"), this);
  sylabel->setMinimumSize(sylabel->sizeHint());
  thlayout->addWidget(sylabel, 0);

  lonlabel = new QLabel(tr("000°0""00'W"), this);
  lonlabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  lonlabel->setMinimumSize(lonlabel->sizeHint());
  thlayout->addWidget(lonlabel, 0);

  // Start the geometry management
  thlayout->activate();
}

void StatusGeopos::handleMousePos(int x, int y)
{
  float xmap = -1., ymap = -1.;
  controller->PhysToMap(x, y, xmap, ymap);
  if (geographicMode()) {
    float lat = 0, lon = 0;
    if (controller->PhysToGeo(x, y, lat, lon)) {
      setPosition(lat, lon);
    } else {
      undefPosition();
    }
  } else if (gridMode()) {
    float gridx = 0, gridy = 0;
    if (controller->MapToGrid(xmap, ymap, gridx, gridy)) {
      setPosition(gridx, gridy);
    } else {
      undefPosition();
    }
  } else if (areaMode()) { // Show area in km2
    double markedArea = controller->getMarkedArea(x, y);
    double windowArea = controller->getWindowArea();
    setPosition(markedArea / (1000.0 * 1000.0), windowArea / (1000.0 * 1000.0));
  } else if (distMode()) { // Show distance in km
    double horizDist = controller->getWindowDistances(x, y, true);
    double vertDist = controller->getWindowDistances(x, y, false);
    setPosition(vertDist / 1000.0, horizDist / 1000.0);
  } else {
    setPosition(xmap, ymap);
  }
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

  QString slat, slon;

  if (xybox->currentIndex() < 2) {
    sxlabel->setText(tr("Lat:"));
    sylabel->setText(tr("Lon:"));
    if (xybox->currentIndex() == 1) {
      slat = QString::number(lat, 'f', 5).rightJustified(6);
      slon = QString::number(lon, 'f', 5).rightJustified(6);
    } else {
      slat = diutil::formatLatitudeDegMin(lat);
      slon = diutil::formatLongitudeDegMin(lon);
    }
  } else if (xybox->currentIndex() == 4){
    sxlabel->setText(tr("  Mark:"));
    sylabel->setText(tr("Window:"));
    slat = QString::number(lat);
    slon = QString::number(lon);
  } else if (xybox->currentIndex() == 5){
    sxlabel->setText(tr("  Vert:"));
    sylabel->setText(tr(" Horiz:"));
    slat = QString::number(lat);
    slon = QString::number(lon);
  } else {
    sxlabel->setText("  X:");
    sylabel->setText("  Y:");
    slat = QString::number(lat).rightJustified(6);
    slon = QString::number(lon).rightJustified(6);
  }

  latlabel->setText(slat);
  lonlabel->setText(slon);
}

void StatusGeopos::undefPosition()
{
  latlabel->setText("");
  lonlabel->setText("");
}
