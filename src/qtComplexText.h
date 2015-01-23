/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef QTCOMPLEXTEXT_H
#define QTCOMPLEXTEXT_H

#include "diCommonTypes.h"

#include <qdialog.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qvalidator.h>

#include <set>

class Controller;

/**
   \brief Dialogue for entering texts for complex symbol

   several texts and colours to be entered
*/
class ComplexText : public QDialog
{
  Q_OBJECT
public:

  /// constructor, symboltext and xtext is text to put in input boxes, cList is the list of texts to choose from
  ComplexText( QWidget* parent, Controller* llctrl, std::vector<std::string> &
      symbolText, std::vector<std::string>& xText, std::set<std::string> cList,
	       bool useColour=false);
  /// get text from dialogs input boxes
  void getComplexText(std::vector<std::string>& symbolText, std::vector<std::string>& xText);
  /// set colourbox colour
  void setColour(Colour::ColourInfo & colour);
  /// get colour from colourbox
  void getColour(Colour::ColourInfo & colour);
  ~ComplexText();


private:
  Controller*    m_ctrl;

  std::vector <QComboBox*> vSymbolEdit;
  std::vector <QLineEdit*> vXEdit;

  QComboBox * colourbox;

  static bool initialized;
  static std::vector<Colour::ColourInfo> colourInfo; // all defined colours
  //pixmaps for combo boxes
  static int        nr_colors;
  static QColor* pixcolor;

  int getColourIndex(std::vector<Colour::ColourInfo>& colourInfo,
		     Colour::ColourInfo colour);
  class complexValidator:public QValidator{
  public:
    complexValidator(QWidget * parent)
      : QValidator(parent) {}
    virtual State validate(QString&,int&) const;
    // virtual void fixup(QString&) const;
  };
  complexValidator * cv;
  bool startEdit;
  void selectText(int);

private Q_SLOTS:
  void textActivated(const QString &);
  void textSelected();
};

#endif
