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

#include <qtTimeControl.h>

#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qtUtility.h>
#include <iostream>



TimeControl::TimeControl(QWidget* parent)
  : QDialog( parent) {

  //m_font= QFont( IQ.fontName.c_str(), IQ.fontSize, IQ.fontWeight );
  m_font= QFont( "Helvetica", 12, 75 );

  QFrame* frame= new QFrame(this);
  frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

  timerangeCheckBox = new  QCheckBox( tr("Time interval"),frame);
  timerangeCheckBox->setChecked(false);
  timerangeCheckBox->setToolTip(tr("Use time interval limits")); 
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

  connect( startSlider, SIGNAL( valueChanged(int)),SLOT(StartValue(int)));
  connect( stopSlider, SIGNAL( valueChanged(int)),SLOT(StopValue(int)));
  connect( startSlider, SIGNAL( sliderReleased()),SLOT(minmaxSlot()));
  connect( stopSlider, SIGNAL( sliderReleased()),SLOT(minmaxSlot()));

  QVBoxLayout* timeLayout = new QVBoxLayout(); 
  timeLayout->addWidget( timerangeCheckBox );
  timeLayout->addWidget( startLabel );
  timeLayout->addWidget( startTimeLabel );
  timeLayout->addWidget( startSlider );
  timeLayout->addWidget( stopLabel );
  timeLayout->addWidget( stopTimeLabel );
  timeLayout->addWidget( stopSlider );

  QVBoxLayout* vblayout = new QVBoxLayout( frame);
  vblayout->addLayout( timeLayout );

  QHBoxLayout* timerangelayout = new QHBoxLayout();
  timerangelayout->addWidget(frame);

  QLabel* timeoutLabel = new QLabel(tr("Animation speed (sec):"), this);
  
  timeoutBox= new QComboBox(this);
  for(float f=0.2; f<2.1; f+=0.1){
    miutil::miString text(f,2);
    timeoutBox->addItem(text.cStr());
  }

  connect(timeoutBox, SIGNAL( highlighted(int)), SLOT(timeoutSlot(int)));

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
  connect(hideButton,SIGNAL(clicked()),SIGNAL(timecontrolHide()));

  // Create a horizontal frame line
//   QFrame *line = new QFrame( this );
//   line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

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
  //  vlayout->freeze();

  vector<miutil::miTime> t;
  setTimes( t );

}


void TimeControl::setTimes( vector<miutil::miTime>& times ){

  int n= times.size();
  int m= m_times.size();
  //try to remeber old limits
  bool resetSlider=false;
  miutil::miTime start,stop;
  if(m>0 && n>0 && (startSlider->value()>0 || stopSlider->value() <m-1)){
    resetSlider=true;
    start= m_times[startSlider->value()];
    stop= m_times[stopSlider->value()];
  }

  //reset times
  if (times.size()>0) {
    m_times= times;
  } else {
    m_times.clear();
    miutil::miTime t= miutil::miTime::nowTime();
    m_times.push_back( t );
  }

  //reset sliders
  n= m_times.size() - 1;
  startSlider->setRange(0,n);
  stopSlider->setRange(0,n);
  if(resetSlider){
    int i = n;
    while(i>0 && times[i]>start) i--;
    StartValue(i);
    i=0;
    while(i<n && times[i]<stop) i++;
    StopValue(i);
  } else{
    StartValue(0);
    StopValue(n);
  } 
}


void TimeControl::StartValue( int v ){
  startSlider->setValue( v );
  if ( v > stopSlider->value() )
    StopValue(v); 
  startTimeLabel->setText(m_times[v].isoTime().cStr());

  return;
}


void TimeControl::StopValue( int v ){
  stopSlider->setValue( v );
  if ( v < startSlider->value())
    StartValue(v);
  stopTimeLabel->setText(m_times[v].isoTime().cStr());
  return;
}

void TimeControl::timeoutSlot(int i){

  float val= i/10.+0.2;
  emit timeoutChanged(val);

}
void TimeControl::dataSlot(int i){

  emit data(dataname[i]);

}

void TimeControl::minmaxSlot(){

  int n = m_times.size();
  if(n==0) return;

  if(timerangeCheckBox->isChecked()){

    int istart, istop;    
    istart= startSlider->value();
    istop=  stopSlider->value(); 
    if(istart<=0 && istop>=n-1){
      emit clearMinMax();
    } else {
      miutil::miTime start= m_times[istart];
      miutil::miTime stop= m_times[istop];
      emit minmaxValue(start,stop);
    }
  } else {
    emit clearMinMax();
  }
    
}


void TimeControl::useData(miutil::miString type, int id){

  int n= dataname.size();
  for(int i=0;i<n;i++){
    if(dataname[i]==type){
      dataBox->setCurrentIndex(i);
      emit data(type);
      return;
    }
  }
  //new dataname
  external_id[id]=type;
  dataname.push_back(type);
  dataBox->addItem(type.cStr());
  dataBox->setCurrentIndex(dataBox->count()-1);
  emit data(type);
  
}


vector<miutil::miString> TimeControl::deleteType(int id)
{
  //id=-1 means remove all external types
  //  cerr <<"TimeControl::deleteType:"<<id<<endl;
  vector<miutil::miString>::iterator p = dataname.begin();
  map<int,miutil::miString>::iterator q = external_id.begin();
  map<int,miutil::miString>::iterator qend = external_id.end();
  
  vector<miutil::miString> type;

  for(;q!=qend;q++){
    if(id>-1 && q->first!=id) continue;
    int i=0;
    vector<miutil::miString>::iterator pend = dataname.end();
    while( p!=pend && q->second != *p) {
      p++;
      i++;
    }
    if(p!=pend){ 
      type.push_back(*p);
      dataname.erase(p);
      dataBox->removeItem(i);
      if(id>-1){  //remove id from external_id
	external_id.erase(q);
	break;
      }
    }
  }
  
  if( id==-1 )
    external_id.clear();

  useData("field");

  return type;

}				       

