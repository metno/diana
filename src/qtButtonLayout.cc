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
#include <qtButtonLayout.h>
#include <qtUtility.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <stdio.h>
#include <q3buttongroup.h>
#include <iostream>
#include <qtooltip.h>


ButtonLayout::ButtonLayout( QWidget* parent, 
			    vector<ObsDialogInfo::Button>& buttons,
			    int nr_col
			   )
  : QWidget(parent)
{

 buttonList  =  buttons;
 int nr_buttons = buttonList.size();
 
 currentButton = -1;

 extraPalette = QPalette( QColor( 100,150,150),  QColor( 100,150,150));

  
// number of lines of buttons
  int nr_lines = nr_buttons/nr_col;
  if( nr_buttons % nr_col )
    nr_lines++;  
 
  bgroup = new Q3ButtonGroup( this );
  b      = new ToggleButton*[nr_buttons];
  buttonOn.insert(buttonOn.end(),nr_buttons,false);
  buttonRightOn.insert(buttonRightOn.end(),nr_buttons,false);
  
  for( int i=0; i< nr_buttons; i++ ){
    b[i] = new ToggleButton( this, buttonList[i].name.c_str());
    connect( b[i], SIGNAL(rightButtonClicked(ToggleButton*)), 
	     SLOT(rightButtonClicked(ToggleButton* ))  );
    bgroup->insert( b[i] );
    if(!buttonList[i].tooltip.empty())
      QToolTip::add( b[i], buttonList[i].tooltip.c_str());
  }
    
  for( int i=0; i< nr_buttons; i++ ){
    b[i]->setToggleButton ( TRUE );
  }

  bgroup->hide();
  
  Q3GridLayout *layoutgrid = new Q3GridLayout(this, nr_lines, nr_col); 
  for( int k=0; k<nr_lines; k++){ 
    for( int i=0; i<nr_col;i++ ){
      int index = i + k*nr_col;
      if( index >= nr_buttons )
 	break;
      layoutgrid->addWidget(b[index], k, i );
    }
  }

  connect( bgroup, SIGNAL(clicked(int)), SLOT(groupClicked( int ))  );

}


bool ButtonLayout::isOn(int id){
  int nr_buttons = buttonList.size();
  if(id<nr_buttons)
    return b[id]->isOn();

  return false;
}


void ButtonLayout::setEnabled( bool enabled ){
/* Sets the buttons in this grid to enabled if the parameter enabled is true,
   else set the buttons in this grid to disabled if the parameter enabled is 
   false.
*/
#ifdef dButLay
    cerr<<"ButtonLayout::setEnabled called"<<endl;
#endif
    int nr_buttons = buttonList.size();

    if( enabled==true){
      for( int i=0; i<nr_buttons; i++){
	b[i]->setEnabled( true );
	if(buttonOn[i]){
	  b[i]->setOn( true);
	}
      }
    }

    if( enabled==false){
	for( int i=0; i<nr_buttons; i++){
	  b[i]->setOn( false );
	  b[i]->setEnabled( false );
	}
    }

}


void ButtonLayout::ALLClicked(){
// A call to this function pushes all the buttons in

  int nr_buttons = buttonList.size();
  for( int k=0; k< nr_buttons; k++ ){ 
    if(b[k]->isEnabled()){
      b[k]->setOn( TRUE );
      buttonOn[k] = true;
    }
  }
}


void ButtonLayout::NONEClicked(){
// A call to this function pushes all the buttons out 

  int nr_buttons = buttonList.size();
  for( int k=0; k< nr_buttons; k++ ){
    b[k]->setOn( FALSE );
    buttonOn[k]=false;
    currentButton = -1;
  }
}


void ButtonLayout::DEFAULTClicked(){
// A call to this function sets the buttons in their default position

  int nr_buttons = buttonList.size();
  for( int k=0; k < nr_buttons; k++ ){
    if( buttonList[k].Default && b[k]->isEnabled()){
      b[k]->setOn( TRUE );
      buttonOn[k]=true;
    }
    else{
      b[k]->setOn( FALSE );
      buttonOn[k]=false;
    } 
  } 
}


int ButtonLayout::setButtonOn( miString buttonName ){

  int n = buttonList.size();

  for( int j=0; j<n; j++){
    if(buttonName.downcase()==buttonList[j].name.downcase()){
      if(b[j]->isEnabled()){
	b[j]->setOn( true );
	buttonOn[j]=true;
      } else {
	buttonOn[j]=true;
      }
      return j;
    }
  }

  return -1;
}


void ButtonLayout::setButton( int tt ){

    b[tt]->setOn(true);
    buttonOn[tt]=true;
    groupClicked( tt );


}




void ButtonLayout::enableButtons(vector<bool> bArr){

  int nr_buttons = buttonList.size();

  if(bArr.size()!=nr_buttons) return;
  for( int i=0; i < nr_buttons; i++ )
    if( bArr[i] == true  ){
      b[i]->setEnabled(true);
      if(buttonOn[i]){
	b[i]->setOn(true);
      }else{
	b[i]->setOn(false);
      }
    }
}


vector<miString> ButtonLayout::getOKString(bool forLog) {

  vector<miString> str;
  int nr_buttons = buttonList.size();

  if(forLog){
   for( int k=0; k < nr_buttons; k++ )
     if( buttonOn[k] )
       str.push_back(buttonList[k].name);
  }else {
    for( int k=0; k < nr_buttons; k++ )
      if( b[k]->isOn()  )
	str.push_back(buttonList[k].name);
  }
  return str;
}


void ButtonLayout::setRightClicked(miString name,bool on  )
{
  //  cerr <<"setRightClicked:"<<name<<endl;

  int n = buttonList.size();

  if(name=="ALL_PARAMS"){
    
    for( int j=0; j<n; j++){
      buttonRightOn[j] = on;
      if(b[j]->isEnabled() && buttonOn[j]){
      }
    }
  } else {
    for( int j=0; j<n; j++){
      if(name==buttonList[j].name){
	buttonRightOn[j]=on;
	if(b[j]->isEnabled() && buttonOn[j]){
	}
	return;
      }
    }
  }
}


void ButtonLayout::rightButtonClicked(ToggleButton* butto  )
{
  //  cerr <<"rightButtonClicked"<<endl;

  int id = bgroup->id(butto);
  if(buttonList.size() > id){
    miString name = buttonList[id].name;
    if( !buttonOn[id] ) return;
    emit rightClickedOn(name);
  }

}

void ButtonLayout::groupClicked( int id )
// This function is called when a button is clicked
{

  int oldbi= currentButton;
  currentButton=id;
  
  if(b[currentButton]->isOn() ){
    buttonOn[currentButton]=true;
    emit inGroupClicked( currentButton );
  }
  else{
    buttonOn[currentButton]=false;
    emit outGroupClicked( currentButton );
  }
  
  return;

}


































