/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: qtTrajectoryDialog.h 1008 2009-01-20 06:59:41Z johan.karlsteen@smhi.se $

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
#ifndef MEASUREMENTSDIALOG_H
#define MEASUREMENTSDIALOG_H

#include "diController.h"

#include <QDialog>
#include <QObject>

#include <vector>

class QPushButton;
class QLabel;
class QCheckBox;

/**
  \brief Dialog for speed, distance and bearing calculation
   
   -select coords in  different timeslots and calculate speed
*/
class MeasurementsDialog: public QDialog
{
  Q_OBJECT

public:

  MeasurementsDialog( QWidget* parent,Controller* llctrl);

  ///add position to list of positions
  void mapPos(float lat, float lon);
  void sendAllPositions();

  bool close(bool alsoDelete);
  bool hasFocus();

protected:
  QLabel *startlabel1;
  QLabel *endlabel1;
  QLabel *latbox1;
  QLabel *lonbox1;
  QLabel *latbox2;
  QLabel *lonbox2;
  QLabel *latlabel1;
  QLabel *latlabel2;
  QLabel *lonlabel1;
  QLabel *lonlabel2;
  QLabel *speedlabel1;
  QLabel *speedlabel2;
  QLabel *speedlabel3;
  QLabel *speedbox1;
  QLabel *speedbox2;
  QLabel *speedbox3;
  QLabel *distancelabel;
  QLabel *distancebox;
  QLabel *bearinglabel;
  QLabel *bearingbox;
  QLabel *timelabel1;
  QLabel *timelabel2;
  QLabel *timebox1;
  QLabel *timebox2;
  QLabel *datelabel1;
  QLabel *datelabel2;
  QLabel *datebox1;
  QLabel *datebox2;
  void focusInEvent( QFocusEvent * );
  void focusOutEvent( QFocusEvent * );
  void focusChanged( QWidget * old, QWidget * now );
  
  
private:
  Controller* contr;
 
  struct posStruct {
    double lat,lon;
    miutil::miTime time;
  };
  std::vector<posStruct> positionVector;

  //functions
  std::string makeString();
  void update_posList(float lat, float lon, miutil::miTime t, int index);
  void calculate();

private slots:
  void deleteClicked();
  void helpClicked();
  void quitClicked();
  

  
 public slots:
 void showplus();

signals:
  void markMeasurementsPos(bool);
  void MeasurementsHide();
  void updateMeasurements();
  void showsource(const std::string, const std::string="");  
};

#endif
