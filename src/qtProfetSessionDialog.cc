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


#include <qpushbutton.h>
#include <qtable.h>
#include <qlistbox.h>
#include <qhbox.h>
#include <qtabwidget.h>

#include "qtProfetSessionDialog.h"
#include <qvbox.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <qtextedit.h>
#include <qlineedit.h>


ProfetSessionDialog::ProfetSessionDialog( QWidget* parent ) 
  : QDialog(parent ){

  setCaption(tr("Edit Field Session"));
  QVBoxLayout * mainLayout = new QVBoxLayout(this);
  QHBoxLayout * titleLayout = new QHBoxLayout(); 
  QHBoxLayout * buttonLayout = new QHBoxLayout();
  
  //Title
  QLabel* qls = new QTitleLabel(tr(" Session: "), this);
  QLabel* qlm = new QTitleLabel(tr(" Model: "), this);
  sessionLabel = new QDataLabel("", this );
  modelLabel = new QDataLabel("", this );
  
  titleLayout->addWidget(qls);
  titleLayout->addWidget(sessionLabel);
  titleLayout->addWidget(qlm);
  titleLayout->addWidget(modelLabel);
  
  QSplitter *split = new QSplitter(QSplitter::Vertical,this);
  
  // Table
  table = new ProfetSessionTable(split);
  if(table){
    table->setResizePolicy(QTable::Manual);
    table->setReadOnly( true );
    table->setSelectionMode( QTable::Single );
  }

  //Tab Widget
  tabWidget = new QTabWidget(split);
  //In QT4: tabWidget->setTabPosition(QTabWidget::West)
  
  //User Panel
  QHBox * userBox = new QHBox(tabWidget);
  userList = new QListBox(userBox);
  tabWidget->addTab(userBox,"&Users");
  chatWidget = new ProfetChatWidget(tabWidget);
  tabWidget->addTab(chatWidget,"&System");
  
  // Object Panel
  objectBox = new QHBox(tabWidget);
  objectBox->setSpacing(5);
  objectList = new QListBox(objectBox);
  QVBox * objectLabelBox = new QVBox(objectBox);
  QVBox * objectInfoBox = new QVBox(objectBox);
  QVBox * objectButtonBox = new QVBox(objectBox);
  QLabel * aobl = new QTitleLabel("Algorithm : ", objectLabelBox);
  QLabel * oobl = new QTitleLabel("Owner : ", objectLabelBox);
  QLabel * sobl = new QTitleLabel("Locked by : ", objectLabelBox);
  objectAlgLabel = new QDataLabel("",objectInfoBox);
  objectOwnerLabel = new QDataLabel("",objectInfoBox);
  objectStatusLabel = new QDataLabel("",objectInfoBox);
  newObjectButton = new QPushButton(tr("New"), objectButtonBox );
  editObjectButton = new QPushButton(tr("Edit"), objectButtonBox );
  deleteObjectButton = new QPushButton(tr("Delete"), objectButtonBox );
  QLabel * emptyLabel = new QLabel("", objectBox);

  tabWidget->addTab(objectBox,"&Objects");
  
  //Buttons
  updateButton = new QPushButton(tr("Update"), this );
  closeButton = new QPushButton(tr("Close"), this );
  buttonLayout->addWidget(new QLabel("", this));
  buttonLayout->addWidget(updateButton);
  buttonLayout->addWidget(closeButton);
  
  mainLayout->addLayout(titleLayout);
  mainLayout->addWidget(split);
  mainLayout->addLayout(buttonLayout);
  lockedObjectSelected(true);
  noObjectSelected();
  updateButton->setFocus();
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
  connect(objectList,SIGNAL(highlighted(const QString &)),
      this,SLOT(objectListChanged(const QString &)));
  connect(chatWidget,SIGNAL(sendMessage(const QString &)),
      this,SIGNAL(sendMessage(const QString &)));
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

miString ProfetSessionDialog::getSelectedObject(){ 
  return currentObject;
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
  objectBox->setEnabled(editable);
}

void ProfetSessionDialog::lockedObjectSelected(bool locked){
  editObjectButton->setEnabled(!locked);
  deleteObjectButton->setEnabled(!locked);
}

void ProfetSessionDialog::setUserList(const vector<Profet::PodsUser> & users){
  cerr << "ProfetSessionDialog::setUserList " << users.size() << endl;
  userList->clear();
  for(int i=0;i<users.size();i++){
    userList->insertItem(users[i].name.cStr());
  }
}

void ProfetSessionDialog::setObjectList(const vector<fetObject> & obj){
  objectList->clear();
  noObjectSelected();
  for(int i=0;i<obj.size();i++){
    objectList->insertItem(obj[i].id());
  }
  if(obj.size()){
    emit objectSelected(currentObject);
  }
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


bool ProfetSessionDialog::setCurrentObject(const fetObject & current){
  if(setSelectedObject(current.id())){
    objectAlgLabel->setText(current.name().cStr());
    objectOwnerLabel->setText(current.user());
    objectStatusLabel->setText(current.lock());
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

void ProfetSessionDialog::objectListChanged(const QString & qs){
  currentObject = qs.latin1();
  emit objectSelected(currentObject);
}


void ProfetSessionDialog::noObjectSelected(){
  QString noObj("No object selected");
  objectAlgLabel->setText(noObj);
  objectOwnerLabel->setText(noObj);
  objectStatusLabel->setText(noObj);
  lockedObjectSelected(true);
}

