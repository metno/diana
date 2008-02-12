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
#ifndef _textdialog_h
#define _textdialog_h

#include <qdialog.h>
#include <qfont.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <diCommonTypes.h>

using namespace std;

class Q3VBoxLayout;
class Q3HBoxLayout;
class QPushButton;
class QCheckBox;
class Q3TextBrowser;

/**
   \brief Ascii text viewer
   
   Dialogue for displaying information text from ascii files

*/

class TextDialog: public QDialog
{
  Q_OBJECT
public:
  TextDialog(QWidget* parent, const InfoFile ifile);
  
  void setSource(const InfoFile ifile);

public slots:
  void finish();
  void openwild();
  void fixedfont();

private:
  Q3TextBrowser* tb;
  QCheckBox* fixedb;
  InfoFile infofile;
  miString path;

  //toplayout
  Q3VBoxLayout* vlayout;
  Q3HBoxLayout* hlayout;
};


#endif
