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
#ifndef VPROFSETUP_H
#define VPROFSETUP_H

#include <miString.h>
#include <diCommonTypes.h>
#include <vector>
#include <qwidget.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <QLabel>

using namespace std;

class QFont;
class QGridLayout;
class QCheckBox;
class QLabel;
class QComboBox;
class VprofManager;

/**
   \brief Elements in the Vertical Profile setup dialogue

*/
class VprofSetup: public QObject
{
  Q_OBJECT

public:

  //the constructor
  VprofSetup( QWidget* parent, VprofManager* vm, miString, QGridLayout *,
	      int,int, bool);

  bool isOn();
  Colour::ColourInfo getColour();
  void setColour(const miString&);
  void setLinetype(const miString&);
  miString getLinetype();
  void setLinewidth(float linew);
  float getLinewidth();
  miString name;

public slots:
  void checkClicked(bool);

private:
  VprofManager * vprofm;

  static bool initialized;
  //init QT stuff
  static vector<Colour::ColourInfo> m_cInfo; // all defined colours
  //pixmaps for combo boxes
  static int        nr_colors;
  static QColor* pixcolor;
  static int        nr_linewidths;
  static int        nr_linetypes;
  static vector<miString> linetypes;

  QCheckBox * checkbox;
  QLabel * label;
  //three comboboxes, side by side
  QComboBox * colourbox;
  QComboBox * thicknessbox;
  QComboBox * linetypebox;

};

#endif
