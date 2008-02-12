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
//#define dSatDlg 
#include <qslider.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qmessagebox.h> 
#include <qlcdnumber.h>
#include <qtSatDialogAdvanced.h>
#include <qtToggleButton.h>
#include <qtUtility.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3Frame>
#include <QPixmap>
#include <Q3VBoxLayout>

#include <miString.h>
#include <stdio.h>
#include <iostream>

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

  cut = new ToggleButton( this,tr("Cut").latin1() );      

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
  alphacut = new ToggleButton( this,tr("Alpha cut").latin1() ); 
  connect( alphacut, SIGNAL( toggled(bool)), SLOT( greyAlphaCut( bool) ));
  connect( alphacut, SIGNAL( clicked()),SIGNAL(SatChanged()));
  
  alphacutlcd = LCDNumber( 4, this);
  
  salphacut  = Slider( m_alphacut.minValue, m_alphacut.maxValue, 1,
		      m_alphacut.value, Qt::Horizontal, this);
  
  connect( salphacut, SIGNAL( valueChanged( int )), 
	   SLOT( alphacutDisplay( int )));
  connect(salphacut, SIGNAL(valueChanged(int)),SIGNAL(SatChanged()));

  // Alpha
  alpha = new ToggleButton( this,tr("Alpha").latin1() ); 
  connect( alpha, SIGNAL( toggled(bool)), SLOT( greyAlpha( bool) ));
  connect( alpha, SIGNAL( clicked()),SIGNAL(SatChanged()));
  
  m_alphanr= 1.0;
  
  alphalcd = LCDNumber( 4, this);
  
  salpha  = Slider( m_alpha.minValue, m_alpha.maxValue, 1, m_alpha.value, 
		   Qt::Horizontal, this);
  
  connect( salpha, SIGNAL( valueChanged( int )), 
	   SLOT( alphaDisplay( int )));
  connect(salpha, SIGNAL(valueChanged(int)),SIGNAL(SatChanged()));
  
  
  classified = new ToggleButton( this,tr("Table").latin1() ); 
  connect( classified, SIGNAL(clicked()),SIGNAL(SatChanged()));
  
  colourcut = new ToggleButton( this,tr("Colour cut").latin1() ); 
  connect( colourcut, SIGNAL(clicked()),SIGNAL(SatChanged()));
  connect( colourcut, SIGNAL(toggled(bool)),SLOT(colourcutClicked(bool)));
  colourcut->setOn(false);
  
  standard=NormalPushButton( tr("Standard").latin1(), this);
  connect( standard, SIGNAL( clicked()), SLOT( setStandard()));
  connect( standard, SIGNAL( clicked()), SIGNAL( SatChanged()));
  
  colourEdit = new Q3GroupBox(2,Qt::Vertical,this);
  colourstring= TitleLabel(tr("Hide colours"), colourEdit );
  colourstring->setAlignment(Qt::AlignCenter);
  colourList = new Q3ListBox( colourEdit);
  colourList->setMinimumHeight(HEIGHTLB);
  colourList->setSelectionMode(Q3ListBox::Multi);
  connect(colourList, SIGNAL(clicked(Q3ListBoxItem *))
   	  ,SIGNAL(SatChanged()));     
  connect(colourList, SIGNAL(selectionChanged ()),SLOT(colourcutOn()));     

  Q3GridLayout*sliderlayout = new Q3GridLayout( 3, 4);
  sliderlayout->addWidget( cut,   0,  0 );
  sliderlayout->addWidget( scut,  0, 1 );
  sliderlayout->addWidget( cutlcd,0, 2 );
  sliderlayout->addWidget( alphacut,   1, 0 );
  sliderlayout->addWidget( salphacut,  1, 1 );
  sliderlayout->addWidget( alphacutlcd,1, 2 );
  sliderlayout->addWidget( alpha,   2, 0 );
  sliderlayout->addWidget( salpha,  2, 1 );
  sliderlayout->addWidget( alphalcd,2, 2 );  
  sliderlayout->addWidget( classified, 3, 0 );
  sliderlayout->addWidget( colourcut, 3, 1 );
  sliderlayout->addWidget( standard, 3, 2 );



  // Create a horizontal frame line
  Q3Frame *line = new Q3Frame( this );
  line->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );

  vlayout = new Q3VBoxLayout( this, 5, 5 );
  vlayout->addWidget( line );
  vlayout->addWidget( cutCheckBox );
  vlayout->addLayout( sliderlayout );
  vlayout->addWidget(colourEdit);

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
    cut->setOn(false); greyCut( false );
    classified->setOn(true);
    scut->setValue(  m_cut.minValue ); 
  }
  else{
    cutCheckBox->setChecked(false);
    cut->setOn(true); greyCut(true );
    classified->setOn(false);
    scut->setValue(  m_cut.value ); 
  }
  salphacut->setValue(  m_alphacut.value ); 
  alphacut->setOn(false); greyAlphaCut( false );
  salpha->setValue(  m_alpha.value );
  alpha->setOn(false); greyAlpha( false );

  colourcut->setOn(false);
  colourList->clearSelection();
  //colourEdit->hide();
  blockSignals(false);
}


/*********************************************/
void SatDialogAdvanced::setOff(){
  //turn off all options 
  blockSignals(true);
  cutCheckBox->setChecked(false);
  scut->setValue(  m_cut.minValue ); 
  cut->setOn(false); greyCut( false );
  salphacut->setValue(  m_alphacut.value ); 
  alphacut->setOn(false); greyAlphaCut( false );
  salpha->setValue(  m_alpha.value );
  alpha->setOn(false); greyAlpha( false );
  classified->setOn(false);
  colourcut->setOn(false);
  colourList->clear();
  colourList->clearSelection();
  //colourEdit->hide();
  blockSignals(false);
}
/*********************************************/
void SatDialogAdvanced::colourcutOn(){
  colourcut->setOn(true);
}
/*********************************************/
void SatDialogAdvanced::colourcutClicked(bool on){

  if (on){
    if (!colourList->count())
      emit getSatColours();  
  }
}
/*********************************************/
miString SatDialogAdvanced::getOKString(){
    miString str;
    ostringstream ostr;

    if(!palette){
      if( cutCheckBox->isChecked() )
	ostr<<" cut=-0.5";
      else if( cut->isOn() )
	ostr<<" cut="<<m_cutnr;
      else
	ostr<<" cut=-1";
      
      if( alphacut->isOn() )
	ostr<<" alphacut="<<m_alphacutnr;
      else
	ostr<<" alphacut=0";
    }
    
    if( alpha->isOn() )
	ostr<<" alpha="<<m_alphanr;
    else
	ostr<<" alpha=1";

    if (palette){
      if( classified->isOn())
	ostr<<" Table=1";
      else 
	ostr<<" Table=0";

      //colours to hide
      if(colourcut->isOn()){
	ostr << " hide=";
	int n =colourList->count();
	for (int i=0;i<n;i++){
	  if (colourList->isSelected(i))
	    ostr << i <<",";
	}
      }
      
    }

    str = ostr.str();

    return str;

}



/*********************************************/
void SatDialogAdvanced::setPictures(miString str){
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
    classified->setEnabled(false);
    //    colourcut->setEnabled(false);
    standard->setEnabled(false);
    colourList->clear();
    colourList->clearSelection();
    //colourEdit->hide();
  }
  else if (palette){
    cutCheckBox->setEnabled(false);
    cut->setEnabled(false);
    alphacut->setEnabled(false);
    alpha->setEnabled(true);
    classified->setEnabled(true);
    //    colourcut->setEnabled(true);
    standard->setEnabled(true);
  }
  else {
    cutCheckBox->setEnabled(true);
    cut->setEnabled(!cutCheckBox->isChecked());
    alphacut->setEnabled(true);
    alpha->setEnabled(true);
    classified->setEnabled(false);
    //    colourcut->setEnabled(false);
    standard->setEnabled(true);
  }
}

/*********************************************/

void SatDialogAdvanced::setColours(vector <Colour> &colours){

  //HACK
  palette = colours.size();

  colourList->clear();
  //  colourList->clearSelection();
  colourList->setColumnMode(Q3ListBox::FitToWidth);
  int w = colourList->width();
  int itemwidth=int (w/8);

  if (colours.size()){
    int nr_colors=colours.size();
    QPixmap*  pmapColor= new QPixmap( itemwidth, 20 );
    QColor pixcolor;
    for(int i=0;i<nr_colors;i++){
      pixcolor = QColor(colours[i].R(),colours[i].G(),
			colours[i].B());
      pmapColor->fill( pixcolor );       
      colourList->insertItem(*pmapColor);
    }
    delete pmapColor;
  }
  else{
    colourcut->setOn(false);
    //colourEdit->hide();
  }
  //  if (!colourcut->isOn()) colourList->setEnabled(false);
}

/*********************************************/
miString SatDialogAdvanced::putOKString(miString str){
  //  cerr << "SatDialogAdvanced::putOKString: " << str<<endl;
  setStandard();
  blockSignals(true);
  
  miString external;
  vector<miString> tokens= str.split('"','"');
  int n= tokens.size();

  miString key, value;
  for (int i=0; i<n; i++){    // search through plotinfo
    vector<miString> stokens= tokens[i].split('=');
    if ( stokens.size()==2) {
      key = stokens[0].downcase();
      value = stokens[1];
      if ( key=="cut" && ! palette){
	m_cutnr = atof(value.c_str());
	if (m_cutnr<0){
	  if (m_cutnr==-0.5){
	    cutCheckBox->setChecked(true);
	    cutCheckBoxSlot(true);
	  } else {
	    cut->setOn(false); 
	    greyCut(false);
	  }
	}else{
	  int cutvalue = int(m_cutnr/m_cutscale+m_cutscale/2);
	  scut->setValue(  cutvalue);
	  cut->setOn(true); 
	  greyCut( true );
	} 
      }
      else if ( (key=="alphacut" || key=="alfacut") && !palette){
	if (value!="0"){
	  m_alphacutnr = atof(value.c_str());
	  int m_alphacutvalue = int(m_alphacutnr/m_alphacutscale+m_alphacutscale/2);
	  salphacut->setValue(  m_alphacutvalue ); 
	  alphacut->setOn(true); greyAlphaCut( true );
	}else{
	  alphacut->setOn(false); greyAlphaCut(false);
	}
      }
      else if ( key=="alpha" || key=="alfa"){
	if (value!="1"){
	  m_alphanr = atof(value.c_str());
	  int m_alphavalue = int(m_alphanr/m_alphascale+m_alphascale/2);
	  salpha->setValue(  m_alphavalue ); 
	  alpha->setOn(true); greyAlpha( true );
	}else{
	  alpha->setOn(false); greyAlpha(false);
	}
      }else if ( key=="table" && palette){
	if (atoi(value.c_str())!=0) classified->setOn(true);
	else classified->setOn(false);
      }
      else if (key=="hide" && palette){
	colourcut->setOn(true);
	//colourEdit->show();
	//set selected colours
	vector <miString> stokens=value.split(',');
	int m= stokens.size();
	for (int j=0; j<m; j++){
	  int ih = atoi(stokens[j].c_str());
	  colourList->setSelected(ih,true);
	}
      }else{
	//anythig unknown, add to external string
	external+=" " + tokens[i];
      }
    } else if (stokens.size() ==1 && stokens[0].downcase()=="hide"){
      //colourList should be visible
      //colourEdit->show();
      colourcut->setOn(true);
    }else{
      //anythig unknown, add to external string
      external+=" " + tokens[i];
    }
  }
  blockSignals(false);
  return external;
}


bool SatDialogAdvanced::close(bool alsoDelete){
  emit SatHide();
  return true;
} 


void SatDialogAdvanced::blockSignals(bool b){
  cutCheckBox->blockSignals(b);
  cut->blockSignals(b);
  alphacut->blockSignals(b);
  alpha->blockSignals(b);
  scut->blockSignals(b);
  salphacut->blockSignals(b);
  salpha->blockSignals(b);
  classified->blockSignals(b);
  colourcut->blockSignals(b);
  standard->blockSignals(b);
  colourList->blockSignals(b);
  if (!b) {
    //after signals have been turned off, make sure buttons and LCD displays
    // are correct
    cutCheckBox->setChecked(cutCheckBox->isChecked());
    cut->Toggled(cut->isOn());
    alphacut->Toggled(alphacut->isOn());
    alpha->Toggled(alpha->isOn());
    classified->Toggled(classified->isOn());
    colourcut->Toggled(colourcut->isOn());
    cutDisplay(scut->value());
    alphacutDisplay(salphacut->value());
    alphaDisplay(salpha->value());
  }
}
