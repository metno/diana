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
#ifndef _GridEditWidget_h
#define _GridEditWidget_h


#include <qwidget.h>
#include <fetCodeExecutor.h>

using namespace std;

class fetDynamicGui;
class GridEditManager;
class QCheckBox;
class QComboBox;
 
/**



*/

class GridEditWidget : public QWidget
{
  Q_OBJECT
public:

  GridEditWidget( GridEditManager* gm,  fetBaseObject& fobject);

  //  void updateWidget(int i);

private:

  GridEditManager* gridm;
  
  fetDynamicGui* fetWidget;

  vector<fetDynamicGui::GuiComponent> components;

  
private slots:
  void componentsChanged();

public slots:

//  void areaChanged();
  void execute();

signals:
  void updateGL();


};



#endif 






