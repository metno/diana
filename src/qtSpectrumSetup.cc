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
#include <qapplication.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtUtility.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <diCommonTypes.h>
#include <diLinetype.h>
#include <diSpectrumManager.h>
#include <qtSpectrumSetup.h>

// initialize static members
bool               SpectrumSetup::initialized= false;
vector<Colour::ColourInfo> SpectrumSetup::m_cInfo; // all defined colours
int                SpectrumSetup::nr_colors=0;
QColor*            SpectrumSetup::pixcolor=0;
int                SpectrumSetup::nr_linewidths=0;
int                SpectrumSetup::nr_linetypes=0;
vector<miString>   SpectrumSetup::linetypes;


SpectrumSetup::SpectrumSetup( QWidget* parent,SpectrumManager *vm, miString text,
			Q3GridLayout * glayout, int row, int options, bool check)
  : QObject(parent),spectrumm(vm),name(text)
{
  //a Qobject with a checkbox,and up to three comboboxes
#ifdef DEBUGPRINT
  cout<<"SpectrumSetup::SpectrumSetup called"<<endl;
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

  //**************************************************************

  int ncol = 0;

  if (options & useOnOff) {
    checkbox = new QCheckBox(text.c_str(),parent);
    label= 0;
    glayout->addWidget(checkbox,row,ncol);
    connect( checkbox, SIGNAL( toggled(bool)), SLOT( setOn( bool) ));
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

  if (options & useText) {
    textbox = new QComboBox(parent );
    glayout->addWidget(textbox,row,ncol);
    ncol++;
  } else
    textbox=0;

  if (minvaluespinbox && maxvaluespinbox) {
    connect( minvaluespinbox, SIGNAL(valueChanged(int)),
	                      SLOT( forceMaxValue(int)));
    connect( maxvaluespinbox, SIGNAL(valueChanged(int)),
	                      SLOT( forceMinValue(int)));
  }

  //make sure everything is switched off to start
  //checkbox->setChecked(false);
//###########  if (check) setOn(false);
//###  if (check) setOn(true);

}



/******************************************/

void SpectrumSetup::setOn(bool on){
  if (checkbox)               checkbox->setChecked(on);
  if (colourbox)             colourbox->setEnabled(on);
  if (linewidthbox)       linewidthbox->setEnabled(on);
  if (linetypebox)         linetypebox->setEnabled(on);
  if (valuespinbox)       valuespinbox->setEnabled(on);
  if (minvaluespinbox) minvaluespinbox->setEnabled(on);
  if (maxvaluespinbox) maxvaluespinbox->setEnabled(on);
  if (textbox)                 textbox->setEnabled(on);
}

/******************************************/

bool SpectrumSetup::isOn(){
  if (checkbox && checkbox->isChecked())
    return true;
  else
    return false;
}

Colour::ColourInfo SpectrumSetup::getColour(){
  Colour::ColourInfo sColour;
  if (colourbox){
    int index = colourbox->currentItem();
    sColour = m_cInfo[index];
  }
  return sColour;
}


void SpectrumSetup::setColour(const miString& colourString){
  //cerr << "SpectrumSetup::setColour " << colourString << endl;
  int nr_colours = m_cInfo.size();
  for (int i = 0;i<nr_colours;i++){
    if (colourString==m_cInfo[i].name){
      if (colourbox && i<colourbox->count()) colourbox->setCurrentItem(i);
      break;
    }
  }
}


void SpectrumSetup::setLinetype(const miString& linetype){
  if (linetypebox) {
    int index= 0;
    while (index<nr_linetypes && linetypes[index]!=linetype) index++;
    if (index==nr_linetypes) index= 0;
    linetypebox->setCurrentItem(index);
  }
}


miString SpectrumSetup::getLinetype(){
  miString sString;
  if (linetypebox) {
    int index = linetypebox->currentItem();
    sString = linetypes[index];
  }
  return sString;
}


void SpectrumSetup::setLinewidth(float linew){
  if (linewidthbox) {
    int index = int(linew - 1.0);
    if (index<0) index= 0;
    if (index>=nr_linewidths) index= nr_linewidths-1;
    linewidthbox->setCurrentItem(index);
  }
}


float SpectrumSetup::getLinewidth(){
  float linew= 1.;
  if (linewidthbox)
    linew= linewidthbox->currentItem() + 1.0;
  return linew;
}


void SpectrumSetup::defineValue(int low, int high, int step, int value,
			        const miString& prefix,
			        const miString& suffix) {
  if (valuespinbox) {
    valuespinbox->setMinValue(low);
    valuespinbox->setMaxValue(high);
    valuespinbox->setLineStep(step);
    valuespinbox->setValue(value);
    if (prefix.exists())
      valuespinbox->setPrefix(QString(prefix.cStr()));
    else if (suffix.exists())
      valuespinbox->setSuffix(QString(suffix.cStr()));
    valuespinbox->setValue(value);
  }
}


void SpectrumSetup::setValue(int value){
  if (valuespinbox)
    valuespinbox->setValue(value);
}


int SpectrumSetup::getValue(){
  if (valuespinbox)
    return valuespinbox->value();
  else
    return 0;
}


void SpectrumSetup::defineMinValue(int low, int high, int step, int value,
			         const miString& prefix,
			         const miString& suffix) {
  if (minvaluespinbox) {
    minvaluespinbox->setMinValue(low);
    minvaluespinbox->setMaxValue(high);
    minvaluespinbox->setLineStep(step);
    minvaluespinbox->setValue(value);
    if (prefix.exists())
      minvaluespinbox->setPrefix(QString(prefix.cStr()));
    else if (suffix.exists())
      minvaluespinbox->setSuffix(QString(suffix.cStr()));
    minvaluespinbox->setValue(value);
  }
}


void SpectrumSetup::setMinValue(int value){
  if (minvaluespinbox)
    minvaluespinbox->setValue(value);
}


int SpectrumSetup::getMinValue(){
  if (minvaluespinbox)
    return minvaluespinbox->value();
  else
    return 0;
}


void SpectrumSetup::defineMaxValue(int low, int high, int step, int value,
			         const miString& prefix,
			         const miString& suffix) {
  if (maxvaluespinbox) {
    maxvaluespinbox->setMinValue(low);
    maxvaluespinbox->setMaxValue(high);
    maxvaluespinbox->setLineStep(step);
    maxvaluespinbox->setValue(value);
    if (prefix.exists())
      maxvaluespinbox->setPrefix(QString(prefix.cStr()));
    else if (suffix.exists())
      maxvaluespinbox->setSuffix(QString(suffix.cStr()));
    maxvaluespinbox->setValue(value);
  }
}


void SpectrumSetup::setMaxValue(int value){
  if (maxvaluespinbox)
    maxvaluespinbox->setValue(value);
}


int SpectrumSetup::getMaxValue(){
  if (maxvaluespinbox)
    return maxvaluespinbox->value();
  else
    return 0;
}


void SpectrumSetup::forceMaxValue(int minvalue){
  if (maxvaluespinbox) {
    int step=  maxvaluespinbox->singleStep();
    int value= maxvaluespinbox->value();
    if (minvalue > value - step)
      maxvaluespinbox->setValue(value+step);
  }
}


void SpectrumSetup::forceMinValue(int maxvalue){
  if (minvaluespinbox) {
    int step=  minvaluespinbox->singleStep();
    int value= minvaluespinbox->value();
    if (maxvalue < value + step)
      minvaluespinbox->setValue(value-step);
  }
}


void SpectrumSetup::defineText(const vector<miString>& texts,
			       int defaultItem){
  textbox->clear();
  int n= texts.size();
  if (n>0) {
    if (defaultItem>=0 && defaultItem<n)
      defaultTextItem= defaultItem;
    else
      defaultTextItem= 0;
    const char** cvstr= new const char*[n];
    for (int i=0; i<n; i++)
      cvstr[i]=  texts[i].cStr();
    // qt4 fix: insertStrList() -> insertStringList()
    // (uneffective, have to make QStringList and QString!)
    textbox->insertStringList( QStringList(QString(cvstr[0])), n );
    textbox->setCurrentItem(defaultTextItem);
    delete[] cvstr;
    textbox->setEnabled(true);
  } else {
    textbox->setEnabled(false);
  }
}


void SpectrumSetup::setText(const miString& text){
  if (textbox) {
    int n= textbox->count();
    QString qstr= QString(text.cStr());
    int i=0;
    while (i<n && qstr != textbox->text(i)) i++;
    if (i==n) i= defaultTextItem;
    textbox->setCurrentItem(i);
  }
}


void SpectrumSetup::setText(int index){
  if (textbox) {
    if (index<0 || index>=textbox->count())
      index= defaultTextItem;
    textbox->setCurrentItem(index);
  }
}


miString SpectrumSetup::getText(){
  miString str;
  if (textbox && textbox->count()>0)
    str= miString(textbox->currentText().latin1());
  return str;
}
