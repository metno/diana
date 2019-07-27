/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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
#ifndef _satdialogadvanced_h
#define _satdialogadvanced_h

#include <QWidget>

#include "diColour.h"
#include "diSatPlotCommand.h"

#include <vector>

class ToggleButton;
class SliderValues;

class QCheckBox;
class QSlider;
class QPushButton;
class QLCDNumber;
class QListWidget;

/**
  \brief Advanced dialogue for plotting satellite and radar pictures
  
  displayed when more>> is pressed in satellite dialog. colour cuts, alpha value etc.
*/
class SatDialogAdvanced: public QWidget
{
   Q_OBJECT

public:
  SatDialogAdvanced(QWidget* parent, const SliderValues& sv_cut, const SliderValues& sv_alphacut, const SliderValues& sv_alpha);

  /// put settings from widgets into plot command
  void applyToCommand(SatPlotCommand_p cmd);

  /// set the dialogue elements from a plot command
  void setFromCommand(SatPlotCommand_cp cmd);

  /// set picture string
  void setPictures(const std::string&);
  /// set colours from palette in colourlist (to hide colours in picture)
  void setColours(const std::vector<Colour>&);
  /// disable/enable options according to type of picture
  void greyOptions();

  void setOff();

Q_SIGNALS:
  ///emitted when dialog changed
  void SatChanged();
  ///emitted when colourcut clicked
  void getSatColours();

public Q_SLOTS:
  void setStandard();

private Q_SLOTS:
  void cutCheckBoxSlot( bool on );
  void greyCut( bool on );
  void greyAlphaCut( bool on );
  void greyAlpha( bool on );
  void cutDisplay( int number );
  void alphacutDisplay( int number );
  void alphaDisplay( int number );
  void colourcutClicked(bool);
  void colourcutOn();

private:
  void blockSignals(bool b);

private:
  const SliderValues& m_cut;
  const SliderValues& m_alphacut;
  const SliderValues& m_alpha;

  float m_cutnr;
  float m_alphacutnr;
  float m_alphanr;

  // true if selected picture is palette file
  bool palette;

  std::string picturestring; // string describing selected picture

  QLCDNumber* cutlcd;
  QLCDNumber* alphacutlcd;
  QLCDNumber* alphalcd;

  QSlider* scut;
  QSlider* salphacut;
  QSlider* salpha;

  QCheckBox* cutCheckBox;
  ToggleButton* cut;
  ToggleButton* alphacut;
  ToggleButton* alpha;
  ToggleButton* legendButton;
  ToggleButton* colourcut;

  QPushButton* standard;

  QListWidget * colourList;
};

#endif
