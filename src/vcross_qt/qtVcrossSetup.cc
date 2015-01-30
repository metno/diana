/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "qtVcrossSetup.h"

#include "qtUtility.h"
#include "diLinetype.h"

#include <puTools/miStringFunctions.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#define MILOGGER_CATEGORY "diana.VcrossSetupUI"
#include <miLogger/miLogging.h>

// initialize static members
bool               VcrossSetupUI::initialized= false;
std::vector<Colour::ColourInfo> VcrossSetupUI::m_cInfo; // all defined colours
int                VcrossSetupUI::nr_colors=0;
QColor*            VcrossSetupUI::pixcolor=0;
int                VcrossSetupUI::nr_linewidths=0;
int                VcrossSetupUI::nr_linetypes=0;
std::vector<std::string>   VcrossSetupUI::linetypes;

VcrossSetupUI::VcrossSetupUI(QWidget* parent, const QString& name,
    QGridLayout * glayout, int row, int options)
  : QObject(parent)
{
  METLIBS_LOG_SCOPE();

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
    checkbox = new QCheckBox(name, parent);
    label= 0;
    glayout->addWidget(checkbox,row,ncol);
    connect( checkbox, SIGNAL( toggled(bool)), SLOT( setChecked( bool) ));
    ncol++;
  } else{
    checkbox= 0;
    label = new QLabel(name, parent);
    glayout->addWidget(label, row,ncol,Qt::AlignLeft);
    ncol++;
  }

  if (options & useColour) {
    colourbox = ComboBox(parent, pixcolor, nr_colors, true, 0 );
    glayout->addWidget(colourbox,row,ncol);
    ncol++;
  } else
    colourbox=0;

  if (options & useLineWidth) {
    linewidthbox =  LinewidthBox(parent, true);
    glayout->addWidget(linewidthbox,row,ncol);
    ncol++;
  } else
    linewidthbox=0;

  if (options & useLineType) {
    linetypebox = LinetypeBox(parent,true );
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


void VcrossSetupUI::setChecked(bool on)
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


bool VcrossSetupUI::isChecked()
{
  return (checkbox && checkbox->isChecked());
}

Colour::ColourInfo VcrossSetupUI::getColour()
{
  Colour::ColourInfo sColour;
  if (colourbox) {
    int index = colourbox->currentIndex();
    if (index >= 0 && index < m_cInfo.size())
      sColour = m_cInfo[index];
    else
      METLIBS_LOG_ERROR("invalid colour index=" << index);
  }
  return sColour;
}


void VcrossSetupUI::setColour(const std::string& colourString)
{
  int nr_colours = m_cInfo.size();
  for (int i = 0;i<nr_colours;i++){
    if (colourString==m_cInfo[i].name){
      if (colourbox && i<colourbox->count())
        colourbox->setCurrentIndex(i);
      break;
    }
  }
}


void VcrossSetupUI::setLinetype(const std::string& linetype)
{
  if (linetypebox) {
    int index= 0;
    while (index<nr_linetypes && linetypes[index]!=linetype) index++;
    if (index==nr_linetypes) index= 0;
    linetypebox->setCurrentIndex(index);
  }
}


std::string VcrossSetupUI::getLinetype()
{
  std::string sString;
  if (linetypebox) {
    int index = linetypebox->currentIndex();
    if (index >= 0 && index < linetypes.size())
      sString = linetypes[index];
    else
      METLIBS_LOG_ERROR("invalid linetype index=" << index);
  }
  return sString;
}


void VcrossSetupUI::setLinewidth(float linew)
{
  if (linewidthbox) {
    int index = int(linew - 1.0);
    if (index<0) index= 0;
    if (index>=nr_linewidths) index= nr_linewidths-1;
    linewidthbox->setCurrentIndex(index);
  }
}


float VcrossSetupUI::getLinewidth()
{
  float linew= 1.;
  if (linewidthbox)
    linew= linewidthbox->currentIndex() + 1.0;
  return linew;
}


void VcrossSetupUI::defineValue(int low, int high, int step, int value,
			      const std::string& prefix,
			      const std::string& suffix)
{
  if (valuespinbox) {
    valuespinbox->setMinimum(low);
    valuespinbox->setMaximum(high);
    valuespinbox->setSingleStep(step);
    if (not prefix.empty())
      valuespinbox->setPrefix(QString::fromStdString(prefix));
    else if (not suffix.empty())
      valuespinbox->setSuffix(QString::fromStdString(suffix));
    valuespinbox->setValue(value);
  }
}


void VcrossSetupUI::setValue(int value)
{
  if (valuespinbox)
    valuespinbox->setValue(value);
}


int VcrossSetupUI::getValue()
{
  if (valuespinbox)
    return valuespinbox->value();
  else
    return 0;
}


void VcrossSetupUI::defineMinValue(int low, int high, int step, int value,
			         const std::string& prefix,
			         const std::string& suffix)
{
  if (minvaluespinbox) {
    minvaluespinbox->setMinimum(low);
    minvaluespinbox->setMaximum(high);
    minvaluespinbox->setSingleStep(step);
    minvaluespinbox->setValue(value);
    if (not prefix.empty())
      minvaluespinbox->setPrefix(QString(prefix.c_str()));
    else if (not suffix.empty())
      minvaluespinbox->setSuffix(QString(suffix.c_str()));
    minvaluespinbox->setValue(value);
  }
}


void VcrossSetupUI::setMinValue(int value)
{
  if (minvaluespinbox)
    minvaluespinbox->setValue(value);
}


int VcrossSetupUI::getMinValue()
{
  if (not minvaluespinbox)
    return 0;

  const int mi = minvaluespinbox->value();
  if (not maxvaluespinbox or mi < maxvaluespinbox->value())
    return mi;
  else
    return minvaluespinbox->minimum();
}


void VcrossSetupUI::defineMaxValue(int low, int high, int step, int value,
			         const std::string& prefix,
			         const std::string& suffix)
{
  if (maxvaluespinbox) {
    maxvaluespinbox->setMinimum(low);
    maxvaluespinbox->setMaximum(high);
    maxvaluespinbox->setSingleStep(step);
    maxvaluespinbox->setValue(value);
    if (not prefix.empty())
      maxvaluespinbox->setPrefix(QString::fromStdString(prefix));
    else if (not suffix.empty())
      maxvaluespinbox->setSuffix(QString::fromStdString(suffix));
    maxvaluespinbox->setValue(value);
  }
}


void VcrossSetupUI::setMaxValue(int value)
{
  if (maxvaluespinbox)
    maxvaluespinbox->setValue(value);
}


int VcrossSetupUI::getMaxValue()
{
  if (not maxvaluespinbox)
    return 0;

  const int ma = maxvaluespinbox->value();
  if (not minvaluespinbox or ma > minvaluespinbox->value())
    return ma;
  else
    return maxvaluespinbox->maximum();
}


void VcrossSetupUI::defineTextChoice(const std::vector<std::string>& vchoice, int ndefault)
{
  if (textchoicebox) {
    textchoicebox->clear();
    vTextChoice= vchoice;
    int m= vTextChoice.size();
    for (int i=0; i<m; i++) {
      textchoicebox->addItem(QString::fromStdString(vchoice[i]));
    }
    textchoicebox->setEnabled(true);
    if (ndefault>=0 && ndefault<m)
      textchoicebox->setCurrentIndex(ndefault);
    else
      textchoicebox->setCurrentIndex(0);
  }
}

void VcrossSetupUI::defineTextChoice2(const std::vector<std::string>& vchoice, int ndefault)
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


void VcrossSetupUI::setTextChoice(const std::string& choice)
{
  if (textchoicebox) {
    const std::string t = miutil::to_lower(choice);
    const int m= vTextChoice.size();
    int i= 0;
    while (i<m && t!=miutil::to_lower(vTextChoice[i]))
      i++;
    if (i==m)
      i=0;
    textchoicebox->setCurrentIndex(i);
  }
}


std::string VcrossSetupUI::getTextChoice()
{
  if (textchoicebox && !vTextChoice.empty()) {
    int i = textchoicebox->currentIndex();
    if (i<0 || i>=int(vTextChoice.size()))
      i=0;
    return vTextChoice[i];
  } else
    return "";
}

void VcrossSetupUI::setTextChoice2(const std::string& choice)
{
  if (textchoicebox2) {
    const std::string t = miutil::to_lower(choice);
    int m = vTextChoice2.size();
    int i = 0;
    while (i<m && t!=miutil::to_lower(vTextChoice2[i]))
      i++;
    if (i==m)
      i=0;
    textchoicebox2->setCurrentIndex(i);
  }
}

std::string VcrossSetupUI::getTextChoice2()
{
  if (textchoicebox2 && !vTextChoice2.empty()) {
    int i= textchoicebox2->currentIndex();
    if (i<0 || i>=int(vTextChoice2.size()))
      i=0;
    return vTextChoice2[i];
  } else
    return "";
}
