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
#ifndef _qtGeoPosLineEdit_h
#define _qtGeoPosLineEdit_h

#include <qlineedit.h>
#include <qvalidator.h>

#include <string>

/**
  \brief An input widget for geographical positions

  A Qt line editor with validator for geographical positions
*/
class GeoPosLineEdit : public QLineEdit {
  Q_OBJECT
private:

  class geovalidator : public QValidator {
  public:
    geovalidator(QWidget * parent)
      : QValidator(parent) {}
    virtual State validate(QString&,int&) const;
    virtual void fixup(QString&) const;
    bool toFloat(std::string s, float& val,
		 bool isLat) const;
  };
  geovalidator* gv;

public:
  GeoPosLineEdit(QWidget*);
  // get latitude, longitude from LineEdit-string
  bool getValues(float& lat, float& lng);
};

#endif
