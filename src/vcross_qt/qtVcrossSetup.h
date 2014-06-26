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
#ifndef VCROSSSETUP_H
#define VCROSSSETUP_H

#include "diColour.h"

#include <QObject>

#include <vector>
#include <string>

class VcrossManager;

class QCheckBox;
class QColor;
class QComboBox;
class QFont;
class QGridLayout;
class QLabel;
class QSpinBox;

/**
   \brief Elements in the Vertical Crossection setup dialogue
*/
class VcrossSetupUI : public QObject
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

  VcrossSetupUI(QWidget* parent, const QString& name,
      QGridLayout* glayout, int row, int options);

  bool isChecked();
  Colour::ColourInfo getColour();
  void setColour(const std::string&);
  void setLinetype(const std::string&);
  std::string getLinetype();
  void setLinewidth(float linew);
  float getLinewidth();

  void defineValue(int low, int high, int step, int value,
  		   const std::string& prefix, const std::string& suffix);
  void setValue(int value);
  int getValue();
  void defineMinValue(int low, int high, int step, int value,
		      const std::string& prefix, const std::string& suffix);
  void setMinValue(int value);
  int  getMinValue();
  void defineMaxValue(int low, int high, int step, int value,
		      const std::string& prefix, const std::string& suffix);
  void setMaxValue(int value);
  int  getMaxValue();
  void  defineTextChoice(const std::vector<std::string>& vchoice, int ndefault=0);
  void     setTextChoice(const std::string& choice);
  std::string getTextChoice();

  void  defineTextChoice2(const std::vector<std::string>& vchoice, int ndefault=0);
  void     setTextChoice2(const std::string& choice);
  std::string getTextChoice2();

public Q_SLOTS:
  void setChecked(bool on);

private:
  VcrossManager * vcrossm;

  static bool initialized;
  //init QT stuff
  static std::vector<Colour::ColourInfo> m_cInfo; // all defined colours
  //pixmaps for combo boxes
  static int        nr_colors;
  static QColor* pixcolor;
  static int        nr_linewidths;
  static int        nr_linetypes;
  static std::vector<std::string> linetypes;

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

  std::vector<std::string> vTextChoice;
  std::vector<std::string> vTextChoice2;
};

#endif
