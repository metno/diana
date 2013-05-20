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

#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <QGridLayout>

#include "qtUtility.h"
#include "diCommonTypes.h"
#include "diVcrossManager.h"
#include "qtVcrossSetup.h"

// initialize static members
bool               VcrossSetup::initialized= false;
vector<Colour::ColourInfo> VcrossSetup::m_cInfo; // all defined colours
int                VcrossSetup::nr_colors=0;
QColor*            VcrossSetup::pixcolor=0;
int                VcrossSetup::nr_linewidths=0;
int                VcrossSetup::nr_linetypes=0;
vector<std::string>   VcrossSetup::linetypes;

VcrossSetup::VcrossSetup( QWidget* parent, miutil::miString text,
			QGridLayout * glayout, int row, int options)
  : QObject(parent),name(text)
{
  //a Qobject with a checkbox,and up to three comboboxes
#ifdef DEBUGPRINT
  cout<<"VcrossSetup::VcrossSetup called"<<endl;
#endif

  //------------------------
  if (!initialized) {
  //------------------------

  // color types
      m_cInfo = Colour::getColourInfo();
  nr_colors= m_cInfo.size();
  pixcolor = new QColor[nr_colors];// this must exist in the objectlifetime
  for(int i=0; i<nr_colors; i++ ){
    pixcolor[i]=QColor( m_cInfo[i].rgb[0], m_cInfo[i].rgb[1],
			m_cInfo[i].rgb[2] );
  }

  // linewidths
  nr_linewidths= 12;

  // linetypes
  linetypes = Linetype::getLinetypeNames();
  nr_linetypes= linetypes.size();

  //------------------------
  initialized= true;
  }
  //------------------------

  int ncol = 0;

  if (options & useOnOff) {
    checkbox = new QCheckBox(text.c_str(),parent);
    label= 0;
    glayout->addWidget(checkbox,row,ncol);
    connect( checkbox, SIGNAL( toggled(bool)), SLOT( setChecked( bool) ));
    ncol++;
  } else{
    checkbox= 0;
    label = new QLabel(text.c_str(),parent);
    glayout->addWidget(label,row,ncol,Qt::AlignLeft);
    ncol++;
  }

  if (options & useColour) {
    colourbox = ComboBox(parent, pixcolor, nr_colors, true, 0 );
    glayout->addWidget(colourbox,row,ncol);
    ncol++;
  } else
    colourbox=0;

  if (options & useLineWidth) {
    linewidthbox =  LinewidthBox( parent, true);
    glayout->addWidget(linewidthbox,row,ncol);
    ncol++;
  } else
    linewidthbox=0;

  if (options & useLineType) {
    linetypebox = LinetypeBox( parent,true );
    glayout->addWidget(linetypebox,row,ncol);
    ncol++;
  } else
    linetypebox=0;

  if (options & useValue) {
    valuespinbox= new QSpinBox(parent);
    glayout->addWidget(valuespinbox,row,ncol);
    ncol++;
  } else
    valuespinbox= 0;

  if (options & useMinValue) {
    minvaluespinbox= new QSpinBox(parent);
    glayout->addWidget(minvaluespinbox,row,ncol);
    ncol++;
  } else
    minvaluespinbox= 0;

  if (options & useMaxValue) {
    maxvaluespinbox= new QSpinBox(parent);
    glayout->addWidget(maxvaluespinbox,row,ncol);
    ncol++;
  } else
    maxvaluespinbox= 0;

  if (minvaluespinbox && maxvaluespinbox) {
    connect( minvaluespinbox, SIGNAL(valueChanged(int)),
	                      SLOT( forceMaxValue(int)));
    connect( maxvaluespinbox, SIGNAL(valueChanged(int)),
	                      SLOT( forceMinValue(int)));
  }

  if (options & useTextChoice) {
    textchoicebox = new QComboBox(parent);
    //    if (ncol<2) ncol=2;
    glayout->addWidget(textchoicebox,row,ncol);
    ncol++;
  } else
    textchoicebox=0;

  if (options & useTextChoice2) {
    textchoicebox2 = new QComboBox(parent);
    if (ncol<2) ncol=2;
    glayout->addWidget(textchoicebox2,row,ncol);
    ncol++;
  } else
    textchoicebox2=0;

}


void VcrossSetup::setChecked(bool on)
{
  if (checkbox)               checkbox->setChecked(on);
  if (colourbox)             colourbox->setEnabled(on);
  if (linewidthbox)       linewidthbox->setEnabled(on);
  if (linetypebox)         linetypebox->setEnabled(on);
  if (valuespinbox)       valuespinbox->setEnabled(on);
  if (minvaluespinbox) minvaluespinbox->setEnabled(on);
  if (maxvaluespinbox) maxvaluespinbox->setEnabled(on);
  if (textchoicebox)     textchoicebox->setEnabled(on);
  if (textchoicebox2)   textchoicebox2->setEnabled(on);
}


bool VcrossSetup::isChecked()
{
  if (checkbox && checkbox->isChecked())
    return true;
  else
    return false;
}

Colour::ColourInfo VcrossSetup::getColour()
{
  Colour::ColourInfo sColour;
  if (colourbox){
    int index = colourbox->currentIndex();
    sColour = m_cInfo[index];
  }
  return sColour;
}


void VcrossSetup::setColour(const miutil::miString& colourString)
{
  int nr_colours = m_cInfo.size();
  for (int i = 0;i<nr_colours;i++){
    if (colourString==m_cInfo[i].name){
      if (colourbox && i<colourbox->count()) colourbox->setCurrentIndex(i);
      break;
    }
  }
}


void VcrossSetup::setLinetype(const miutil::miString& linetype)
{
  if (linetypebox) {
    int index= 0;
    while (index<nr_linetypes && linetypes[index]!=linetype) index++;
    if (index==nr_linetypes) index= 0;
    linetypebox->setCurrentIndex(index);
  }
}


miutil::miString VcrossSetup::getLinetype()
{
  miutil::miString sString;
  if (linetypebox) {
    int index = linetypebox->currentIndex();
    sString = linetypes[index];
  }
  return sString;
}


void VcrossSetup::setLinewidth(float linew)
{
  if (linewidthbox) {
    int index = int(linew - 1.0);
    if (index<0) index= 0;
    if (index>=nr_linewidths) index= nr_linewidths-1;
    linewidthbox->setCurrentIndex(index);
  }
}


float VcrossSetup::getLinewidth()
{
  float linew= 1.;
  if (linewidthbox)
    linew= linewidthbox->currentIndex() + 1.0;
  return linew;
}


void VcrossSetup::defineValue(int low, int high, int step, int value,
			      const miutil::miString& prefix,
			      const miutil::miString& suffix)
{
  if (valuespinbox) {
    valuespinbox->setMinimum(low);
    valuespinbox->setMaximum(high);
    valuespinbox->setSingleStep(step);
    valuespinbox->setValue(value);
    if (prefix.exists())
      valuespinbox->setPrefix(QString(prefix.c_str()));
    else if (suffix.exists())
      valuespinbox->setSuffix(QString(suffix.c_str()));
    valuespinbox->setValue(value);
  }
}


void VcrossSetup::setValue(int value)
{
  if (valuespinbox)
    valuespinbox->setValue(value);
}


int VcrossSetup::getValue()
{
  if (valuespinbox)
    return valuespinbox->value();
  else
    return 0;
}


void VcrossSetup::defineMinValue(int low, int high, int step, int value,
			         const miutil::miString& prefix,
			         const miutil::miString& suffix)
{
  if (minvaluespinbox) {
    minvaluespinbox->setMinimum(low);
    minvaluespinbox->setMaximum(high);
    minvaluespinbox->setSingleStep(step);
    minvaluespinbox->setValue(value);
    if (prefix.exists())
      minvaluespinbox->setPrefix(QString(prefix.c_str()));
    else if (suffix.exists())
      minvaluespinbox->setSuffix(QString(suffix.c_str()));
    minvaluespinbox->setValue(value);
  }
}


void VcrossSetup::setMinValue(int value)
{
  if (minvaluespinbox)
    minvaluespinbox->setValue(value);
}


int VcrossSetup::getMinValue()
{
  if (minvaluespinbox)
    return minvaluespinbox->value();
  else
    return 0;
}


void VcrossSetup::defineMaxValue(int low, int high, int step, int value,
			         const miutil::miString& prefix,
			         const miutil::miString& suffix)
{
  if (maxvaluespinbox) {
    maxvaluespinbox->setMinimum(low);
    maxvaluespinbox->setMaximum(high);
    maxvaluespinbox->setSingleStep(step);
    maxvaluespinbox->setValue(value);
    if (prefix.exists())
      maxvaluespinbox->setPrefix(QString(prefix.c_str()));
    else if (suffix.exists())
      maxvaluespinbox->setSuffix(QString(suffix.c_str()));
    maxvaluespinbox->setValue(value);
  }
}


void VcrossSetup::setMaxValue(int value)
{
  if (maxvaluespinbox)
    maxvaluespinbox->setValue(value);
}


int VcrossSetup::getMaxValue()
{
  if (maxvaluespinbox)
    return maxvaluespinbox->value();
  else
    return 0;
}


void VcrossSetup::forceMaxValue(int minvalue)
{
  if (maxvaluespinbox) {
    int step=  maxvaluespinbox->singleStep();
    int value= maxvaluespinbox->value();
    if (minvalue > value - step)
      maxvaluespinbox->setValue(value+step);
  }
}


void VcrossSetup::forceMinValue(int maxvalue)
{
  if (minvaluespinbox) {
    int step=  minvaluespinbox->singleStep();
    int value= minvaluespinbox->value();
    if (maxvalue < value + step)
      minvaluespinbox->setValue(value-step);
  }
}


void VcrossSetup::defineTextChoice(const vector<miutil::miString>& vchoice, int ndefault)
{
  if (textchoicebox) {
    textchoicebox->clear();
    vTextChoice= vchoice;
    int m= vTextChoice.size();
    for (int i=0; i<m; i++){
      textchoicebox->addItem(QString(vchoice[i].c_str()));
    }
    textchoicebox->setEnabled(true);
    if (ndefault>=0 && ndefault<m)
      textchoicebox->setCurrentIndex(ndefault);
    else
      textchoicebox->setCurrentIndex(0);
  }
}

void VcrossSetup::defineTextChoice2(const vector<miutil::miString>& vchoice, int ndefault)
{
  if (textchoicebox2) {
    textchoicebox2->clear();
    vTextChoice2= vchoice;
    int m= vTextChoice2.size();
    for (int i=0; i<m; i++)
      textchoicebox2->addItem(QString(vTextChoice2[i].c_str()));
    textchoicebox2->setEnabled(true);
    if (ndefault>=0 && ndefault<m)
      textchoicebox2->setCurrentIndex(ndefault);
    else
      textchoicebox2->setCurrentIndex(0);
  }
}


void VcrossSetup::setTextChoice(const miutil::miString& choice)
{
  if (textchoicebox) {
    miutil::miString t= choice.downcase();
    int m= vTextChoice.size();
    int i= 0;
    while (i<m && t!=vTextChoice[i].downcase()) i++;
    if (i==m) i=0;
    textchoicebox->setCurrentIndex(i);
  }
}


miutil::miString VcrossSetup::getTextChoice()
{
  if (textchoicebox) {
    int i= textchoicebox->currentIndex();
    if (i<0 || i>=int(vTextChoice.size())) i=0;
    return vTextChoice[i];
  } else
    return "";
}

void VcrossSetup::setTextChoice2(const miutil::miString& choice)
{
  if (textchoicebox2) {
    miutil::miString t= choice.downcase();
    int m= vTextChoice2.size();
    int i= 0;
    while (i<m && t!=vTextChoice2[i].downcase()) i++;
    if (i==m) i=0;
    textchoicebox2->setCurrentIndex(i);
  }
}


miutil::miString VcrossSetup::getTextChoice2()
{
  if (textchoicebox2) {
    int i= textchoicebox2->currentIndex();
    if (i<0 || i>=int(vTextChoice2.size())) i=0;
    return vTextChoice2[i];
  } else
    return "";
}

