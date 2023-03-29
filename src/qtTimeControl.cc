/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "qtTimeControl.h"
#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>


TimeControl::TimeControl(QWidget* parent)
  : QDialog( parent)
{
  QGroupBox* frame= new QGroupBox("Time Control", this);

  timerangeCheckBox = new  QCheckBox( tr("Time interval"),frame);
  timerangeCheckBox->setChecked(false);
  timerangeCheckBox->setToolTip(tr("Use time interval limits"));
  connect(timerangeCheckBox,SIGNAL(clicked(bool)),SLOT(timerangeCheckBoxClicked()));
  connect(timerangeCheckBox,SIGNAL(toggled(bool)),SLOT(minmaxSlot()));

  QLabel* startLabel= new QLabel(tr("Start"),frame);
  startLabel->setMinimumSize(startLabel->sizeHint());

  QLabel* stopLabel= new QLabel(tr("Stop"),frame);
  stopLabel->setMinimumSize(stopLabel->sizeHint());

  startTimeLabel= new QLabel("0000-00-00 00:00:00",frame);
  startTimeLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  startTimeLabel->setMinimumSize(startTimeLabel->sizeHint());

  stopTimeLabel= new QLabel("0000-00-00 00:00:00",frame);
  stopTimeLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  stopTimeLabel->setMinimumSize(stopTimeLabel->sizeHint());

  startSlider= new QSlider( Qt::Horizontal, frame);
  startSlider->setMinimumWidth(150);

  stopSlider= new QSlider( Qt::Horizontal, frame);
  stopSlider->setMinimumWidth(150);

  timeoffsetCheckBox = new  QCheckBox( tr("Time offset"), frame);
  timeoffsetCheckBox->setChecked(false);
  timeoffsetCheckBox->setToolTip(tr("Use time interval from latest timestamp"));
  connect(timeoffsetCheckBox,SIGNAL(clicked(bool)),SLOT(timeoffsetCheckBoxClicked()));
  connect(timeoffsetCheckBox,SIGNAL(toggled(bool)),SLOT(minmaxSlot()));

  QLabel* offsetLabel= new QLabel(tr("Offset"), frame);
  offsetLabel->setMinimumSize(offsetLabel->sizeHint());

  offsetTimeLabel= new QLabel("0", frame);
  offsetTimeLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  offsetTimeLabel->setMinimumSize(offsetTimeLabel->sizeHint());

  offsetSlider= new QSlider( Qt::Horizontal, frame);
  offsetSlider->setMinimumWidth(150);

  connect( startSlider, SIGNAL( valueChanged(int)),SLOT(StartValue(int)));
  connect( stopSlider, SIGNAL( valueChanged(int)),SLOT(StopValue(int)));
  connect( offsetSlider, SIGNAL( valueChanged(int)),SLOT(OffsetValue(int)));
  connect( startSlider, SIGNAL( sliderReleased()),SLOT(minmaxSlot()));
  connect( stopSlider, SIGNAL( sliderReleased()),SLOT(minmaxSlot()));
  connect( offsetSlider, SIGNAL( sliderReleased()),SLOT(minmaxSlot()));

  QVBoxLayout* timeLayout = new QVBoxLayout();
  timeLayout->addWidget( timerangeCheckBox );
  timeLayout->addWidget( startLabel );
  timeLayout->addWidget( startTimeLabel );
  timeLayout->addWidget( startSlider );
  timeLayout->addWidget( stopLabel );
  timeLayout->addWidget( stopTimeLabel );
  timeLayout->addWidget( stopSlider );
  timeLayout->addWidget( timeoffsetCheckBox );
  timeLayout->addWidget( offsetLabel );
  timeLayout->addWidget( offsetTimeLabel );
  timeLayout->addWidget( offsetSlider );


  QVBoxLayout* vblayout = new QVBoxLayout( frame);
  vblayout->addLayout( timeLayout );

  QHBoxLayout* timerangelayout = new QHBoxLayout();
  timerangelayout->addWidget(frame);

  QLabel* timeoutLabel = new QLabel(tr("Animation speed (sec):"), this);
  timeoutBox= new QComboBox(this);
  for (int f10 = 0; f10 < 21; ++f10) {
    timeoutBox->addItem(QString::number(0.1*f10, ' ', 1));
  }
  timeoutBox->setCurrentIndex(2);

  connect(timeoutBox, SIGNAL(highlighted(int)), SLOT(timeoutSlot(int)));

  //init dataname
  dataname.resize(8);
  dataname[0] = "field";
  dataname[1] = "sat";
  dataname[2] = "obs";
  dataname[3] = "obj";
  dataname[4] = "vprof";
  dataname[5] = "vcross";
  dataname[6] = "spectrum";
  dataname[7] = "product";

  QLabel* dataLabel = new QLabel(tr("Data basis for time slider:"),this);
  dataBox = new QComboBox(this);
  dataBox->addItem(tr("Field"));
  dataBox->addItem(tr("Satellite"));
  dataBox->addItem(tr("Observations"));
  dataBox->addItem(tr("Objects"));
  dataBox->addItem(tr("Vertical profiles"));
  dataBox->addItem(tr("Vertical cross-sections"));
  dataBox->addItem(tr("Wave spectra"));
  dataBox->addItem(tr("Products"));

  connect(dataBox, SIGNAL( activated(int)),SLOT(dataSlot(int)));

  QPushButton* hideButton = new QPushButton(tr("Hide"),this);
  connect(hideButton, SIGNAL(clicked()), SIGNAL(timecontrolHide()));

  QVBoxLayout* vlayout=new QVBoxLayout(this);
  vlayout->addLayout( timerangelayout );
  vlayout->addSpacing(5);
  vlayout->addWidget( timeoutLabel );
  vlayout->addWidget( timeoutBox );
  vlayout->addWidget( dataLabel );
  vlayout->addWidget( dataBox );
  vlayout->addSpacing(5);
  vlayout->addWidget( hideButton );
  vlayout->activate();

  setTimes(plottimes_t());
}

void TimeControl::timeoffsetCheckBoxClicked()
{
  timerangeCheckBox->setChecked(false);
}

void TimeControl::timerangeCheckBoxClicked()
{
  timeoffsetCheckBox->setChecked(false);
}

void TimeControl::setTimes(const plottimes_t& times)
{
  int n= times.size();
  int m= m_times.size();
  //try to remeber old limits
  bool resetSlider=false;
  miutil::miTime start,stop,offset;
  if (m>0 && n>0
      && (startSlider->value()>0 || stopSlider->value() <m-1 || offsetSlider->value()>0))
  {
    resetSlider=true;
    start= m_times[startSlider->value()];
    stop= m_times[stopSlider->value()];
    offset= m_times[offsetSlider->value()];
  }

  //reset times
  if (times.size()>0) {
    m_times = std::vector<miutil::miTime>(times.begin(), times.end());
  } else {
    m_times.clear();
    m_times.push_back(miutil::miTime::nowTime());
  }

  //reset sliders
  n= m_times.size() - 1;
  startSlider->setRange(0,n);
  stopSlider->setRange(0,n);
  offsetSlider->setRange(0,n);
  if (resetSlider) {
    int i = n;
    while (i > 0 && m_times[i] > start)
      i--;
    StartValue(i);
    i=0;
    while (i < n && m_times[i] < stop)
      i++;
    StopValue(i);
    // no need to reset offset
    if (offsetSlider->value() > 0) {
      minmaxSlot();
    }
  } else {
    StartValue(0);
    StopValue(n);
    OffsetValue(0);
  }
}


void TimeControl::StartValue(int v)
{
  startSlider->setValue(v);
  if (v > stopSlider->value())
    StopValue(v);
  startTimeLabel->setText(QString::fromStdString(m_times[v].isoTime()));
}

void TimeControl::StopValue(int v)
{
  stopSlider->setValue(v);
  if (v < startSlider->value())
    StartValue(v);
  stopTimeLabel->setText(QString::fromStdString(m_times[v].isoTime()));
}

void TimeControl::OffsetValue(int v)
{
  offsetSlider->setValue(v);
  int hour = miutil::miTime::hourDiff(m_times[v], m_times[0]);
  offsetTimeLabel->setText(QString::number(hour));
}

void TimeControl::timeoutSlot(int i)
{
  float val = i / 10.;
  emit timeoutChanged(val);
}

void TimeControl::dataSlot(int i)
{
  emit data(dataname[i]);
}

void TimeControl::minmaxSlot()
{
  int n = m_times.size();
  if(n==0)
    return;

  if (timerangeCheckBox->isChecked()) {
    int istart = startSlider->value();
    int istop = stopSlider->value();

    if (istart <= 0 && istop >= n-1) {
      emit clearMinMax();
    } else {
      miutil::miTime start= m_times[istart];
      miutil::miTime stop= m_times[istop];
      emit minmaxValue(start, stop);
    }
  } else if (timeoffsetCheckBox->isChecked()) {
    int ioffset = offsetSlider->value();
    // Offset is enabled
    miutil::miTime start= m_times[n - ioffset - 1];
    miutil::miTime stop= m_times[n - 1];
    emit minmaxValue(start, stop);
  }
  else {
    emit clearMinMax();
  }
}


void TimeControl::useData(const std::string& type, int id)
{
  int n= dataname.size();
  for(int i=0;i<n;i++){
    if (dataname[i]==type) {
      dataBox->setCurrentIndex(i);
      emit data(type);
      return;
    }
  }
  //new dataname
  external_id[id]=type;
  dataname.push_back(type);
  dataBox->addItem(type.c_str());
  dataBox->setCurrentIndex(dataBox->count()-1);
  emit data(type);
}

std::vector<std::string> TimeControl::deleteType(int id)
{
  //id=-1 means remove all external types
  std::vector<std::string>::iterator p = dataname.begin();
  std::map<int, std::string>::iterator q = external_id.begin();
  std::map<int, std::string>::iterator qend = external_id.end();

  std::vector<std::string> type;
  std::string currentDataname;
  if (dataBox->currentIndex() >=0) {
    currentDataname = dataname[dataBox->currentIndex()];
  }

  for (;q!=qend;q++) {
    if (id>-1 && q->first!=id)
      continue;
    int i=0;
    std::vector<std::string>::iterator pend = dataname.end();
    while (p!=pend && q->second != *p) {
      p++;
      i++;
    }
    if (p!=pend) {
      type.push_back(*p);
      dataname.erase(p);
      dataBox->removeItem(i);
      if (*p == currentDataname) {
        currentDataname = "";
      }
      if (id>-1) {  //remove id from external_id
        external_id.erase(q);
        break;
      }
    }
  }

  if (id==-1)
    external_id.clear();

  if (currentDataname != "")
    useData(currentDataname);
  else
    useData("field");

  return type;
}
