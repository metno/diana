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

#include <QDialog>
#include <diCommonTypes.h>
#include <string>

class QPushButton;
class QCheckBox;
class QTextBrowser;

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
  QTextBrowser* tb;
  QCheckBox* fixedb;
  InfoFile infofile;
  std::string path;
};

#endif
