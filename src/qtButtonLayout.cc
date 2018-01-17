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

#include "diana_config.h"

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
    b[i]->setEnabled(true);
  }

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

  connect(bgroup, SIGNAL(buttonClicked(int)), SIGNAL(buttonClicked(int)));
}

void ButtonLayout::setEnabled(bool enabled)
{

  int nr_buttons = buttonList.size();

  for (int i = 0; i < nr_buttons; i++)
    b[i]->setEnabled(enabled);
}

void ButtonLayout::ALLClicked()
{
  int nr_buttons = buttonList.size();
  for( int k=0; k< nr_buttons; k++ ){
      b[k]->setChecked(true);
  }
}

void ButtonLayout::NONEClicked(){
  // A call to this function pushes all the buttons out

  int nr_buttons = buttonList.size();
  for( int k=0; k< nr_buttons; k++ ){
    b[k]->setChecked(false);
  }
}

bool ButtonLayout::noneChecked()
{
  const size_t nr_buttons = buttonList.size();
  for (size_t k = 0; k < nr_buttons; k++)
    if (b[k]->isChecked())
      return false;
  return true;
}

int ButtonLayout::setButtonOn( std::string buttonName ){

  int n = buttonList.size();

  for( int j=0; j<n; j++){
    if(miutil::to_lower(buttonName)==miutil::to_lower(buttonList[j].name)){
        b[j]->setChecked( true );
      return j;
    }
  }

  return -1;
}

std::vector<std::string> ButtonLayout::getOKString(bool forLog)
{
  std::vector<std::string> str;
  const size_t nr_buttons = buttonList.size();
  for (size_t k=0; k < nr_buttons; k++)
    if (b[k]->isChecked())
      str.push_back(buttonList[k].name);
  return str;
}

void ButtonLayout::rightButtonClicked(ToggleButton* butto  )
{

  unsigned int id = bgroup->id(butto);
  if(buttonList.size() > id){
    std::string name = buttonList[id].name;
    emit rightClickedOn(name);
  }

}

