/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "qtMeasurementsDialog.h"

#include "util/qstring_util.h"

#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QToolTip>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include <cmath>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.MeasurementsDialog"
#include <miLogger/miLogging.h>


static const float RAD_TO_DEG = 180 / M_PI;

MeasurementsDialog::MeasurementsDialog( QWidget* parent, Controller* llctrl)
  : QDialog(parent), contr(llctrl)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle(tr("Measurements"));
  setFocusPolicy(Qt::StrongFocus);

  //push button to delete last pos
  QPushButton * deleteButton = new QPushButton(tr("Clear"), this );
  connect( deleteButton, SIGNAL(clicked()),SLOT(deleteClicked()));

  //push button to hide dialog
  QPushButton* Hide = new QPushButton(tr("Hide"), this);
  connect( Hide, SIGNAL(clicked()), SIGNAL(MeasurementsHide()));

  //push button to quit, deletes all trajectoryPlot objects
  QPushButton* quit = new QPushButton(tr("Quit"), this);
  connect(quit, SIGNAL(clicked()), SLOT( quitClicked()) );

  QFrame *line1 = new QFrame( this );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  QFrame *line2 = new QFrame( this );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  QFrame *line3 = new QFrame( this );
  line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );


  startlabel1= new QLabel(tr("Start:"),this);
  startlabel1->setFrameStyle( QFrame::Panel);
  startlabel1->setMinimumSize(startlabel1->sizeHint());


  latlabel1= new QLabel(tr("Lat:"),this);
  latlabel1->setFrameStyle( QFrame::Panel);
  latlabel1->setMinimumSize(latlabel1->sizeHint());

  latbox1= new QLabel("00°00'" + diutil::latitudeNorth(), this); // was "00<deg>00'N"
  latbox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  latbox1->setMinimumSize(latbox1->sizeHint());

  lonlabel1= new QLabel("Lon:",this);
  lonlabel1->setFrameStyle( QFrame::Panel);
  lonlabel1->setMinimumSize(lonlabel1->sizeHint());

  lonbox1= new QLabel("00°00'" + diutil::longitudeWest(), this);
  lonbox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  lonbox1->setMinimumSize(lonbox1->sizeHint());


  datelabel1= new QLabel(tr("Date:"),this);
  datelabel1->setFrameStyle( QFrame::Panel);
  datelabel1->setMinimumSize(datelabel1->sizeHint());

  datebox1= new QLabel("0000-00-00",this);
  datebox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  datebox1->setMinimumSize(datebox1->sizeHint());

  timelabel1= new QLabel(tr("Time:"),this);
  timelabel1->setFrameStyle( QFrame::Panel);
  timelabel1->setMinimumSize(timelabel1->sizeHint());

  timebox1= new QLabel("00:00",this);
  timebox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  timebox1->setMinimumSize(timebox1->sizeHint());

  endlabel1= new QLabel(tr("End:"),this);
  endlabel1->setFrameStyle( QFrame::Panel);
  endlabel1->setMinimumSize(endlabel1->sizeHint());


  latlabel2= new QLabel(tr("Lat:"),this);
  latlabel2->setFrameStyle( QFrame::Panel);
  latlabel2->setMinimumSize(latlabel2->sizeHint());

  latbox2= new QLabel(tr("00°00'N"),this);
  latbox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  latbox2->setMinimumSize(latbox2->sizeHint());

  lonlabel2= new QLabel(tr("Lon:"),this);
  lonlabel2->setFrameStyle( QFrame::Panel);
  lonlabel2->setMinimumSize(lonlabel2->sizeHint());

  lonbox2= new QLabel(tr("00°00'W"),this);
  lonbox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  lonbox2->setMinimumSize(lonbox2->sizeHint());

  datelabel2= new QLabel(tr("Date:"),this);
  datelabel2->setFrameStyle( QFrame::Panel);
  datelabel2->setMinimumSize(datelabel2->sizeHint());

  datebox2= new QLabel("0000-00-00",this);
  datebox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  datebox2->setMinimumSize(datebox2->sizeHint());

  timelabel2= new QLabel(tr("Time:"),this);
  timelabel2->setFrameStyle( QFrame::Panel);
  timelabel2->setMinimumSize(timelabel2->sizeHint());

  timebox2= new QLabel("00:00",this);
  timebox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  timebox2->setMinimumSize(timebox2->sizeHint());

  speedlabel1= new QLabel(tr("Velocity:"),this);
  speedlabel1->setFrameStyle( QFrame::Panel);
  speedlabel1->setMinimumSize(speedlabel1->sizeHint());

  speedlabel2= new QLabel(tr("Velocity:"),this);
  speedlabel2->setFrameStyle( QFrame::Panel);
  speedlabel2->setMinimumSize(speedlabel2->sizeHint());

  speedlabel3= new QLabel(tr("Velocity:"),this);
  speedlabel3->setFrameStyle( QFrame::Panel);
  speedlabel3->setMinimumSize(speedlabel3->sizeHint());

  speedbox1= new QLabel("0 m/s",this);
  speedbox1->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  speedbox1->setMinimumSize(speedbox1->sizeHint());

  speedbox2= new QLabel("0 m/s",this);
  speedbox2->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  speedbox2->setMinimumSize(speedbox2->sizeHint());

  speedbox3= new QLabel("0 m/s",this);
  speedbox3->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  speedbox3->setMinimumSize(speedbox3->sizeHint());

  distancelabel= new QLabel(tr("Distance:"),this);
  distancelabel->setFrameStyle( QFrame::Panel);
  distancelabel->setMinimumSize(distancelabel->sizeHint());

  distancebox= new QLabel("0 km",this);
  distancebox->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  distancebox->setMinimumSize(distancebox->sizeHint());

  bearinglabel= new QLabel(tr("Bearing:"),this);
  bearinglabel->setFrameStyle( QFrame::Panel);
  bearinglabel->setMinimumSize(bearinglabel->sizeHint());

  bearingbox= new QLabel("0",this);
  bearingbox->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  bearingbox->setMinimumSize(bearingbox->sizeHint());

  //now create a grid layout
  QGridLayout* gridlayout = new QGridLayout(this);
  gridlayout->setColumnStretch(1,1);

  gridlayout->addWidget(startlabel1,          0,0);

  gridlayout->addWidget(latlabel1,          1,0);
  gridlayout->addWidget(latbox1,            1,1);
  gridlayout->addWidget(lonlabel1,          2,0);
  gridlayout->addWidget(lonbox1,            2,1);
  gridlayout->addWidget(datelabel1,         3,0);
  gridlayout->addWidget(datebox1,           3,1);
  gridlayout->addWidget(timelabel1,         4,0);
  gridlayout->addWidget(timebox1,           4,1);
  gridlayout->addWidget(endlabel1,          0,2);
  gridlayout->addWidget(latlabel2,          1,2);
  gridlayout->addWidget(latbox2,            1,3);
  gridlayout->addWidget(lonlabel2,          2,2);
  gridlayout->addWidget(lonbox2,            2,3);
  gridlayout->addWidget(datelabel2,         3,2);
  gridlayout->addWidget(datebox2,           3,3);
  gridlayout->addWidget(timelabel2,         4,2);
  gridlayout->addWidget(timebox2,           4,3);
  gridlayout->addWidget(line1,              5,0,1,4);
  gridlayout->addWidget(deleteButton,       6,0,1,4);//6,0,1,2);
  gridlayout->addWidget(line2,              7,0,1,4);
  gridlayout->addWidget(speedlabel1,        8,0,1,2);
  gridlayout->addWidget(speedbox1,          8,2,1,2);
  gridlayout->addWidget(speedlabel2,        9,0,1,2);
  gridlayout->addWidget(speedbox2,          9,2,1,2);
  gridlayout->addWidget(speedlabel3,        10,0,1,2);
  gridlayout->addWidget(speedbox3,          10,2,1,2);
  gridlayout->addWidget(distancelabel,      11,0,1,2);
  gridlayout->addWidget(distancebox,        11,2,1,2);
  gridlayout->addWidget(bearinglabel,      12,0,1,2);
  gridlayout->addWidget(bearingbox,        12,2,1,2);
  gridlayout->addWidget(line3,              13,0,1,4);
  gridlayout->addWidget( Hide,           16,0,1,2);
  gridlayout->addWidget( quit,           16,2,1,2);
}


void MeasurementsDialog::deleteClicked()
{
  //delete selected group of start positions

  positionVector.clear();

  latbox1->setText("00°00'" + diutil::latitudeNorth());
  lonbox1->setText("00°00'" + diutil::longitudeWest());
  latbox2->setText("00°00'" + diutil::latitudeNorth());
  lonbox2->setText("00°00'" + diutil::longitudeWest());
  datebox1->setText("0000-00-00");
  datebox2->setText("0000-00-00");
  timebox1->setText("00:00");
  timebox2->setText("00:00");
  speedbox1->setText("0 m/s");
  speedbox2->setText("0 km/h");
  speedbox3->setText(tr("0 knots"));
  distancebox->setText("0 km");

  contr->measurementsPos(std::vector<std::string>(1, "delete"));
  sendAllPositions();

  Q_EMIT updateMeasurements();
}


void MeasurementsDialog::calculate()
{
  //this slot is called when start calc. button pressed
  METLIBS_LOG_SCOPE();

  speedbox1->clear();
  speedbox2->clear();
  speedbox3->clear();
  distancebox->clear();
  bearingbox->clear();

  if (positionVector.size() == 2) {
    double start_lat = positionVector[0].lat;
    double stop_lat = positionVector[1].lat;
    double start_lon = positionVector[0].lon;
    double stop_lon = positionVector[1].lon;
    miutil::miTime start_time = positionVector[0].time;
    miutil::miTime stop_time = positionVector[1].time;
    METLIBS_LOG_INFO(LOGVAL(start_lat));
    METLIBS_LOG_INFO(LOGVAL(start_lon));
    METLIBS_LOG_INFO(LOGVAL(stop_lat));
    METLIBS_LOG_INFO(LOGVAL(stop_lon));
    METLIBS_LOG_INFO(LOGVAL(start_time));
    METLIBS_LOG_INFO(LOGVAL(stop_time));

    LonLat start_lonlat = LonLat::fromDegrees(start_lon,start_lat);
    LonLat stop_lonlat = LonLat::fromDegrees(stop_lon,stop_lat);
    double distance = start_lonlat.distanceTo(stop_lonlat);
    double distance_in_kilometers = distance/1000.0;
    METLIBS_LOG_INFO(LOGVAL(distance_in_kilometers)<< " km");
    QString distanceresult = QString::number(distance_in_kilometers, 'f', 2) + " km";
    distancebox->setText(distanceresult);

    double bearing = start_lonlat.bearingTo(stop_lonlat) * RAD_TO_DEG;
    METLIBS_LOG_INFO(LOGVAL(bearing));
    QString bearingresult = QString::number(bearing, 'f', 2);
    bearingbox->setText(bearingresult);

    if ( start_time != stop_time ) {
      int t = abs(miutil::miTime::secDiff(start_time, stop_time));
      double speed_in_ms = distance/t;
      double speed_in_kmh = speed_in_ms*3.6;
      double speed_in_knots = speed_in_kmh/1.852;
      METLIBS_LOG_INFO(LOGVAL(speed_in_ms)<< " m/s");
      METLIBS_LOG_INFO(LOGVAL(speed_in_kmh)<< " km/h");
      METLIBS_LOG_INFO(LOGVAL(speed_in_knots)<< " knots");

      QString speedresult1 = QString::number(speed_in_ms, 'f', 2) + " m/s";
      QString speedresult2 = QString::number(speed_in_kmh, 'f', 2) + " km/h";
      QString speedresult3 = QString::number(speed_in_knots, 'f', 2) + " " + tr("knots");

      speedbox1->setText(speedresult1);
      speedbox2->setText(speedresult2);
      speedbox3->setText(speedresult3);
    }
  }
  Q_EMIT updateMeasurements();
}


void MeasurementsDialog::quitClicked()
{
  contr->measurementsPos(std::vector<std::string>(1, "quit"));
  Q_EMIT markMeasurementsPos(false);
  Q_EMIT MeasurementsHide();
}


void MeasurementsDialog::helpClicked()
{
  // emit showsource("ug_messurements.html");
}

static void insertlatlon(std::ostream& str, float lat, float lon)
{
#if 0
  str << std::setw(5) << std::setprecision(2)<< std::setiosflags(std::ios::fixed);
  str << "latitudelongitude=" << lat << "," << lon;
#else
  lat = roundf(lat*60)/60;
  lon = roundf(lon*60)/60;
  str << "latitudelongitude=" << lat << "," << lon;
#endif
}

void MeasurementsDialog::mapPos(float lat, float lon)
{
  METLIBS_LOG_SCOPE();

  //Put this position in vector of positions
  posStruct pos;
  pos.lat=lat;
  pos.lon=lon;
  pos.time = contr->getPlotTime();

  if (positionVector.size() < 3) {
    positionVector.push_back(pos);
  }

  //Make string and insert in posList
  if ((positionVector.size() == 1) || (positionVector.size() == 2)) {
    update_posList(lat, lon, pos.time, positionVector.size());
  } else {
    return;
  }

  //  //Make string and send to measuremenPlot
  std::ostringstream str;
  insertlatlon(str, lat, lon);
  str << ' ';
  insertlatlon(str, lat, lon); // TODO why is the same lat-lon sent twice?
  str <<" numpos="<<1;
  str <<" time="<<pos.time;

  contr->measurementsPos(std::vector<std::string>(1, str.str()));

  Q_EMIT updateMeasurements();
}


void MeasurementsDialog::update_posList(float lat, float lon, const miutil::miTime& t, int index)
{
  METLIBS_LOG_SCOPE();
  const QString latstr = diutil::formatLatitudeDegMin(lat);
  const QString lonstr = diutil::formatLongitudeDegMin(lon);
  const QString datestr = QString::fromStdString(t.isoDate());
  const QString timestr = QString::fromStdString(t.isoClock());

  switch (index) {
  case 1:
    latbox1->setText(latstr);
    lonbox1->setText(lonstr);
    datebox1->setText(datestr);
    timebox1->setText(timestr);
    break;
  case 2:
    latbox2->setText(latstr);
    lonbox2->setText(lonstr);
    datebox2->setText(datestr);
    timebox2->setText(timestr);

    calculate();

    break;
  }
}


void MeasurementsDialog::sendAllPositions()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> vstr;

  for (const posStruct& p : positionVector) {
    std::ostringstream str;
    insertlatlon(str, p.lat, p.lon);
    vstr.push_back(str.str());
  }
  contr->measurementsPos(vstr);
  Q_EMIT updateMeasurements();
}


void MeasurementsDialog::showplus()
{
  METLIBS_LOG_SCOPE();
  this->show();

  Q_EMIT markMeasurementsPos(true);

  sendAllPositions();

  Q_EMIT updateMeasurements();
}

bool MeasurementsDialog::close(bool /*alsoDelete*/)
{
  Q_EMIT markMeasurementsPos(false);
  Q_EMIT MeasurementsHide();
  return true;
}


bool MeasurementsDialog::hasFocus()
{
  Q_EMIT markMeasurementsPos(true);
  return isActiveWindow();
}

void MeasurementsDialog::focusInEvent(QFocusEvent *)
{
  Q_EMIT markMeasurementsPos(true);
}

void MeasurementsDialog::focusOutEvent(QFocusEvent *)
{
  Q_EMIT markMeasurementsPos(false);
}

void MeasurementsDialog::focusChanged(QWidget* /*old*/, QWidget* /*now*/) {}
