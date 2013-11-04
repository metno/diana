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
#ifndef _qtBrowserBox_h
#define _qtBrowserBox_h

#include <QDialog>

#include <string.h>

class QLabel;
class QKeyEvent;

/**
   \brief Browse through history or quick menues

   Pops up a separat window for browsing through the plot history or one of the quick menues
*/
class BrowserBox : public QDialog {
  Q_OBJECT
private:
  QLabel* listname;
  QLabel* label;
  QLabel* numlabel;
  void keyPressEvent(QKeyEvent*);
  void keyReleaseEvent(QKeyEvent*);

public:
  BrowserBox(QWidget* parent);

public slots:
  void upDate(const std::string& name,
	      const int num,
	      const std::string& item);

signals:
  void selectplot();
  void prevplot();
  void nextplot();
  void prevlist();
  void nextlist();
  void cancel();
};

#endif
