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
//#define dSatDlg

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtSatDialogAdvanced.h"
#include "qtToggleButton.h"
#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QSlider>
#include <QLabel>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qlcdnumber.h>
#include <QGridLayout>
#include <QFrame>
#include <QPixmap>
#include <QVBoxLayout>

#include <cstdio>
#include <sstream>

using namespace std;

#define HEIGHTLB 85

SatDialogAdvanced::SatDialogAdvanced( QWidget* parent,
    SatDialogInfo info)
: QWidget(parent){

  m_cut = info.cut;
  m_alphacut = info.alphacut;
  m_alpha = info.alpha;
  m_cutscale=m_cut.scale;
  m_alphacutscale=m_alphacut.scale;
  m_alphascale=m_alpha.scale;

  // Cut
  cutCheckBox = new QCheckBox(tr("Use stretch from first picture"),this);

  cut = new ToggleButton(this, tr("Cut"));

  cutlcd = LCDNumber( 4, this);

  scut  = Slider( m_cut.minValue, m_cut.maxValue, 1, m_cut.value,
      Qt::Horizontal, this);
  connect( cutCheckBox, SIGNAL( toggled( bool )), SLOT( cutCheckBoxSlot( bool )));
  connect( cutCheckBox, SIGNAL( toggled( bool )), SIGNAL( SatChanged()));

  connect( scut, SIGNAL( valueChanged( int )),
      SLOT( cutDisplay( int )));
  connect(scut, SIGNAL(valueChanged(int)),SIGNAL(SatChanged()));

  connect( cut, SIGNAL( toggled(bool)), SLOT( greyCut( bool) ));
  connect( cut, SIGNAL( clicked()),SIGNAL(SatChanged()));

  // AlphaCut
  alphacut = new ToggleButton(this, tr("Alpha cut"));
  connect( alphacut, SIGNAL( toggled(bool)), SLOT( greyAlphaCut( bool) ));
  connect( alphacut, SIGNAL( clicked()),SIGNAL(SatChanged()));

  alphacutlcd = LCDNumber( 4, this);

  salphacut  = Slider( m_alphacut.minValue, m_alphacut.maxValue, 1,
      m_alphacut.value, Qt::Horizontal, this);

  connect( salphacut, SIGNAL( valueChanged( int )),
      SLOT( alphacutDisplay( int )));
  connect(salphacut, SIGNAL(valueChanged(int)),SIGNAL(SatChanged()));

  // Alpha
  alpha = new ToggleButton(this, tr("Alpha"));
  connect( alpha, SIGNAL( toggled(bool)), SLOT( greyAlpha( bool) ));
  connect( alpha, SIGNAL( clicked()),SIGNAL(SatChanged()));

  m_alphanr= 1.0;

  alphalcd = LCDNumber( 4, this);

  salpha  = Slider( m_alpha.minValue, m_alpha.maxValue, 1, m_alpha.value,
      Qt::Horizontal, this);

  connect( salpha, SIGNAL( valueChanged( int )),
      SLOT( alphaDisplay( int )));
  connect(salpha, SIGNAL(valueChanged(int)),SIGNAL(SatChanged()));


  legendButton = new ToggleButton(this, tr("Table"));
  connect( legendButton, SIGNAL(clicked()),SIGNAL(SatChanged()));

  colourcut = new ToggleButton(this, tr("Colour cut"));
  connect( colourcut, SIGNAL(clicked()),SIGNAL(SatChanged()));
  connect( colourcut, SIGNAL(toggled(bool)),SLOT(colourcutClicked(bool)));
  colourcut->setChecked(false);

  standard=NormalPushButton( tr("Standard"), this);
  connect( standard, SIGNAL( clicked()), SLOT( setStandard()));
  connect( standard, SIGNAL( clicked()), SIGNAL( SatChanged()));

  colourList = new QListWidget( this );
  colourList->setSelectionMode(QAbstractItemView::MultiSelection);
  connect(colourList, SIGNAL(itemClicked(QListWidgetItem *))
      ,SIGNAL(SatChanged()));
  connect(colourList, SIGNAL(itemSelectionChanged ()),SLOT(colourcutOn()));

  QGridLayout*sliderlayout = new QGridLayout( this );
  sliderlayout->addWidget( cutCheckBox,0,0,1,3 );
  sliderlayout->addWidget( cut,         1,  0 );
  sliderlayout->addWidget( scut,        1, 1 );
  sliderlayout->addWidget( cutlcd,      1, 2 );
  sliderlayout->addWidget( alphacut,    2, 0 );
  sliderlayout->addWidget( salphacut,   2, 1 );
  sliderlayout->addWidget( alphacutlcd, 2, 2 );
  sliderlayout->addWidget( alpha,       3, 0 );
  sliderlayout->addWidget( salpha,      3, 1 );
  sliderlayout->addWidget( alphalcd,    3, 2 );
  sliderlayout->addWidget( legendButton,4, 0 );
  sliderlayout->addWidget( colourcut,   4, 1 );
  sliderlayout->addWidget( standard,    4, 2 );
  sliderlayout->addWidget( colourList, 6,0,1,3 );

  setOff();
  greyOptions();


}

/*********************************************/
void SatDialogAdvanced::cutCheckBoxSlot( bool on )
{
  greyCut(!on);
  cut->setEnabled(!on);
}
/*********************************************/
void SatDialogAdvanced::greyCut( bool on ){
  if( on ){
    scut->setEnabled( true );
    cutlcd->setEnabled( true );
    cutlcd->display( m_cutnr );
    cutCheckBox->setChecked(false);
  }
  else{
    scut->setEnabled( false );
    cutlcd->setEnabled( false );
    cutlcd->display( "OFF" );
  }
}

/*********************************************/
void SatDialogAdvanced::greyAlphaCut( bool on ){
  if( on ){
    salphacut->setEnabled( true );
    alphacutlcd->setEnabled( true );
    alphacutlcd->display( m_alphacutnr );
  }
  else{
    salphacut->setEnabled( false );
    alphacutlcd->setEnabled( false );
    alphacutlcd->display( "OFF" );
  }
}

/*********************************************/
void SatDialogAdvanced::greyAlpha( bool on ){
  if( on ){
    salpha->setEnabled( true );
    alphalcd->setEnabled( true );
    alphalcd->display( m_alphanr );
  }
  else{
    salpha->setEnabled( false );
    alphalcd->setEnabled( false );
    alphalcd->display( "OFF" );
  }
}


/*********************************************/
void SatDialogAdvanced::cutDisplay( int number ){
  m_cutnr= ((double)number)*m_cutscale;
  cutlcd->display( m_cutnr );
}


/*********************************************/
void SatDialogAdvanced::alphacutDisplay( int number ){
  m_alphacutnr= ((double)number)*m_alphacutscale;
  alphacutlcd->display( m_alphacutnr );
}


/*********************************************/
void SatDialogAdvanced::alphaDisplay( int number ){
  m_alphanr= ((double)number)*m_alphascale;
  alphalcd->display( m_alphanr );
}

/*********************************************/
void SatDialogAdvanced::setStandard(){
  // cerr << "SatDialogAdvanced::setStandard" << endl;
  blockSignals(true);
  //set standard dialog options for palette or rgb files
  if (palette){
    cut->setChecked(false); greyCut( false );
    legendButton->setChecked(true);
    scut->setValue(  m_cut.minValue );
  }
  else{
    cutCheckBox->setChecked(false);
    cut->setChecked(true); greyCut(true );
    legendButton->setChecked(false);
    scut->setValue(  m_cut.value );
  }
  salphacut->setValue(  m_alphacut.value );
  alphacut->setChecked(false); greyAlphaCut( false );
  salpha->setValue(  m_alpha.value );
  alpha->setChecked(false); greyAlpha( false );

  colourcut->setChecked(false);
  colourList->clearSelection();
  blockSignals(false);
}


/*********************************************/
void SatDialogAdvanced::setOff(){
  //turn off all options
  blockSignals(true);
  palette = false;
  cutCheckBox->setChecked(false);
  scut->setValue(  m_cut.minValue );
  cut->setChecked(false); greyCut( false );
  salphacut->setValue(  m_alphacut.value );
  alphacut->setChecked(false); greyAlphaCut( false );
  salpha->setValue(  m_alpha.value );
  alpha->setChecked(false); greyAlpha( false );
  legendButton->setChecked(false);
  colourcut->setChecked(false);
  colourList->clear();
  blockSignals(false);
}
/*********************************************/
void SatDialogAdvanced::colourcutOn(){
  colourcut->setChecked(true);
}
/*********************************************/
void SatDialogAdvanced::colourcutClicked(bool on){

  if (on){
    if (!colourList->count())
      emit getSatColours();
  }
}
/*********************************************/
std::string SatDialogAdvanced::getOKString()
{
  std::string str;
  ostringstream ostr;

  if(!palette){
    if( cutCheckBox->isChecked() )
      ostr<<" cut=-0.5";
    else if( cut->isChecked() )
      ostr<<" cut="<<m_cutnr;
    else
      ostr<<" cut=-1";

    if( alphacut->isChecked() )
      ostr<<" alphacut="<<m_alphacutnr;
    else
      ostr<<" alphacut=0";
  }

  if( alpha->isChecked() )
    ostr<<" alpha="<<m_alphanr;
  else
    ostr<<" alpha=1";

  if (palette){
    if( legendButton->isChecked())
      ostr<<" Table=1";
    else
      ostr<<" Table=0";

    //colours to hide
    if(colourcut->isChecked()){
      ostr << " hide=";
      int n =colourList->count();
      for (int i=0;i<n;i++){
        if (colourList->item(i)!=0 &&
            colourList->item(i)->isSelected()){
          ostr << i;
          if ( !colourList->item(i)->text().isEmpty() ) {
            ostr <<":"<<colourList->item(i)->text().toStdString();
          }
          ostr <<",";
        }
      }
    }
  }


  str = ostr.str();

  return str;

}



/*********************************************/
void SatDialogAdvanced::setPictures(std::string str){
  picturestring=str;
}

/*********************************************/
void SatDialogAdvanced::greyOptions(){

  if (picturestring.empty()){
    cutCheckBox->setEnabled(false);
    cut->setEnabled(false);
    greyCut(false);
    alphacut->setEnabled(false);
    alpha->setEnabled(false);
    legendButton->setEnabled(false);
    //    colourcut->setEnabled(false);
    standard->setEnabled(false);
    colourList->clear();
  }
  else if (palette){
    cutCheckBox->setEnabled(false);
    cut->setEnabled(false);
    alphacut->setEnabled(false);
    alpha->setEnabled(true);
    legendButton->setEnabled(true);
    //    colourcut->setEnabled(true);
    standard->setEnabled(true);
  }
  else {
    cutCheckBox->setEnabled(true);
    cut->setEnabled(!cutCheckBox->isChecked());
    alphacut->setEnabled(true);
    alpha->setEnabled(true);
    legendButton->setEnabled(false);
    //    colourcut->setEnabled(false);
    standard->setEnabled(true);
  }
}

/*********************************************/

void SatDialogAdvanced::setColours(vector <Colour> &colours){

  colourList->clear();
  palette = colours.size();
  if (colours.size()){
    int nr_colours=colours.size();
    QPixmap* pmap = new QPixmap( 20, 20 );
    for(int i=0;i<nr_colours;i++){
      pmap->fill( QColor(colours[i].R(),colours[i].G(),colours[i].B()) );
      QIcon qicon( *pmap );
      QString qs;
      QListWidgetItem* item= new QListWidgetItem(qicon,qs);
      colourList->addItem(item);
    }
    delete pmap;
    pmap=0;

  } else {
    colourcut->setChecked(false);
  }

}

/*********************************************/
std::string SatDialogAdvanced::putOKString(std::string str)
{
  //  cerr << "SatDialogAdvanced::putOKString: " << str<<endl;
  setStandard();
  blockSignals(true);

  std::string external;
  vector<std::string> tokens= miutil::split_protected(str, '"','"');
  int n= tokens.size();

  std::string key, value;
  for (int i=0; i<n; i++){    // search through plotinfo
    vector<std::string> stokens= miutil::split(tokens[i], 0, "=");
    if ( stokens.size()==2) {
      key = miutil::to_lower(stokens[0]);
      value = stokens[1];
      if ( key=="cut" && ! palette){
        m_cutnr = atof(value.c_str());
        if (m_cutnr<0){
          if (m_cutnr==-0.5){
            cutCheckBox->setChecked(true);
            cutCheckBoxSlot(true);
          } else {
            cut->setChecked(false);
            greyCut(false);
          }
        }else{
          int cutvalue = int(m_cutnr/m_cutscale+m_cutscale/2);
          scut->setValue(  cutvalue);
          cut->setChecked(true);
          greyCut( true );
        }
      }
      else if ( (key=="alphacut" || key=="alfacut") && !palette){
        if (value!="0"){
          m_alphacutnr = atof(value.c_str());
          int m_alphacutvalue = int(m_alphacutnr/m_alphacutscale+m_alphacutscale/2);
          salphacut->setValue(  m_alphacutvalue );
          alphacut->setChecked(true); greyAlphaCut( true );
        }else{
          alphacut->setChecked(false); greyAlphaCut(false);
        }
      }
      else if ( key=="alpha" || key=="alfa"){
        if (value!="1"){
          m_alphanr = atof(value.c_str());
          int m_alphavalue = int(m_alphanr/m_alphascale+m_alphascale/2);
          salpha->setValue(  m_alphavalue );
          alpha->setChecked(true); greyAlpha( true );
        }else{
          alpha->setChecked(false); greyAlpha(false);
        }
      }else if ( key=="table" && palette){
        if (atoi(value.c_str())!=0) legendButton->setChecked(true);
        else legendButton->setChecked(false);
      }
      else if (key=="hide" && palette){
        colourcut->setChecked(true);
        //set selected colours
        vector <std::string> stokens=miutil::split(value, 0, ",");
        int m= stokens.size();
        for (int j=0; j<m; j++){
          vector <std::string> sstokens=miutil::split(stokens[j], 0, ":");
          int icol=miutil::to_int(sstokens[0]);
          if(icol < colourList->count()){
            colourList->item(icol)->setSelected(true);
          }
          if ( sstokens.size() == 2 ) {
            colourList->item(icol)->setText(sstokens[1].c_str());
          }
        }
      }else{
        //anythig unknown, add to external string
        external+=" " + tokens[i];
      }
    } else if (stokens.size() ==1 && miutil::to_lower(stokens[0])=="hide"){
      //colourList should be visible
      colourcut->setChecked(true);
    }else{
      //anythig unknown, add to external string
      external+=" " + tokens[i];
    }
  }
  blockSignals(false);
  return external;
}


void SatDialogAdvanced::closeEvent( QCloseEvent* e) {
  emit SatHide();
}


void SatDialogAdvanced::blockSignals(bool b){
  cutCheckBox->blockSignals(b);
  cut->blockSignals(b);
  alphacut->blockSignals(b);
  alpha->blockSignals(b);
  scut->blockSignals(b);
  salphacut->blockSignals(b);
  salpha->blockSignals(b);
  legendButton->blockSignals(b);
  colourcut->blockSignals(b);
  standard->blockSignals(b);
  colourList->blockSignals(b);
  if (!b) {
    //after signals have been turned off, make sure buttons and LCD displays
    // are correct
    cutCheckBox->setChecked(cutCheckBox->isChecked());
    cut->Toggled(cut->isChecked());
    alphacut->Toggled(alphacut->isChecked());
    alpha->Toggled(alpha->isChecked());
    legendButton->Toggled(legendButton->isChecked());
    colourcut->Toggled(colourcut->isChecked());
    cutDisplay(scut->value());
    alphacutDisplay(salphacut->value());
    alphaDisplay(salpha->value());
  }
}
