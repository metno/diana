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

#include "qtButtonLayout.h"
#include "qtUtility.h"

#include <puTools/miStringFunctions.h>

#include <QGridLayout>
#include <QButtonGroup>

#define MILOGGER_CATEGORY "diana.ButtonLayout"
#include <miLogger/miLogging.h>

using namespace std;

ButtonLayout::ButtonLayout(QWidget* parent, vector<ObsDialogInfo::Button>& buttons, int nr_col)
  : QWidget(parent)
{
  buttonList  =  buttons;
  int nr_buttons = buttonList.size();

  // number of lines of buttons
  int nr_lines = nr_buttons/nr_col;
  if( nr_buttons % nr_col )
    nr_lines++;

  bgroup = new QButtonGroup( this );
  b      = new ToggleButton*[nr_buttons];
  buttonOn.insert(buttonOn.end(),nr_buttons,false);
  buttonRightOn.insert(buttonRightOn.end(),nr_buttons,false);

  for( int i=0; i< nr_buttons; i++ ){
    b[i] = new ToggleButton(this, QString::fromStdString(buttonList[i].name));
    connect(b[i], SIGNAL(rightButtonClicked(ToggleButton*)), SLOT(rightButtonClicked(ToggleButton*)));
    bgroup->addButton( b[i] ,i);
    if(!buttonList[i].tooltip.empty())
      b[i]->setToolTip(buttonList[i].tooltip.c_str());
  }

  bgroup->setExclusive(false);
  for( int i=0; i< nr_buttons; i++ ){
    b[i]->setCheckable(true);
  }

  //  bgroup->hide();

  QGridLayout *layoutgrid = new QGridLayout(this);
  layoutgrid->setSpacing(1);
  for( int k=0; k<nr_lines; k++){
    for( int i=0; i<nr_col;i++ ){
      int index = i + k*nr_col;
      if( index >= nr_buttons )
        break;
      layoutgrid->addWidget(b[index], k, i );
    }
  }

  connect(bgroup, SIGNAL(buttonClicked(int)), SLOT(groupClicked(int)));
}


bool ButtonLayout::isChecked(int id){
  int nr_buttons = buttonList.size();
  if(id<nr_buttons)
    return b[id]->isChecked();

  return false;
}


void ButtonLayout::setEnabled( bool enabled ){
  /* Sets the buttons in this grid to enabled if the parameter enabled is true,
   else set the buttons in this grid to disabled if the parameter enabled is
   false.
   */
#ifdef dButLay
  METLIBS_LOG_DEBUG("ButtonLayout::setEnabled called");
#endif
  int nr_buttons = buttonList.size();

  if( enabled==true){
    for( int i=0; i<nr_buttons; i++){
      b[i]->setEnabled( true );
      if(buttonOn[i]){
        b[i]->setChecked( true);
      }
    }
  }

  if( enabled==false){
    for( int i=0; i<nr_buttons; i++){
      b[i]->setChecked( false );
      b[i]->setEnabled( false );
    }
  }

}


void ButtonLayout::ALLClicked(){
  // A call to this function pushes all the buttons in

  int nr_buttons = buttonList.size();
  for( int k=0; k< nr_buttons; k++ ){
    if(b[k]->isEnabled()){
      b[k]->setChecked(true);
      buttonOn[k] = true;
    }
  }
}


void ButtonLayout::NONEClicked(){
  // A call to this function pushes all the buttons out

  int nr_buttons = buttonList.size();
  for( int k=0; k< nr_buttons; k++ ){
    b[k]->setChecked(false);
    buttonOn[k]=false;
  }
}


void ButtonLayout::DEFAULTClicked(){
  // A call to this function sets the buttons in their default position

  int nr_buttons = buttonList.size();
  for( int k=0; k < nr_buttons; k++ ){
    if( buttonList[k].Default && b[k]->isEnabled()){
      b[k]->setChecked(true);
      buttonOn[k]=true;
    }
    else{
      b[k]->setChecked(false);
      buttonOn[k]=false;
    }
  }
}


int ButtonLayout::setButtonOn( std::string buttonName ){

  int n = buttonList.size();

  for( int j=0; j<n; j++){
    if(miutil::to_lower(buttonName)==miutil::to_lower(buttonList[j].name)){
      if(b[j]->isEnabled()){
        b[j]->setChecked( true );
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

  b[tt]->setChecked(true);
  buttonOn[tt]=true;
  groupClicked( tt );


}




void ButtonLayout::enableButtons(vector<bool> bArr){

  unsigned int nr_buttons = buttonList.size();

  if(bArr.size()!=nr_buttons) return;
  for( unsigned int i=0; i < nr_buttons; i++ )
    if( bArr[i] == true  ){
      b[i]->setEnabled(true);
      if(buttonOn[i]){
        b[i]->setChecked(true);
      }else{
        b[i]->setChecked(false);
      }
    }
}


vector<std::string> ButtonLayout::getOKString(bool forLog) {

  vector<std::string> str;
  int nr_buttons = buttonList.size();

  if(forLog){
    for( int k=0; k < nr_buttons; k++ )
      if( buttonOn[k] )
        str.push_back(buttonList[k].name);
  }else {
    for( int k=0; k < nr_buttons; k++ )
      if( b[k]->isChecked()  )
        str.push_back(buttonList[k].name);
  }
  return str;
}


void ButtonLayout::setRightClicked(std::string name,bool on  )
{
  //  METLIBS_LOG_DEBUG("setRightClicked:"<<name);

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
  //  METLIBS_LOG_DEBUG("rightButtonClicked");

  unsigned int id = bgroup->id(butto);
  if(buttonList.size() > id){
    std::string name = buttonList[id].name;
    if( !buttonOn[id] ) return;
    emit rightClickedOn(name);
  }

}

void ButtonLayout::groupClicked( int id )
// This function is called when a button is clicked
{
  if(b[id]->isChecked() ){
    buttonOn[id]=true;
    emit inGroupClicked( id );
  }
  else{
    buttonOn[id]=false;
    emit outGroupClicked( id );
  }
}
