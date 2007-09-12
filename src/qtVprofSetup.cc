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
#include <qlayout.h>
#include <qpixmap.h>
#include <qtUtility.h>
#include <diCommonTypes.h>
#include <diVprofManager.h>
#include <qtVprofSetup.h>

// initialize static members
bool               VprofSetup::initialized= false;
vector<Colour::ColourInfo> VprofSetup::m_cInfo; // all defined colours
int                VprofSetup::nr_colors=0;
QColor*            VprofSetup::pixcolor=0;
int                VprofSetup::nr_linewidths=0;
int                VprofSetup::nr_linetypes=0;
vector<miString>   VprofSetup::linetypes;


VprofSetup::VprofSetup( QWidget* parent,VprofManager *vm,miString text, 
			QGridLayout * glayout, int row, int ncombo, bool check)
  : QObject(parent),vprofm(vm),name(text)
{
  //a Qobject with a checkbox,and up to three comboboxes
#ifdef DEBUGPRINT 
  cout<<"VprofSetUp::VprofSetUp called"<<endl;
#endif

  //------------------------
  if (!initialized) {
  //------------------------

  //**************************************************************

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

  if (check){
    checkbox = new QCheckBox(text.c_str(),parent);
    glayout->addWidget(checkbox,row,ncol);
    connect( checkbox, SIGNAL( toggled(bool)), SLOT( checkClicked( bool) ));
    ncol++;
    label=0;
  } else{
    label = new QLabel(text.c_str(),parent);
    glayout->addWidget(label,row,ncol,AlignLeft);
    ncol++;
    checkbox=0;
  }


  if (ncombo>0){
    colourbox = ComboBox(parent, pixcolor, nr_colors, false, 0 );
    colourbox->setEnabled(true);
    glayout->addWidget(colourbox,row,ncol);
    ncol++;
  } else
    colourbox=0;

  //three comboboxes, side by side
  if (ncombo>1){
    thicknessbox = LinewidthBox( parent, true);
    glayout->addWidget(thicknessbox,row,ncol);
    ncol++;
  }else
    thicknessbox=0;
  
  if (ncombo>2){
    linetypebox = LinetypeBox( parent,true );
    glayout->addWidget(linetypebox,row,ncol);
    ncol++;
  }else
    linetypebox=0;

  //make sure everything is switched off to start
  if (check) checkClicked(true);
}



/******************************************/

void VprofSetup::checkClicked(bool on){
  if (checkbox){
    checkbox->setChecked(on);
/******************************************************
    if (on){
      if (colourbox ) colourbox->show();
      if (thicknessbox) thicknessbox->show();
      if (linetypebox) linetypebox->show();
      if (colourbox ) colourbox->show();
      if (thicknessbox) thicknessbox->show();
      if (linetypebox) linetypebox->show();
    } else{
      if (colourbox) colourbox->hide();
      if (thicknessbox) thicknessbox->hide();
      if (linetypebox) linetypebox->hide();
    }
******************************************************/
    if (colourbox)       colourbox->setEnabled(on);
    if (thicknessbox) thicknessbox->setEnabled(on);
    if (linetypebox)   linetypebox->setEnabled(on);
  }
}

/******************************************/

bool VprofSetup::isOn(){
  if (checkbox && checkbox->isChecked())
    return true;
  else
    return false;
}

Colour::ColourInfo VprofSetup::getColour(){
  Colour::ColourInfo sColour;
  if (colourbox){
    int index = colourbox->currentItem();
    sColour = m_cInfo[index];
  }
  return sColour;
}


void VprofSetup::setColour(const miString& colourString){
  //cerr << "VprofSetup::setColour " << colourString << endl;
  int nr_colours = m_cInfo.size();
  for (int i = 0;i<nr_colours;i++){
    if (colourString==m_cInfo[i].name){
      if (colourbox && i<colourbox->count()) colourbox->setCurrentItem(i);
      break;
    }
  }
}


void VprofSetup::setLinetype(const miString& linetype){
  if (linetypebox) {
    int index= 0;
    while (index<nr_linetypes && linetypes[index]!=linetype) index++;
    if (index==nr_linetypes) index= 0;
    linetypebox->setCurrentItem(index);
  }
}


miString VprofSetup::getLinetype(){
  miString sString;
  if (linetypebox) {
    int index = linetypebox->currentItem();
    sString = linetypes[index]; 
  }
  return sString;
}


void VprofSetup::setLinewidth(float linew){
  if (thicknessbox) {
    int index = int(linew - 1.0);
    if (index<0) index= 0;
    if (index>=nr_linewidths) index= nr_linewidths-1;
    thicknessbox->setCurrentItem(index);
  }
}


float VprofSetup::getLinewidth(){
  float linew= 1.;
  if (thicknessbox)
    linew= thicknessbox->currentItem() + 1.0;
  return linew;
}


