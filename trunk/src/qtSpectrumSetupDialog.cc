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

#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#include <qtUtility.h>
#include <diSpectrumManager.h>
#include <qtVcrossSetup.h>
#include <qtSpectrumSetupDialog.h>
#include <diSpectrumOptions.h>


SpectrumSetupDialog::SpectrumSetupDialog( QWidget* parent, SpectrumManager* vm )
  : QDialog(parent), spectrumm(vm)
{
#ifdef DEBUGPRINT
  cout<<"SpectrumSetupDialog::SpectrumSetupDialog called"<<endl;
#endif

  //caption to appear on top of dialog
  setWindowTitle( tr("Diana Wavespectrum - settings") );

  // text constants
 TEXTPLOT         =  tr("Text").toStdString();
 FIXEDTEXT        =  tr("Fixed text").toStdString();
 FRAME            =  tr("Frame").toStdString();
 SPECTRUMLINES    =  tr("Spectrum lines").toStdString();
 SPECTRUMCOLOUR   =  tr("Spectrum coloured").toStdString();
 ENERGYLINE       =  tr("Graph line").toStdString();
 ENERGYCOLOUR     =  tr("Graph coloured").toStdString();
 PLOTWIND         =  tr("Wind").toStdString();
 PLOTPEAKDIREC    =  tr("Max direction").toStdString();
 FREQUENCYMAX     =  tr("Max frequency").toStdString();
 BACKCOLOUR       =  tr("Background colour").toStdString();

  //******** create the various QT widgets to appear in dialog *****

  spSetups.clear();
  initOptions( this );


  //******** standard buttons **************************************

  // push button to show help
      QPushButton * setuphelp = NormalPushButton( tr("Help"), this );
  connect(  setuphelp, SIGNAL(clicked()), SLOT( helpClicked()));

  // push button to set to default
  QPushButton * standard = NormalPushButton( tr("Default"), this );
  connect(  standard, SIGNAL(clicked()), SLOT( standardClicked()));

  // push button to hide dialog
  QPushButton * setuphide = NormalPushButton( tr("Hide"), this );
  connect( setuphide, SIGNAL(clicked()), SIGNAL(SetupHide()));

  // push button to apply the selected setup and then hide dialog
  QPushButton * setupapplyhide = NormalPushButton( tr("Apply+Hide"), this );
  connect( setupapplyhide, SIGNAL(clicked()), SLOT(applyhideClicked()));

  // push button to apply the selected setup
  QPushButton * setupapply = NormalPushButton( tr("Apply"), this );
  connect(setupapply, SIGNAL(clicked()), SLOT( applyClicked()) );

  // *********** place all the widgets in layouts ****************

  //place buttons "oppdater", "hjelp" etc. in horizontal layout
  QHBoxLayout* hlayout1 = new QHBoxLayout();
  hlayout1->addWidget( setuphelp );
  hlayout1->addWidget( standard );

  //place buttons "utfør", "help" etc. in horizontal layout
  QHBoxLayout* hlayout2 = new QHBoxLayout();
  hlayout2->addWidget( setuphide );
  hlayout2->addWidget( setupapplyhide );
  hlayout2->addWidget( setupapply );

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this);
  vlayout->addLayout( glayout );
  vlayout->addLayout( hlayout1 );
  vlayout->addLayout( hlayout2 );

  isInitialized=false;

#ifdef DEBUGPRINT
  cout<<"SpectrumSetupDialog::SpectrumSetupDialog finished"<<endl;
#endif
}


void SpectrumSetupDialog::initOptions(QWidget* parent)
{
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::initOptions" << endl;
#endif

  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes/spinboxes
  int numrows= 14;
//glayout = new QGridLayout(numrows,4); // linewidth not used, yet...
  glayout = new QGridLayout();
  glayout->setMargin( 5 );
  glayout->setSpacing( 2 );

  int nrow=0;

  QLabel* label1= new QLabel(tr("On/off"),parent);
  QLabel* label2= new QLabel(tr("Colour"),parent);
  QLabel* label3= new QLabel(tr("Line thickness"),parent);
//QLabel* label4= new QLabel(tr("Line type"),parent);
  glayout->addWidget(label1,nrow,0);
  glayout->addWidget(label2,nrow,1);
  glayout->addWidget(label3,nrow,2);
//glayout->addWidget(label4,nrow,3);
  nrow++;

  int n,opts;

  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour);
  spSetups.push_back(new VcrossSetup(parent,TEXTPLOT,glayout,nrow++,opts));
  spSetups.push_back(new VcrossSetup(parent,FIXEDTEXT,glayout,nrow++,opts));
  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour |
	 VcrossSetup::useLineWidth);
  spSetups.push_back(new VcrossSetup(parent,FRAME,glayout,nrow++,opts));
  spSetups.push_back(new VcrossSetup(parent,SPECTRUMLINES,glayout,nrow++,opts));
  opts= (VcrossSetup::useOnOff);
  spSetups.push_back(new VcrossSetup(parent,SPECTRUMCOLOUR,glayout,nrow++,opts));
  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour |
	 VcrossSetup::useLineWidth);
  spSetups.push_back(new VcrossSetup(parent,ENERGYLINE,glayout,nrow++,opts));
  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour);
  spSetups.push_back(new VcrossSetup(parent,ENERGYCOLOUR,glayout,nrow++,opts));
  opts= (VcrossSetup::useOnOff | VcrossSetup::useColour |
	 VcrossSetup::useLineWidth);
  spSetups.push_back(new VcrossSetup(parent,PLOTWIND,glayout,nrow++,opts));
  spSetups.push_back(new VcrossSetup(parent,PLOTPEAKDIREC,glayout,nrow++,opts));

  nrow++;
  opts= VcrossSetup::useTextChoice;
  spSetups.push_back(new VcrossSetup(parent,FREQUENCYMAX,glayout,nrow++,opts));
  vector<miutil::miString> vfreq;
  vfreq.push_back(miutil::miString(0.50));
  vfreq.push_back(miutil::miString(0.45));
  vfreq.push_back(miutil::miString(0.40));
  vfreq.push_back(miutil::miString(0.35));
  vfreq.push_back(miutil::miString(0.30));
  vfreq.push_back(miutil::miString(0.25));
  vfreq.push_back(miutil::miString(0.20));
  vfreq.push_back(miutil::miString(0.15));
  vfreq.push_back(miutil::miString(0.10));
  n= spSetups.size()-1;
  spSetups[n]->defineTextChoice(vfreq,4);

  nrow++;
  opts= VcrossSetup::useColour;
  spSetups.push_back(new VcrossSetup(parent,BACKCOLOUR,glayout,nrow++,opts));

  if (nrow!=numrows) {
    cerr<<"=================================================="<<endl;
    cerr<<"===== SpectrumSetupDialog: glayout numrows= "<<numrows<<endl;
    cerr<<"=====                                 nrow= "<<nrow<<endl;
    cerr<<"=================================================="<<endl;
  }
}


void SpectrumSetupDialog::standardClicked()
{
  //this slot is called when standard button pressed
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::standardClicked()" << endl;
#endif
  SpectrumOptions * spopt= new SpectrumOptions; // diana defaults
  setup(spopt);
  delete spopt;
  //emit SetupApply();
}


void SpectrumSetupDialog::start()
{
  if (!isInitialized){
    // pointer to logged options (the first time)
    SpectrumOptions * spopt= spectrumm->getOptions();
    setup(spopt);
    isInitialized=true;
  }
}


void SpectrumSetupDialog::setup(SpectrumOptions *spopt)
{
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::setup()" << endl;
#endif

  int n= spSetups.size();

  for (int i=0; i<n; i++) {

    if (spSetups[i]->name== TEXTPLOT) {
      spSetups[i]->setChecked    (spopt->pText);
      spSetups[i]->setColour(spopt->textColour);

    } else if (spSetups[i]->name== FIXEDTEXT) {
      spSetups[i]->setChecked    (spopt->pFixedText);
      spSetups[i]->setColour(spopt->fixedTextColour);

    } else if (spSetups[i]->name== FRAME) {
      spSetups[i]->setChecked       (spopt->pFrame);
      spSetups[i]->setColour   (spopt->frameColour);
      spSetups[i]->setLinewidth(spopt->frameLinewidth);

    } else if (spSetups[i]->name== SPECTRUMLINES) {
      spSetups[i]->setChecked       (spopt->pSpectrumLines);
      spSetups[i]->setColour   (spopt->spectrumLineColour);
      spSetups[i]->setLinewidth(spopt->spectrumLinewidth);

    } else if (spSetups[i]->name== SPECTRUMCOLOUR) {
      spSetups[i]->setChecked(spopt->pSpectrumColoured);

    } else if (spSetups[i]->name== ENERGYLINE) {
      spSetups[i]->setChecked       (spopt->pEnergyLine);
      spSetups[i]->setColour   (spopt->energyLineColour);
      spSetups[i]->setLinewidth(spopt->energyLinewidth);

    } else if (spSetups[i]->name== ENERGYCOLOUR) {
      spSetups[i]->setChecked       (spopt->pEnergyColoured);
      spSetups[i]->setColour   (spopt->energyFillColour);

    } else if (spSetups[i]->name== PLOTWIND) {
      spSetups[i]->setChecked       (spopt->pWind);
      spSetups[i]->setColour   (spopt->windColour);
      spSetups[i]->setLinewidth(spopt->windLinewidth);

    } else if (spSetups[i]->name== PLOTPEAKDIREC) {
      spSetups[i]->setChecked       (spopt->pPeakDirection);
      spSetups[i]->setColour   (spopt->peakDirectionColour);
      spSetups[i]->setLinewidth(spopt->peakDirectionLinewidth);

    } else if (spSetups[i]->name== FREQUENCYMAX) {
      spSetups[i]->setTextChoice(miutil::miString(spopt->freqMax));

    } else if (spSetups[i]->name== BACKCOLOUR) {
      spSetups[i]->setColour(spopt->backgroundColour);

    } else {
      cerr<<"SpectrumSetupDialog::setup ERROR : "
	  <<spSetups[i]->name<<endl;
    }

  }
}


void SpectrumSetupDialog::applySetup()
{
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::applySetup()" << endl;
#endif
  SpectrumOptions * spopt= spectrumm->getOptions();

  int n= spSetups.size();

  for (int i=0; i<n; i++) {

    if (spSetups[i]->name== TEXTPLOT) {
      spopt->pText=      spSetups[i]->isChecked();
      spopt->textColour= spSetups[i]->getColour().name;

    } else if (spSetups[i]->name== FIXEDTEXT) {
      spopt->pFixedText=      spSetups[i]->isChecked();
      spopt->fixedTextColour= spSetups[i]->getColour().name;

    } else if (spSetups[i]->name== FRAME) {
      spopt->pFrame=         spSetups[i]->isChecked();
      spopt->frameColour=    spSetups[i]->getColour().name;
      spopt->frameLinewidth= spSetups[i]->getLinewidth();

    } else if (spSetups[i]->name== SPECTRUMLINES) {
      spopt->pSpectrumLines=     spSetups[i]->isChecked();
      spopt->spectrumLineColour= spSetups[i]->getColour().name;
      spopt->spectrumLinewidth=  spSetups[i]->getLinewidth();

    } else if (spSetups[i]->name== SPECTRUMCOLOUR) {
      spopt->pSpectrumColoured= spSetups[i]->isChecked();

    } else if (spSetups[i]->name== ENERGYLINE) {
      spopt->pEnergyLine=      spSetups[i]->isChecked();
      spopt->energyLineColour= spSetups[i]->getColour().name;
      spopt->energyLinewidth=  spSetups[i]->getLinewidth();

    } else if (spSetups[i]->name== ENERGYCOLOUR) {
      spopt->pEnergyColoured=  spSetups[i]->isChecked();
      spopt->energyFillColour= spSetups[i]->getColour().name;

    } else if (spSetups[i]->name== PLOTWIND) {
      spopt->pWind=         spSetups[i]->isChecked();
      spopt->windColour=    spSetups[i]->getColour().name;
      spopt->windLinewidth= spSetups[i]->getLinewidth();

    } else if (spSetups[i]->name== PLOTPEAKDIREC) {
      spopt->pPeakDirection=         spSetups[i]->isChecked();
      spopt->peakDirectionColour=    spSetups[i]->getColour().name;
      spopt->peakDirectionLinewidth= spSetups[i]->getLinewidth();

    } else if (spSetups[i]->name== FREQUENCYMAX) {
      miutil::miString str= spSetups[i]->getTextChoice();
      spopt->freqMax= atof(str.cStr());

    } else if (spSetups[i]->name== BACKCOLOUR) {
      spopt->backgroundColour= spSetups[i]->getColour().name;

    } else {
      cerr<<"SpectrumSetupDialog::applySetup ERROR : "
	  <<spSetups[i]->name<<endl;
    }

  }
}


void SpectrumSetupDialog::helpClicked()
{
  //this slot is called when help button pressed
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::helpClicked()" << endl;
#endif
  emit showsource("ug_spectrum.html");
}


void SpectrumSetupDialog::applyClicked()
{
  //this slot is called when applyhide button pressed
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::applyClicked()" << endl;
#endif
  applySetup();
  emit SetupApply();
}


void SpectrumSetupDialog::applyhideClicked()
{
  //this slot is called when applyhide button pressed
#ifdef DEBUGPRINT
  cerr <<"SpectrumSetupDialog::applyhideClicked()" << endl;
#endif
  applySetup();
  emit SetupHide();
  emit SetupApply();
}


void SpectrumSetupDialog::closeEvent( QCloseEvent* e)
{
  emit SetupHide();
}
