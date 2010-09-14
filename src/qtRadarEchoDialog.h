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
#ifndef RADARECHODIALOG_H
#define RADARECHODIALOG_H

#include <QDialog>
#include <QObject>

#include <puTools/miString.h>
#include <vector>
#include "diController.h"
#include "qtTimeSlider.h"

using namespace std;

class QComboBox;
class QListWidget;
class QPushButton;
class QSpinBox;
class QLabel;
class GeoPosLineEdit;
class QCheckBox;

/**

  \brief Dialogue for radar echo speed and distance calculation
   
   -select coords in for different timeslots in a radar och satellite image

*/
class RadarEchoDialog: public QDialog
{
  Q_OBJECT

public:

  RadarEchoDialog( QWidget* parent,Controller* llctrl);

  ///add position to list of positions
  void mapPos(float lat, float lon);
  //send all positions to TrajectoryPlotdiObsDi
  void sendAllPositions();

  vector<miutil::miString> writeLog();
  void readLog(const vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion, const miutil::miString& logVersion);
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
    int radius;
    miutil::miString numPos;
    miutil::miTime time;
  };
  vector<posStruct> positionVector;
   
  //init QT stuff
  vector<Colour::ColourInfo> colourInfo;
  vector<miutil::miString> linetypes;

  //qt widget
  QLabel* fieldName;
  QComboBox* colbox;
  QComboBox* lineWidthBox;
  QComboBox* lineTypeBox;
  GeoPosLineEdit* edit;
  QListWidget* posList;
  QCheckBox* posButton;
  QSpinBox* radiusSpin;
  QSpinBox* timeSpin;
  QPushButton* deleteButton;
  QPushButton* deleteAllButton;
  QPushButton* startCalcButton;
  QComboBox* numposBox;
  

  //functions
  miutil::miString makeString();
  void update_posList(float lat, float lon, miutil::miTime t, int index);
  void calculateVelocity();

private slots:
  void posButtonToggled(bool);
  void deleteClicked();
  void deleteAllClicked();
  void startCalcButtonClicked();
  void helpClicked();
  void printClicked();
  void applyhideClicked();
  void quitClicked();
  void editDone();
  double ArcInRadians(double lat1, double lon1, double lat2, double lon2);
  double DistanceInMeters(double lat1, double lon1, double lat2, double lon2);
  

  
 public slots:
 void showplus();

signals:
  void markRadePos(bool);
  void RadeHide();
  void updateRadarEchos();
  void showsource(const miutil::miString, const miutil::miString="");  
};

#endif
