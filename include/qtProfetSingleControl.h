#ifndef QTPROFETSINGLECONTROL_H_
#define QTPROFETSINGLECONTROL_H_
/*
  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <vector>
#include <map>

#include <qUtilities/miSliderWidget.h>
#include <QPushButton>
#include <QWidget>

#include <puTools/miTime.h>
#include <puTools/miString.h>
#include <profet/fetObject.h>


using namespace std;

class ProfetSingleControl : public QFrame {
  Q_OBJECT
private:
  map<miString,miSliderWidget*> slider;
  map<miString,float>           scale;
  QPushButton                  *button;
  fetObject::TimeValues         data;
  int                           column;
  bool                          isSunken;
  void                          checkState();
  bool                          isprocessed;

  
public:
  ProfetSingleControl(QWidget *p, fetObject::TimeValues, int col);
  bool   isParent() const  { return data.isParent();} 
  void   setValue(miString par, float value);
  void   resetValue(miString par);
  miTime time() const {return data.validTime;}
  fetObject::TimeValues Data() const {return data;}
  float  value(miString par)  { return data.parameters[par];}
  float  zero(miString  par)  { return data.valuesForZeroImpact[par];}
  void   set(fetObject::TimeValues);
 
  // empty processed removes object
  void  processed(miString id);
 
  
public slots:
  void buttonPressed();
  void valueChanged(float,miString);
  void valueChangedBySlider(float v, miString par);
signals:
  void buttonAtPressed(int);
  void pushundo();
  
};



#endif /*QTPROFETSINGLECONTROL_H_*/
