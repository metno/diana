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
#ifndef _EditTimeDialog_h
#define _EditTimeDialog_h


#include <qdialog.h>
#include <qpixmap.h>
#include <diGridEditManager.h>
#include <qframe.h>
#include <qlabel.h>
#include <qscrollview.h>

using namespace std;

class QTable;
class QComboBox;
class QPushButton;
class GridEditDialog;
 
/**

  \brief Profet object table
  
   Widget used for managing objects

*/
class EditTimeDialog : public QDialog
{
  Q_OBJECT
public:

  EditTimeDialog( QWidget* parent, GridEditManager* gm );

private:

  GridEditManager* gridm;

  /// backgrounds

  QPixmap gpixmap; // lightgray (secday)
  QPixmap rpixmap; // red  ( object found) 
  QPixmap ypixmap; // yellow (curent window) 
  
  int ccol;  // currentcol for background tracking
  int crow;  // currentrow for background tracking

  QLabel         *header;
  QFrame         *tableFrame;
  QScrollView    *sv;
  QTable         *table;
  QHBoxLayout    *dayTable;
  QComboBox      *objectComboBox;
  GridEditDialog *gridEditDialog;
  QPushButton    *hideButton;
  QPushButton    *refreshButton;
  QPushButton    *quitButton;
  
  map<int,miTime> timeMap;
  map<miString,int> parameterMap;
  bool            initialized;
  vector<bool>    whitecell;
  

  bool init();

private slots:

  void cellClicked(int row, int col);
  void refreshClicked();
  void quitClicked();
  void markCell(bool);
  void parameterChanged();

public slots:

  void catchGridAreaChanged();
  void showDialog();

signals:
  void showField(miString);
  void setTime(const miTime&);
  void newStack(const miString&);
  void apply();
  void hideDialog();
  void updateGL();
  void paintMode(int);
  void gridAreaChanged();
};



#endif 






