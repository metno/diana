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

#include <qtGridEditWidget.h>
#include <diGridEditManager.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <miSliderWidget.h>
#include <qlayout.h>

#include <iostream>

GridEditWidget::GridEditWidget( GridEditManager* gm, 
				fetBaseObject& fobject)
  : QWidget() //parent
{
//   cerr << "******** GridEditWidget::GridEditWidget for object "
//     << fobject.name() << endl;

  gridm = gm;

  QVBoxLayout* layout = new QVBoxLayout(this);

  // get GUI-components from baseObject
  components = gridm->setObject(fobject,true);

  // Generate widget from gui components
  fetWidget = new fetDynamicGui(this,components);

  connect( fetWidget, SIGNAL( valueChanged() ),
	   this, SLOT( componentsChanged() ) );
  
  layout->addWidget(fetWidget);
}


void GridEditWidget::componentsChanged()
{
  if( gridm->autoExecute() ){
    execute();
  }
}

// void GridEditWidget::areaChanged()
// {
//   cerr <<" GridEditWidget::areaChanged()"<<endl;
//   gridm->execute();
//   emit updateGL();
// }

void GridEditWidget::execute()
{

  components.clear();
  fetWidget->getValues(components);
  gridm->prepareObject(components);
  
  gridm->execute();
  emit updateGL();
  
}


