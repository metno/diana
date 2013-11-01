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
#ifndef _satdialogadvanced_h
#define _satdialogadvanced_h

#include <diColour.h>
#include "qtToggleButton.h"
#include "diController.h"

#include <qdialog.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlayout.h>
#include <QListWidget>

#include <vector>

class QCheckBox;
class QSlider;
class QPushButton;
class QLCDNumber;

/**
  \brief Advanced dialogue for plotting satellite and radar pictures
  
  displayed when more>> is pressed in satellite dialog. colour cuts, alpha value etc.
*/
class SatDialogAdvanced: public QWidget
{
   Q_OBJECT

public:
   SatDialogAdvanced( QWidget* parent,  SatDialogInfo info);
  /// the plot info strings
  std::string getOKString();
  /// set the dialogue elements from a plot info string
  std::string putOKString(std::string);
  /// set picture string
  void setPictures(std::string);
  /// set colours from palette in colourlist (to hide colours in picture)
  void setColours(std::vector<Colour>&);
  /// disable/enable options according to type of picture
  void greyOptions();
 // true if selected picture is palette file
  bool palette;
  /// true if colourcut button pressed
  bool colourCutOn(){return (colourcut->isChecked());}

protected:
  ///called when the dialog is closed by the window manager
  void closeEvent( QCloseEvent* );

signals:
  /// emit when close selected
  void SatHide();
  ///emitted when dialog changed
  void SatChanged();
  ///emitted when colourcut clicked
  void getSatColours();

  private slots:
  void cutCheckBoxSlot( bool on );
  void greyCut( bool on );
  void greyAlphaCut( bool on );
  void greyAlpha( bool on );
  void cutDisplay( int number );
  void alphacutDisplay( int number );
  void alphaDisplay( int number );
  void colourcutClicked(bool);
  void colourcutOn();

public slots:
  void setStandard();
  void setOff();


private:
  float m_cutscale;
  float m_alphacutscale;
  float m_alphascale;

  float m_cutnr;
  float m_alphacutnr;
  float m_alphanr;

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

  SliderValues m_cut;
  SliderValues m_alphacut;
  SliderValues m_alpha;

  QListWidget * colourList;

  std::string picturestring; //string describing selected picture

  void blockSignals(bool b);
};

#endif



