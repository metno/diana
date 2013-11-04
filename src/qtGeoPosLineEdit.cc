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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtGeoPosLineEdit.h"

#include <puTools/miStringFunctions.h>

QValidator::State GeoPosLineEdit::geovalidator::validate(QString& input, int& pos) const
{
  // accepts:
  // +/-XX.yy +/-ZZ.rr   OR
  // XX.yyS ZZ.rrE OR
  // +/-XX:yy:ss +/-ZZ:rr:tt   OR
  // +/-XX:yy +/-ZZ:rr  OR
  // XX:yyN ZZ:rrW OR
  // mixes of those above

  // zero length strings are ok
  if (!input.length())
    return QValidator::Intermediate;

  // entered char
  QChar a = input[pos - 1];

  // Hmmm..
  if (pos > input.length())
    return QValidator::Intermediate;

  // check for illegal characters
  if (!a.isDigit() && a != '-' && a != '+' && a != '.' && a != ':' && a != ' '
    && a != 'N' && a != 'S' && a != 'E' && a != 'W')
    return QValidator::Invalid;

  // check if numbers are legal, and do not accept
  // more than 2 numbers
  int n = input.length();
  int i = 0, nnum = 0;

  while (1) {
    while (i < n && input[i].isSpace())
      i++;
    if (i == n)
      break;
    else
      nnum++;

    // max two numbers
    if (nnum > 2)
      return QValidator::Invalid;

    int i1 = i;
    // check for appearance of different characters
    bool hascomma = false;
    bool hassemicolon = false;
    bool hassign = false;
    bool hasletter = false;
    int numsemicolons = 0;
    while (i < n && !input[i].isSpace()) { // skip space
      if (input[i] == '.') { // we found a comma
        if (hascomma || hassemicolon)
          return QValidator::Invalid;
        hascomma = true;

      } else if (input[i] == ':') { // we found a semicolon
        if (hascomma || numsemicolons > 1)
          return QValidator::Invalid;
        hassemicolon = true;
        numsemicolons++;

      } else if (input[i] == '+' || input[i] == '-') { // we found a sign!
        if (hassign || hasletter)
          return QValidator::Invalid;
        hassign = true;

      } else if (input[i] == 'S' || input[i] == 'W' || input[i] == 'N'
        || input[i] == 'E') { // we found a letter
        // last character only
        if ((i < n - 1) && !input[i + 1].isSpace())
          return QValidator::Invalid;
        // mix of sign and letter illegal
        if (hassign || hasletter)
          return QValidator::Invalid;
        hasletter = true;
      }
      i++;
    }
    // make a proper number of last found substring
    QString b = input.mid(i1, i - i1);
    float testval;
    if (!toFloat(b.toStdString(), testval, nnum == 1))
      return QValidator::Invalid;
  }

  if (nnum < 2) // ok, but not quite ready
    return QValidator::Intermediate;

  // all is well
  return QValidator::Acceptable;
    }

// Convert string to decimal value
// If string contains ':', deg:min:sec format assumed
bool GeoPosLineEdit::geovalidator::toFloat(std::string s, float& val,
    bool isLat) const
    {

  float testval = 0;
  float lettersign = 1.0;
  // check for letters
  if (miutil::contains(s, "S")) {
    if (!isLat)
      return false;
    lettersign = -1.0;
    miutil::replace(s, 'S', ' ');
  } else if (miutil::contains(s, "N")) {
    if (!isLat)
      return false;
    lettersign = 1.0;
    miutil::replace(s, 'N', ' ');
  } else if (miutil::contains(s, "W")) {
    if (isLat)
      return false;
    lettersign = -1.0;
    miutil::replace(s, 'W', ' ');
  } else if (miutil::contains(s, "E")) {
    if (isLat)
      return false;
    lettersign = 1.0;
    miutil::replace(s, 'E', ' ');
  }

  if (miutil::contains(s, ":")) { // degrees:minutes:seconds
    std::vector<std::string> vs = miutil::split(s, ":");
    float asign = 1.0;
    for (unsigned int k = 0; k < vs.size(); k++) {
      float tval = atof(vs[k].c_str());
      if (k == 0) // degrees (determines sign)
        asign = (miutil::contains(vs[k], "-") ? -1.0 : 1.0);
      else { // minutes or seconds
        if (tval > 59.999999)
          return false;
        tval *= asign;
        if (k == 1)
          tval /= 60.0; // minutes
        else if (k == 2)
          tval /= 3600.0; // seconds
      }
      testval += tval;
    }
  } else { // decimal degrees
    testval = atof(s.c_str());
  }

  testval *= lettersign; // modify sign
  // check for limits
  if (isLat) {
    if (testval > 90.00 || testval < -90.00)
      return false;
  } else {
    if (testval > 180.00 || testval < -180.00)
      return false;
  }
  val = testval;
  return true;
    }

// called when "enter" pressed or widget loses focus...IF
// "validate" do not return QValidator::Acceptable
void GeoPosLineEdit::geovalidator::fixup(QString& input) const
{
  //cerr << "Fix up string:" << input << endl;
}

// ------------------------------------------------------------


// get latitude, longitude from LineEdit-string
bool GeoPosLineEdit::getValues(float& lat, float& lng)
{
  std::string s(text().toStdString());
  miutil::trim(s);
  std::vector<std::string> vs = miutil::split(s, " ");
  if (vs.size() != 2)
    return false;

  if (!gv->toFloat(vs[0], lat, true))
    return false;
  return gv->toFloat(vs[1], lng, false);
}

GeoPosLineEdit::GeoPosLineEdit(QWidget* p)
  : QLineEdit(p)
{
  gv = new geovalidator(0);
  setValidator(gv);
}
