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

#include <QHBoxLayout>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHeaderView>


ProfetSessionDialog::ProfetSessionDialog( QWidget* parent) 
  : QDialog(parent ){
  setWindowTitle(tr("Edit Field Session"));
  setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
  QVBoxLayout * mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(2);
  QHBoxLayout * titleLayout = new QHBoxLayout();
  QVBoxLayout * centerLayout = new QVBoxLayout();
  QHBoxLayout * buttonLayout = new QHBoxLayout();
  mainLayout->addLayout(titleLayout);
  mainLayout->addLayout(centerLayout);
  mainLayout->addLayout(buttonLayout);

  QLabel * qls = new QLabel("Session");
  sessionComboBox = new QComboBox();
  
  titleLayout->addWidget(qls);
  titleLayout->addWidget(sessionComboBox);
  
  QSplitter *split = new QSplitter(Qt::Vertical,this);
  split->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
  // stretch table only
  split->setStretchFactor(0,10);
  split->setStretchFactor(1,0);
  centerLayout->addWidget(split);
  
  // Table
  table = new FetObjectTableView(split);
  if(table){
    table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    table->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
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
    connect(table,SIGNAL(clicked(const QModelIndex &)),
        this,SIGNAL(paramAndTimeChanged(const QModelIndex &)));
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
  connect(sessionComboBox,SIGNAL(activated(int)),
      this,SIGNAL(sessionSelected(int)));
}

void ProfetSessionDialog::printSize(const QModelIndex &){
  cerr << "size: " << table->size().height() << endl;
  cerr << "minimumSize: " << table->minimumSize().height() << endl;
  cerr << "columnWidth: " << table->columnWidth(1) << endl;
}

void ProfetSessionDialog::setUserModel(QAbstractItemModel * userModel){
  chatWidget->setUserModel(userModel);
}


void ProfetSessionDialog::setSessionModel(QAbstractItemModel * sessionModel){
  sessionComboBox->setModel(sessionModel);
}

void ProfetSessionDialog::setTableModel(QAbstractItemModel * tableModel){
  table->setModel(tableModel);
}

void ProfetSessionDialog::selectDefault(){
  paramAndTimeChanged(table->model()->index(0,0));
}

void ProfetSessionDialog::setSelectedObject(const QModelIndex & index){
  objectList->setCurrentIndex(index);
}

QModelIndex ProfetSessionDialog::getCurrentObjectIndex(){
  return objectList->currentIndex();
}

void ProfetSessionDialog::showMessage(const Profet::InstantMessage & msg){ 
  chatWidget->showMessage(msg); 
}


void ProfetSessionDialog::closeEvent(QCloseEvent * e){
  emit closePerformed();
}

void ProfetSessionDialog::setEditable(bool editable){
  if(table){
//    table->setEnabled(editable);
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

void ProfetSessionDialog::setCurrentSession(const QModelIndex & index){
  sessionComboBox->setCurrentIndex(index.row());
}

void FetObjectListView::currentChanged ( const QModelIndex & current,
      const QModelIndex & previous ){
  emit activated(current); 
}



