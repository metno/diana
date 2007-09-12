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

#include <qtGridEditDialog.h>
#include <qtGridEditWidget.h>
#include <qtUtility.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qwidgetstack.h>
#include <miSliderWidget.h>
#include <qlayout.h>
#include <qfile.h>
#include <iostream>
#include <qtooltip.h>

GridEditDialog::GridEditDialog( QWidget* parent, GridEditManager* gm)
    : QDialog(parent)
{
  gridm = gm;
  
  setCaption(tr("Profet"));

  
  QVBoxLayout* layout = new QVBoxLayout(this);
  

  // Title: Font+2 / colored heading

  parameterLabel = new QLabel(this);
  QFont Fo       = parameterLabel->font();
  Fo.setPointSize(Fo.pointSize()+2);

  parameterLabel->setFrameStyle( QFrame::Panel | QFrame::Raised );
  parameterLabel->setBackgroundColor(QColor(88,150,237));
  parameterLabel->setFont(Fo);

  layout->addWidget( parameterLabel ); 

  // New Object Frame --------------------------

  QFrame * newFrame = new QFrame(this); 
  newFrame->setFrameStyle( QFrame::Panel | QFrame::Sunken   );
  newFrame->setLineWidth(1);
  newFrame->setMargin(3);

  QGridLayout *glayout = new QGridLayout(newFrame,3,3,5,5);

  QLabel    *baseLabel = new QLabel(tr("<b>Algorithms:</b>"), newFrame);
  QLabel    *areaLabel = new QLabel(tr("<b>Area:</b>"      ), newFrame);
  QLabel    *newLabel  = new QLabel(tr("<b>New Object:</b>"), newFrame);

  Fo.setPointSize(Fo.pointSize()-1);
  newLabel->setFont(Fo);

  baseObjectComboBox = new QComboBox(newFrame);

  newButton  = new QPushButton(tr("Draw"   ), newFrame);
  areaButton = new QPushButton(tr("Current"), newFrame);

  QToolTip::add(baseObjectComboBox,tr("Choose algorithm for the new object"));
  QToolTip::add(newButton,tr("Start a new object by drawing a new area"));
  QToolTip::add(areaButton,tr("Start a new object by using the current area"));

  connect(baseObjectComboBox, SIGNAL( activated ( int )), 
	  SLOT( baseObjectComboBoxActivated(      int )));
  connect(newButton,  SIGNAL(clicked()), SLOT(newBaseObjectClicked()));
  connect(areaButton, SIGNAL(clicked()), SLOT(useAreaClicked()      ));


  glayout->addMultiCellWidget (newLabel ,0,0,0,2); 
  glayout->addWidget(baseLabel,   1, 0 ); 
  glayout->addMultiCellWidget(baseObjectComboBox,1,1,1,2); 
  glayout->addWidget( areaLabel,  2, 0 );
  glayout->addWidget( newButton,  2, 1 );
  glayout->addWidget( areaButton, 2, 2 );

  layout->addWidget(newFrame);


  /// The stack ----------------------------------
  

  stackWidget = new QWidgetStack(this);
  layout->addWidget( stackWidget );
 
  QGridLayout* g2layout = new QGridLayout(2,2);


  /// Choose object ------------------------------

  QLabel* inuseLabel    = new QLabel(tr("<b>Choose Object:</b>"),this);
  objectComboBox        = new QComboBox(this);
 
  g2layout->addWidget( inuseLabel,     0 ,0 );  
  g2layout->addWidget( objectComboBox ,0 ,1 ); 
 
  connect(objectComboBox, SIGNAL( activated ( int )), 
	  SLOT( objectComboBoxActivated(      int )));

  /// Apply ---------------------------------------

//   autoCheckBox = new QCheckBox(tr("Automatic apply"),this);

//   connect( autoCheckBox, SIGNAL( toggled(bool) ), SLOT( autoChecked(bool) ));
//   autoCheckBox->setChecked(false);
// //   autoChecked(true);

//   g2layout->addWidget( autoCheckBox, 1,0);

//   applyButton = new QPushButton(tr("Apply"),this);
//   connect( applyButton, SIGNAL( clicked() ), SIGNAL( apply()   ));
//   connect( applyButton, SIGNAL( clicked() ), SLOT( applySlot() ));
//   g2layout->addWidget( applyButton, 1,1);
  

  /// Hide and delete -------------------------------

  hideButton = new QPushButton(tr("Hide"),this);
  connect( hideButton, SIGNAL( clicked() ), SLOT( hide()         ));
  connect( hideButton, SIGNAL( clicked() ), SIGNAL( hideDialog() ));
  g2layout->addWidget( hideButton, 1,0);

  deleteButton = new QPushButton(tr("Delete algorithm"),this);
  connect( deleteButton, SIGNAL( clicked() ), SLOT( deleteClicked() ));
  g2layout->addWidget( deleteButton, 1,1);


  layout->addLayout( g2layout );

  newObj =false;
}

void GridEditDialog::changeDialog()
{ 

  //  cerr <<"GridEditDialog::changeDialog()"<<endl;

  miString parameterText=gridm->getCurrentParameter();
  parameterText="<b>"+parameterText+"</b>";
  
  parameterLabel->setText(parameterText.cStr());

  baseObjectComboBox->clear();
  baseObject = gridm->getBaseObjects();

  for( int i=0; i<baseObject.size();i++){
    baseObjectComboBox->insertItem(baseObject[i].name().cStr());
  }

  newButton->setEnabled(baseObject.size());

  //object
  objectIndexMap.clear();
  objectIdMap.clear();
  objectComboBox->clear();
  object = gridm->getObjects();
  map<miString,fetObject>::iterator itr = object.begin();
  map<miString,fetObject>::iterator end = object.end();
  int current = 0;
//   miString currentId = gridm->getObjectId();
  // AC: gridm->getObjectId() returnerer rawObject.id(), og rawObject er ikke satt riktig ennå!
  miString currentId = gridm->areaId();
  for (int i=0 ; itr!=end; itr++,i++ ){ 
    if(itr->first == currentId) current = i;
    objectIndexMap[i]=itr->first;
    objectIdMap[itr->first] = i;
    objectComboBox->insertItem(itr->second.name().cStr());
  }

  //if objects, set current object
  if(objectComboBox->count()>0 ){
    objectComboBox->setCurrentItem(current);
    objectComboBoxActivated(current);
  }

  //guiElements
  //  stackWidget->setEnabled(false);

  areaButton->setEnabled(gridm->isAreaSelected());
  
  emit paintMode(GridAreaManager::SELECT_MODE);


  show();
}

void GridEditDialog::baseObjectComboBoxActivated(int index)
{

  newStack(baseObject[baseObjectComboBox->currentItem()]);
  newObj = false;

}

void GridEditDialog::newBaseObjectClicked()
{
  //  cerr <<"newBaseObjectClicked()"<<endl;
  if( newObj ) return;
  if( !baseObjectComboBox->count() ) return;
  newStack(baseObject[baseObjectComboBox->currentItem()]);
  gridm->addArea(true);
  emit paintMode(GridAreaManager::DRAW_MODE);
  emit gridAreaChanged();
}

void GridEditDialog::useAreaClicked()
{

  newStack(baseObject[baseObjectComboBox->currentItem()]);
  gridm->addArea(false); //use current area
  emit updateGL(); //needed??

}

void GridEditDialog::objectComboBoxActivated(int index)
{

  //change stack, if possible
  miString id = objectIndexMap[index];

  if(idMap.count(id)) {
    stackWidget->raiseWidget(idMap[id]);  
  } else { //or make new stack
    newStack(object[id].baseObject());
  }

  //update baseObject ComboBox
  miString name = object[id].baseObject().name();
  int n=baseObject.size();
  int i=0;
  while(i<n && name != baseObject[i].name() ) i++;
  if(i<baseObjectComboBox->count())
    baseObjectComboBox->setCurrentItem(i);

  //update GridEditManger
  gridm->setCurrentObject(id);

  //apply object
  applySlot();

}
 
void GridEditDialog::autoChecked(bool on)
{
  gridm->setAutoExecute(on);
}

void GridEditDialog::applySlot()
{
  gridm->execute(true);
  emit updateGL();
}

void GridEditDialog::deleteClicked()
{
  
  stackWidget->removeWidget(stackWidget->visibleWidget());
  gridm->deleteObject();
  changeDialog();
  //update time dialog
  emit markCell(objectComboBox->count());
  applySlot();

}



void GridEditDialog::catchGridAreaChanged()
{
  //  cerr <<"GridEditDialog::gridAreaChanged()"<<endl;

  areaButton->setEnabled(gridm->isAreaSelected());
 
  // We are waiting for a polygon for a new object
  if(newObj) {
    if(gridm->isAreaSelected()) {
      cerr << "We have a Polygon for our new object ... create the new object"<<endl;
      newObj = false;
      enableStack(baseObject[baseObjectComboBox->currentItem()]);
      return;
    }
  }

  miString prevParameter = gridm->getCurrentParameter();

 // changes in polygon for current object
  if (!gridm->changeArea()) {
    emit updateGL();// may have run execute
    return;
  }

  // selection of new Polygon/Object 
  if( prevParameter != gridm->getCurrentParameter()) {
    cerr <<"    //if new parameter - send SIGNAL to  EditTimeDialog"<<endl;
    emit parameterChanged();
  } else {
    cerr <<"    //same parameter - set current object"<<endl;
    int index = objectIdMap[gridm->areaId()];
    objectComboBox->setCurrentItem(index);
    objectComboBoxActivated(index);
  }
    
  return;
  
}

void GridEditDialog::newStack(fetBaseObject fobject)
{
  //  cerr <<"GridEditDialog::newStack for baseObject:"<<fobject.name()<<endl;
  GridEditWidget* widget = new GridEditWidget(gridm,fobject);
  connect( widget, SIGNAL( updateGL()),            SIGNAL(updateGL()       ));
  //  connect( this,   SIGNAL( areaChanged()), widget, SLOT( areaChanged()     ));
  connect( this,   SIGNAL( apply()),       widget, SLOT( execute() ));

  widgetId = stackWidget->addWidget(widget);
  stackWidget->raiseWidget(widgetId);
  newObj = true;

  stackWidget->setEnabled(gridm->isAreaSelected());

}

void GridEditDialog::enableStack(fetBaseObject& fobject)
{

  //  cerr <<"enableStack:"<< fobject.name() << endl;

  stackWidget->setEnabled(true);
  gridm->setObject(fobject,false);
  miString objectId = gridm->getObjectId();

  //update objects in use
  object[objectId] = gridm->getCurrentObject();
  objectComboBox->insertItem(object[objectId].name().cStr());
  objectComboBox->setCurrentItem(  objectComboBox->count()-1 );
  objectIndexMap[ objectComboBox->count()-1] = objectId;
  objectIdMap[objectId] = objectComboBox->count()-1;

  emit markCell(objectComboBox->count());

  //update areas

  idMap[objectId]=widgetId;
  applySlot();

}




