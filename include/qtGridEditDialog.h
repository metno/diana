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
#ifndef _GridEditDialog_h
#define _GridEditDialog_h


#include <qdialog.h>
#include <qtEditTimeDialog.h>

using namespace std;

class QWidgetStack;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLabel;
class miSliderWidget;
 
/**

  \brief Profet object table
  
   Widget used for managing objects

*/
class GridEditDialog : public QDialog
{
  Q_OBJECT
public:

  GridEditDialog( QWidget* parent, GridEditManager* gm );
  void changeDialog();
  void catchGridAreaChanged();

private:

  GridEditManager* gridm;

  vector<fetBaseObject> baseObject;
  map<miString,fetObject> object;
  map<int,miString>objectIndexMap;
  map<miString,int>objectIdMap;

  map<miString,int> idMap;

  QLabel*         parameterLabel; 

  QPushButton*    newButton;
  QComboBox*      baseObjectComboBox;
  QCheckBox*      showAllBox;
  QComboBox*      objectComboBox;
  QPushButton*    areaButton;



  QWidgetStack*   stackWidget;
  QCheckBox*      heightMaskCheckBox;
  miSliderWidget* heightMaskSlider;
  QCheckBox*      landMaskCheckBox;
  //  QCheckBox*      autoCheckBox;
  //  QPushButton*    applyButton;
  QPushButton*    hideButton;
  QPushButton*    deleteButton;


  bool newObj;
  int widgetId; //current stack
  
  void newStack(fetBaseObject fobj);
  void enableStack(fetBaseObject& fobj);

private slots:

  void newBaseObjectClicked();
  void baseObjectComboBoxActivated(int);
  void objectComboBoxActivated(int);
  void useAreaClicked();
  void autoChecked(bool);
  void applySlot();
  void deleteClicked();

public slots:


signals:

  void updateGL();
  void markCell(bool);
  void parameterChanged();
  void hideDialog();
  void apply();
  void paintMode(int);
  void gridAreaChanged();

};



#endif 






