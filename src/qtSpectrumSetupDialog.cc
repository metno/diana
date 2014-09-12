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

#include "qtUtility.h"
#include "diSpectrumManager.h"
#include "vcross_qt/qtVcrossSetup.h"
#include "qtSpectrumSetupDialog.h"
#include "diSpectrumOptions.h"

#include <puTools/miStringFunctions.h>

#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>

#define MILOGGER_CATEGORY "diana.SpectrumSetupDialog"
#include <miLogger/miLogging.h>

SpectrumSetupDialog::SpectrumSetupDialog( QWidget* parent, SpectrumManager* vm )
  : QDialog(parent)
  , spectrumm(vm)
  , isInitialized(false)
{
  METLIBS_LOG_SCOPE();

  setWindowTitle( tr("Diana Wavespectrum - settings") );

  //******** create the various QT widgets to appear in dialog *****

  initOptions();

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



  METLIBS_LOG_DEBUG("SpectrumSetupDialog::SpectrumSetupDialog finished");

}


void SpectrumSetupDialog::initOptions()
{

  METLIBS_LOG_DEBUG("SpectrumSetupDialog::initOptions");


  //make a grid with 4 rows, columms for labels and
  // for the checkboxes/comboboxes/spinboxes
  int numrows= 14;
//glayout = new QGridLayout(numrows,4); // linewidth not used, yet...
  glayout = new QGridLayout();
  glayout->setMargin( 5 );
  glayout->setSpacing( 2 );

  int nrow=0;

  QLabel* label1= new QLabel(tr("On/off"), this);
  QLabel* label2= new QLabel(tr("Colour"), this);
  QLabel* label3= new QLabel(tr("Line thickness"), this);
//QLabel* label4= new QLabel(tr("Line type"),parent);
  glayout->addWidget(label1,nrow,0);
  glayout->addWidget(label2,nrow,1);
  glayout->addWidget(label3,nrow,2);
//glayout->addWidget(label4,nrow,3);
  nrow++;

  int opts = (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  mSetupTEXTPLOT = new VcrossSetupUI(this, tr("Text"), glayout, nrow++, opts);
  mSetupFIXEDTEXT = new VcrossSetupUI(this, tr("Fixed text"), glayout, nrow++, opts);
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth);
  mSetupFRAME = new VcrossSetupUI(this,tr("Frame"), glayout, nrow++, opts);
  mSetupSPECTRUMLINES = new VcrossSetupUI(this,tr("Spectrum lines"), glayout, nrow++, opts);
  opts= (VcrossSetupUI::useOnOff);
  mSetupSPECTRUMCOLOUR = new VcrossSetupUI(this, tr("Spectrum coloured"), glayout, nrow++, opts);
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth);
  mSetupENERGYLINE = new VcrossSetupUI(this, tr("Graph line"), glayout, nrow++, opts);
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour);
  mSetupENERGYCOLOUR = new VcrossSetupUI(this, tr("Graph coloured"),glayout, nrow++, opts);
  opts= (VcrossSetupUI::useOnOff | VcrossSetupUI::useColour |
	 VcrossSetupUI::useLineWidth);
  mSetupPLOTWIND = new VcrossSetupUI(this, tr("Wind"), glayout, nrow++, opts);
  mSetupPLOTPEAKDIREC = new VcrossSetupUI(this, tr("Max direction"), glayout, nrow++, opts);

  nrow++;
  opts= VcrossSetupUI::useTextChoice;
  mSetupFREQUENCYMAX = new VcrossSetupUI(this, tr("Max frequency"), glayout, nrow++, opts);
  std::vector<std::string> vfreq;
  vfreq.push_back(miutil::from_number(0.50));
  vfreq.push_back(miutil::from_number(0.45));
  vfreq.push_back(miutil::from_number(0.40));
  vfreq.push_back(miutil::from_number(0.35));
  vfreq.push_back(miutil::from_number(0.30));
  vfreq.push_back(miutil::from_number(0.25));
  vfreq.push_back(miutil::from_number(0.20));
  vfreq.push_back(miutil::from_number(0.15));
  vfreq.push_back(miutil::from_number(0.10));
  mSetupFREQUENCYMAX->defineTextChoice(vfreq,4);

  nrow++;
  opts= VcrossSetupUI::useColour;
  mSetupBACKCOLOUR = new VcrossSetupUI(this, tr("Background colour"), glayout, nrow++, opts);

  if (nrow!=numrows) {
    METLIBS_LOG_DEBUG("==================================================");
    METLIBS_LOG_DEBUG("===== SpectrumSetupDialog: glayout numrows= "<<numrows);
    METLIBS_LOG_DEBUG("=====                                 nrow= "<<nrow);
    METLIBS_LOG_DEBUG("==================================================");
  }
}


void SpectrumSetupDialog::standardClicked()
{
  METLIBS_LOG_SCOPE();
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
  METLIBS_LOG_SCOPE();

  mSetupTEXTPLOT->setChecked    (spopt->pText);
  mSetupTEXTPLOT->setColour(spopt->textColour);

  mSetupFIXEDTEXT->setChecked    (spopt->pFixedText);
  mSetupFIXEDTEXT->setColour(spopt->fixedTextColour);

  mSetupFRAME->setChecked       (spopt->pFrame);
  mSetupFRAME->setColour   (spopt->frameColour);
  mSetupFRAME->setLinewidth(spopt->frameLinewidth);

  mSetupSPECTRUMLINES->setChecked       (spopt->pSpectrumLines);
  mSetupSPECTRUMLINES->setColour   (spopt->spectrumLineColour);
  mSetupSPECTRUMLINES->setLinewidth(spopt->spectrumLinewidth);

  mSetupSPECTRUMCOLOUR->setChecked(spopt->pSpectrumColoured);

  mSetupENERGYLINE->setChecked       (spopt->pEnergyLine);
  mSetupENERGYLINE->setColour   (spopt->energyLineColour);
  mSetupENERGYLINE->setLinewidth(spopt->energyLinewidth);

  mSetupENERGYCOLOUR->setChecked       (spopt->pEnergyColoured);
  mSetupENERGYCOLOUR->setColour   (spopt->energyFillColour);

  mSetupPLOTWIND->setChecked       (spopt->pWind);
  mSetupPLOTWIND->setColour   (spopt->windColour);
  mSetupPLOTWIND->setLinewidth(spopt->windLinewidth);

  mSetupPLOTPEAKDIREC->setChecked       (spopt->pPeakDirection);
  mSetupPLOTPEAKDIREC->setColour   (spopt->peakDirectionColour);
  mSetupPLOTPEAKDIREC->setLinewidth(spopt->peakDirectionLinewidth);

  mSetupFREQUENCYMAX->setTextChoice(miutil::from_number(spopt->freqMax));

  mSetupBACKCOLOUR->setColour(spopt->backgroundColour);
}


void SpectrumSetupDialog::applySetup()
{

  METLIBS_LOG_DEBUG("SpectrumSetupDialog::applySetup()");

  SpectrumOptions * spopt= spectrumm->getOptions();

  spopt->pText=      mSetupTEXTPLOT->isChecked();
  spopt->textColour= mSetupTEXTPLOT->getColour().name;

  spopt->pFixedText=      mSetupFIXEDTEXT->isChecked();
  spopt->fixedTextColour= mSetupFIXEDTEXT->getColour().name;

  spopt->pFrame=         mSetupFRAME->isChecked();
  spopt->frameColour=    mSetupFRAME->getColour().name;
  spopt->frameLinewidth= mSetupFRAME->getLinewidth();

  spopt->pSpectrumLines=     mSetupSPECTRUMLINES->isChecked();
  spopt->spectrumLineColour= mSetupSPECTRUMLINES->getColour().name;
  spopt->spectrumLinewidth=  mSetupSPECTRUMLINES->getLinewidth();

  spopt->pSpectrumColoured= mSetupSPECTRUMCOLOUR->isChecked();

  spopt->pEnergyLine=      mSetupENERGYLINE->isChecked();
  spopt->energyLineColour= mSetupENERGYLINE->getColour().name;
  spopt->energyLinewidth=  mSetupENERGYLINE->getLinewidth();

  spopt->pEnergyColoured=  mSetupENERGYCOLOUR->isChecked();
  spopt->energyFillColour= mSetupENERGYCOLOUR->getColour().name;

  spopt->pWind=         mSetupPLOTWIND->isChecked();
  spopt->windColour=    mSetupPLOTWIND->getColour().name;
  spopt->windLinewidth= mSetupPLOTWIND->getLinewidth();

  spopt->pPeakDirection=         mSetupPLOTPEAKDIREC->isChecked();
  spopt->peakDirectionColour=    mSetupPLOTPEAKDIREC->getColour().name;
  spopt->peakDirectionLinewidth= mSetupPLOTPEAKDIREC->getLinewidth();

  std::string str= mSetupFREQUENCYMAX->getTextChoice();
  spopt->freqMax= atof(str.c_str());

  spopt->backgroundColour= mSetupBACKCOLOUR->getColour().name;
}


void SpectrumSetupDialog::helpClicked()
{
  Q_EMIT showsource("ug_spectrum.html");
}


void SpectrumSetupDialog::applyClicked()
{
  METLIBS_LOG_SCOPE();
  applySetup();
  Q_EMIT SetupApply();
}


void SpectrumSetupDialog::applyhideClicked()
{
  METLIBS_LOG_SCOPE();
  applySetup();
  Q_EMIT SetupHide();
  Q_EMIT SetupApply();
}


void SpectrumSetupDialog::closeEvent(QCloseEvent* e)
{
  Q_EMIT SetupHide();
}
