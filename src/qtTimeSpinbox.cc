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
#include <iostream>
#include <qtTimeSpinbox.h>


TimeSpinbox::TimeSpinbox(bool w_s, QWidget* parent, const char* name):
  QSpinBox(-INT_MAX,INT_MAX,1800,parent,name), with_sec(w_s)
{
  ref= miTime::nowTime();
  setValue(0);
}

miTime TimeSpinbox::Time()
{
  return time;
}


void TimeSpinbox::setTime(const miTime& t)
{
  if (!t.undef() && !ref.undef()){
    time= t;
    int v= miTime::secDiff(time, ref);
    //   cerr << "setTime:" << t << " value:" << v << endl;
    setValue(v);
  }
}

QString TimeSpinbox::mapValueToText( int value )
{
  time= ref;
  if (!time.undef()){
    time.addSec(value);
    //     cerr << "mapValueToText: value:" << value
    // 	 << " :" << time.isoTime() << endl;
    miString s= time.isoTime();
    if (!with_sec) s= s.substr(0,16);
    return QString(s.cStr());
  } else {
//     cerr << "mapValueToText: udefinert" << endl;
    return tr("undefined");
  }
}

int TimeSpinbox::mapTextToValue( bool* ok )
{
  miString s= text().latin1();
  if (!with_sec) s+= ":00";
  if (!miTime::isValid(s)){
    *ok= false;
//     cerr << "mapTextToValue: not legal time:" << s << endl;
    return 0;
  } else {
    time= miTime(s);
    *ok= true;
//     cerr << "mapTextToValue: text:" << s << " value:"
// 	 << miTime::minDiff(time, ref) << endl;
    return miTime::secDiff(time, ref);
  }
}
