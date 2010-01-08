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
#ifndef _buttonlayout_h
#define _buttonlayout_h

#include <qwidget.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include <qpalette.h>
#include <puTools/miString.h>

#include <diCommonTypes.h>
#include <qtToggleButton.h>
#include <vector>


using namespace std;

/**

  \brief Grid of buttons

*/
class ButtonLayout : public QWidget
{
  Q_OBJECT

public:

  ButtonLayout( QWidget* parent,
		vector<ObsDialogInfo::Button>& button,
		int nr_col=3            //number of columns
		);

  bool isChecked(int);
  void setEnabled( bool enabled );
  int setButtonOn(miutil::miString buttonName);
  void enableButtons(vector<bool>);

  vector<miutil::miString> getOKString(bool forLog=false);

 public slots:
 void setRightClicked(miutil::miString name,bool on);
  void ALLClicked();
  void NONEClicked();
  void DEFAULTClicked();

private slots:
 void groupClicked( int );
 void rightButtonClicked(ToggleButton* butto);

signals:
  void inGroupClicked( int );
  void outGroupClicked( int );
  void rightClickedOn( miutil::miString );
  void rightClickedOff( miutil::miString );

private:

  void setButton( int tt );

  ToggleButton** b;
  QButtonGroup* bgroup;

  vector<bool> buttonOn;
  vector<bool> buttonRightOn;
  vector<ObsDialogInfo::Button> buttonList;

};



#endif

