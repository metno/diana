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

#include "qtShowSatValues.h"

#include <puTools/miStringFunctions.h>

#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QHBoxLayout>
#include <QFrame>

#include <iomanip>
#include <sstream>
#include <string>

ShowSatValues::ShowSatValues(QWidget* parent)
  : QWidget(parent)
{
  QHBoxLayout* thlayout = new QHBoxLayout( this);
  thlayout->setMargin(1);
  thlayout->setSpacing(5);

  channelbox = new QComboBox(this);
  channelbox->setMinimumWidth(channelbox->sizeHint().width());
  connect(channelbox,SIGNAL(activated(int)),SLOT(channelChanged(int)));
  thlayout->addWidget(channelbox);

  chlabel= new QLabel("XXXXXXXXXXXX",this);
  chlabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  chlabel->setMinimumSize(chlabel->sizeHint());
  chlabel->setText("    ");
  thlayout->addWidget(chlabel,0);
}

void ShowSatValues::channelChanged(int index)
{
  if( index < int(tooltip.size()) && index > -1 ){
    std::string tip = tooltip[index];
    miutil::replace(tip, '|',' ');
    channelbox->setToolTip(QString::fromStdString(tip));
  }
}

void ShowSatValues::SetChannels(const std::vector<std::string>& channel)
{
  int nch = channel.size();

  //  cerr << " ShowSatValues::SetChannels:"<<channel.size()<<endl;
  //try to remember currentItem
  int index = -1;
  if(tooltip.size( )> 0 && channelbox->count()>0  ){
    std::string currentText = tooltip[channelbox->currentIndex()];
    int i = 0;
    while(i<nch && channel[i]!=currentText) i++;
    if(i<nch) index=i;
  }

  //refresh channelbox
  channelbox->clear();
  for(int i=0;i<nch;i++){
    //    cerr <<"channel:"<<i<<"  "<<channel[i]<<endl;
    std::vector<std::string> token = miutil::split(channel[i], 0, "|");
    if(token.size()==2){
      channelbox->addItem(QString::fromStdString(token[1]));
      //if no currentItem, use channel"4"
      if(index == -1 && miutil::contains(token[1], "4")) index = i;
    }
  }
  tooltip = channel;

  //reset currentItem
  if(index>-1)
    channelbox->setCurrentIndex(index);

  //update tooltip
  if(nch > 0)
    channelChanged(channelbox->currentIndex());
  else
    channelbox->setToolTip( QString());
}

void ShowSatValues::ShowValues(const std::vector<SatValues> &satval)
{
  if (not channelbox->count())
    return;

  const int n = satval.size();
  int i=0;
  while(i<n && satval[i].channel != tooltip[channelbox->currentIndex()])
    i++;

  std::ostringstream svalue;
  if (i < n){
    if (satval[i].value < -999)
      svalue << satval[i].text;
    else if (miutil::contains(satval[i].channel, "Infrared")
        || miutil::contains(satval[i].channel, "IR_CAL")
        || miutil::contains(satval[i].channel, "TEMP"))
      svalue << std::setprecision(1) << std::setiosflags(std::ios::fixed)
             << satval[i].value <<"\260C"; // latin1 degree sign
    else
      svalue << std::setprecision(2) << std::setiosflags(std::ios::fixed)
             << satval[i].value;
  }
  chlabel->setText(QString::fromStdString(svalue.str()));
}
