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

#include <iostream>
#include <climits>
#include "qtTimeSpinbox.h"


TimeSpinbox::TimeSpinbox(bool w_s, QWidget* parent):
  QSpinBox(parent), with_sec(w_s)
{
  ref= miutil::miTime::nowTime();
  setRange(-INT_MAX,INT_MAX);
  setSingleStep(1800);
  setValue(0);
}

miutil::miTime TimeSpinbox::Time()
{
  miutil::miTime time= ref;
  time.addSec(value());
  return time;
}


void TimeSpinbox::setTime(const miutil::miTime& t)
{
  if (!t.undef() && !ref.undef()){
    int v= miutil::miTime::secDiff(t, ref);
    setValue(v);
  }
}


QString TimeSpinbox::textFromValue( int value ) const
{
  miutil::miTime time= ref;
  if (!time.undef()){
    time.addSec(value);
    std::string s= time.isoTime();
    if (!with_sec) s= s.substr(0,16);
   return QString(s.c_str());
  } else {
    return tr("undefined");
  }
}

int TimeSpinbox::valueFromText( const QString& text ) const 
{
  std::string s= text.toStdString();
  if (!with_sec) s+= ":00";
  if (!miutil::miTime::isValid(s)){
    return 0;
  } else {
    miutil::miTime time= miutil::miTime(s);
    return miutil::miTime::secDiff(time, ref);
  }
}
