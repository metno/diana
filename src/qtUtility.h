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
#ifndef _qtutility_h
#define _qtutility_h

#include <puTools/miString.h>
#include <vector>
#include <diCommonTypes.h>
#include <diColourShading.h>
#include <diPattern.h>
#include <QPixmap>
#include <QLabel>

using namespace std;

class QWidget;
class QPushButton;
class QComboBox;
class QListWidget;
class QLabel;
class QLCDNumber;
class QCheckBox;
class QSlider;
class QPixmap;
class QColor;


int getIndex( vector<miutil::miString> vstr, miutil::miString def_str );

int getIndex( vector<Colour::ColourInfo> cInfo, miutil::miString def_str );


// Lables

QLabel* TitleLabel(const QString& name, QWidget* parent);

// PushButtons

QPushButton* SmallPushButton( const QString& name, QWidget* parent);

QPushButton* NormalPushButton( const QString& name, QWidget* parent);

QPushButton* PixmapButton( const QPixmap& pixmap, QWidget* parent,
			 int deltaWidth=0, int deltaHeight=0);

// ComboBox

QComboBox* ComboBox(QWidget* parent, vector<miutil::miString> vstr,
		    bool Enabled=true, int defItem=0);

QComboBox* ComboBox(QWidget* parent, QColor* pixcolor, int nr_colors,
		    bool Enabled=true, int defItem=0);

QComboBox* ColourBox(QWidget* parent, const vector<Colour::ColourInfo>&,
         bool Enabled=true, int defItem=0,
         miutil::miString firstItem="", bool name=false);

QComboBox* ColourBox(QWidget* parent,
         bool Enabled=true, int defItem=0,
         miutil::miString firstItem="", bool name=false);

void ExpandColourBox( QComboBox* box, const Colour& col );

QComboBox* PaletteBox(QWidget* parent,
		      const vector<ColourShading::ColourShadingInfo>&,
		      bool Enabled=true, int defItem=0,
		      miutil::miString firstItem="", bool name=false);

void ExpandPaletteBox( QComboBox* box, const ColourShading& palette );

QComboBox* PatternBox(QWidget* parent, const vector<Pattern::PatternInfo>&,
		      bool Enabled=true, int defItem=0,
		      miutil::miString firstItem="", bool name=false);

QComboBox* LinetypeBox(QWidget* parent,
		    bool Enabled=true, int defItem=0);

QComboBox* LinewidthBox(QWidget* parent,
			bool Enabled=true,
			int nr_linewidths=12,
			int defItem=0);

void ExpandLinewidthBox( QComboBox* box,
    int new_nr_linewidths);

QComboBox* PixmapBox(QWidget* parent, vector<miutil::miString>& markerName);

// Div

QLCDNumber* LCDNumber(uint numDigits, QWidget* parent=0);

QSlider* Slider( int minValue, int maxValue, int pageStep, int value,
		 Qt::Orientation orient, QWidget* parent, int width );

QSlider* Slider( int minValue, int maxValue, int pageStep, int value,
		 Qt::Orientation orient, QWidget* parent );

void listWidget( QListWidget* box, vector<miutil::miString> vstr, int defItem=-1 );

QPixmap* linePixmap(const miutil::miString& pattern, int linewidth);


#endif
