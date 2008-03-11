/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "qtProfetSessionDialog.h"

#include <Q3Table>

#include <QHBoxLayout>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QSplitter>


ProfetSessionDialog::ProfetSessionDialog( QWidget* parent) 
  : QDialog(parent ){
  setCaption(tr("Edit Field Session"));
  QVBoxLayout * mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(2);
  QHBoxLayout * titleLayout = new QHBoxLayout();
  QVBoxLayout * centerLayout = new QVBoxLayout();
  QHBoxLayout * buttonLayout = new QHBoxLayout();
  mainLayout->addLayout(titleLayout);
  mainLayout->addLayout(centerLayout);
  mainLayout->addLayout(buttonLayout);

  //Title
  QLabel* qls = new QTitleLabel(tr(" Session: "), this);
  QLabel* qlm = new QTitleLabel(tr(" Model: "), this);
  sessionLabel = new QDataLabel("", this );
  modelLabel = new QDataLabel("", this );
  
  titleLayout->addWidget(qls);
  titleLayout->addWidget(sessionLabel);
  titleLayout->addWidget(qlm);
  titleLayout->addWidget(modelLabel);
  
  QSplitter *split = new QSplitter(Qt::Vertical,this);
  // stretch table only
  split->setStretchFactor(0,10);
  split->setStretchFactor(1,0);
  centerLayout->addWidget(split);
  
  // Table
  table = new ProfetSessionTable(split);
  if(table){
    
    table->setResizePolicy(Q3Table::Manual);
    table->setReadOnly( true );
    table->setSelectionMode( Q3Table::Single );
  }

  QSplitter *h_split = new QSplitter(Qt::Horizontal,split);
  chatWidget = new ProfetChatWidget(h_split);
  
  // Object Panel
  QWidget *objectWidget = new QWidget(h_split);
  QVBoxLayout * objectTitleLayout = new QVBoxLayout();
  objectTitleLayout->setMargin(0);
  objectTitleLayout->addWidget(new QLabel(tr("Objects")),0);
  QHBoxLayout * objectWidgetLayout = new QHBoxLayout();
  objectTitleLayout->addLayout(objectWidgetLayout,1);
  objectList = new FetObjectListView(this);
  objectWidgetLayout->addWidget(objectList,1);
  QWidget *objectButtonWidget = new QWidget();
  objectWidgetLayout->addWidget(objectButtonWidget,0);
  QVBoxLayout *objectButtonWidgetLayout = new QVBoxLayout();
  newObjectButton = new QPushButton(tr("New"));
  editObjectButton = new QPushButton(tr("Edit"));
  deleteObjectButton = new QPushButton(tr("Delete"));
  objectButtonWidgetLayout->addWidget(newObjectButton);
  objectButtonWidgetLayout->addWidget(editObjectButton);
  objectButtonWidgetLayout->addWidget(deleteObjectButton);
  
  objectButtonWidget->setLayout(objectButtonWidgetLayout);
  objectWidget->setLayout(objectTitleLayout);
  
  //Buttons
  updateButton = new QPushButton(tr("Update"), this );
  updateButton->setDefault(false);
  closeButton = new QPushButton(tr("Close"), this );
  closeButton->setDefault(false);
  buttonLayout->addWidget(new QLabel("", this));
  buttonLayout->addWidget(updateButton);
  buttonLayout->addWidget(closeButton);
  

  lockedObjectSelected(true);
  connectSignals();
}

void ProfetSessionDialog::connectSignals(){
  if(table){
    connect(table,SIGNAL(paramAndTimeChanged(miString,miTime)),
      this,SIGNAL(paramAndTimeChanged(miString,miTime)));
  }
  connect(newObjectButton,SIGNAL(clicked()),
      this,SIGNAL(newObjectPerformed()));
  connect(editObjectButton,SIGNAL(clicked()),
      this,SIGNAL(editObjectPerformed()));
  connect(deleteObjectButton,SIGNAL(clicked()),
      this,SIGNAL(deleteObjectPerformed()));
  connect(closeButton,SIGNAL(clicked()),
      this,SIGNAL(closePerformed()));
  connect(objectList,SIGNAL(activated(const QModelIndex &)),
      this,SIGNAL(objectSelected(const QModelIndex &)));
  connect(chatWidget,SIGNAL(sendMessage(const QString &)),
      this,SIGNAL(sendMessage(const QString &)));
}

void ProfetSessionDialog::setUserModel(QAbstractItemModel * userModel){
  chatWidget->setUserModel(userModel);
}

miString ProfetSessionDialog::getSelectedParameter() const { 
  if(table){
    return table->selectedParameter();
  }
  return "T2M";
}

miTime ProfetSessionDialog::getSelectedTime() const { 
  if(table){
    return table->selectedTime();
  }
  return miTime::nowTime();
}

void ProfetSessionDialog::selectDefault(){ 
  if(table){
    table->selectDefault();
  }
}

void ProfetSessionDialog::setSelectedObject(const QModelIndex & index){
  objectList->setCurrentIndex(index);
}

QModelIndex ProfetSessionDialog::getCurrentObjectIndex(){
  return objectList->currentIndex();
}

void ProfetSessionDialog::setObjectSignatures( vector<fetObject::Signature> s){
  if(table){
    table->setObjectSignatures(s);
  }
}
  
void ProfetSessionDialog::showMessage(const Profet::InstantMessage & msg){ 
  chatWidget->showMessage(msg); 
}


void ProfetSessionDialog::closeEvent(QCloseEvent * e){
  emit closePerformed();
}

void ProfetSessionDialog::setEditable(bool editable){
  if(table){
    table->setEnabled(editable);
  }
//  objectWidget->setEnabled(editable);
}

void ProfetSessionDialog::lockedObjectSelected(bool locked){
  editObjectButton->setEnabled(!locked);
  deleteObjectButton->setEnabled(!locked);
}

void ProfetSessionDialog::setObjectModel(QAbstractItemModel * objectModel){
  objectList->setModel(objectModel);
}

void ProfetSessionDialog::setModel(const fetModel & model){
  modelLabel->setText(model.model().cStr());
}

void ProfetSessionDialog::initializeTable(  const vector<fetParameter> & p,
    const fetSession & s){
  miString o= s.session().format("%k:00 %A %e.%b");
  sessionLabel->setText(o.cStr());
  if(table){
    table->initialize(p,s.progs());
  }
}

/*

bool ProfetSessionDialog::setCurrentObject(const fetObject & current){
  if(setSelectedObject(current.id())){
    lockedObjectSelected(current.is_locked());
  }
  else{
    return false;
  }
  return true;
}

bool ProfetSessionDialog::setSelectedObject(const miString & id){
  for(int i=0;i<objectList->count();i++){
    miString t = objectList->item(i)->text().latin1();
    if(t==id){
      objectList->setCurrentItem(i);
      return true;
    }
  }
  return false;
}

*/


