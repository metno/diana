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
#ifndef VCROSSSETUP_H
#define VCROSSSETUP_H

#include <miString.h>
#include <diCommonTypes.h>
#include <vector>
#include <qwidget.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <QLabel>

using namespace std;

class QFont;
class Q3GridLayout;
class QCheckBox;
class QLabel;
class QComboBox;
class QSpinBox;
class VcrossManager;

/**
   \brief Elements in the Vertical Crossection setup dialogue

*/
class VcrossSetup : public QObject
{
  Q_OBJECT

public:

  enum useOpts {
    useOnOff=      0x0001,
    useColour=     0x0002,
    useLineWidth=  0x0004,
    useLineType=   0x0008,
    useValue=      0x0010,
    useMinValue=   0x0020,
    useMaxValue=   0x0040,
    useTextChoice= 0x0080,
    useTextChoice2=0x0100
  };

  //the constructor
  VcrossSetup( QWidget* parent, VcrossManager* vm, miString text,
	       Q3GridLayout* glayout,int row, int options, bool);

  bool isOn();
  Colour::ColourInfo getColour();
  void setColour(const miString&);
  void setLinetype(const miString&);
  miString getLinetype();
  void setLinewidth(float linew);
  float getLinewidth();

  void defineValue(int low, int high, int step, int value,
  		   const miString& prefix, const miString& suffix);
  void setValue(int value);
  int getValue();
  void defineMinValue(int low, int high, int step, int value,
		      const miString& prefix, const miString& suffix);
  void setMinValue(int value);
  int  getMinValue();
  void defineMaxValue(int low, int high, int step, int value,
		      const miString& prefix, const miString& suffix);
  void setMaxValue(int value);
  int  getMaxValue();
  void  defineTextChoice(const vector<miString>& vchoice, int ndefault=0);
  void     setTextChoice(const miString& choice);
  miString getTextChoice();

  void  defineTextChoice2(const vector<miString>& vchoice, int ndefault=0);
  void     setTextChoice2(const miString& choice);
  miString getTextChoice2();

  miString name;

public slots:
  void setOn(bool on);

private slots:
  void forceMaxValue(int minvalue);
  void forceMinValue(int maxvalue);

private:
  VcrossManager * vcrossm;

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
  QLabel    * label;
  QComboBox * colourbox;
  QComboBox * linewidthbox;
  QComboBox * linetypebox;
  QSpinBox  * valuespinbox;
  QSpinBox  * minvaluespinbox;
  QSpinBox  * maxvaluespinbox;
  QComboBox * textchoicebox;
  QComboBox * textchoicebox2;

  vector<miString> vTextChoice;
  vector<miString> vTextChoice2;

};

#endif
