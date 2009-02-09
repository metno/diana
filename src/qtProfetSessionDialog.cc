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
#include "qtProfetEvents.h"
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QWindowsStyle>


ProfetSessionDialog::ProfetSessionDialog( QWidget* parent)
  : QDialog(parent )
  {
  setWindowTitle(tr("Edit Field Session"));
  setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

  QVBoxLayout * mainLayout   = new QVBoxLayout(this);
  mainLayout->setMargin(2);
  QHBoxLayout * titleLayout  = new QHBoxLayout();
  QVBoxLayout * centerLayout = new QVBoxLayout();
  QHBoxLayout * buttonLayout = new QHBoxLayout();

  mainLayout->addLayout(titleLayout);
  mainLayout->addLayout(centerLayout);
  mainLayout->addLayout(buttonLayout);

  QLabel * qls    = new QLabel("Session");
  sessionComboBox = new SessionComboBox();

  animationCheckBox = new QCheckBox(tr("Time follows map"));
  animationCheckBox->setChecked(true);

  titleLayout->addWidget(qls);
  titleLayout->addWidget(sessionComboBox);
  titleLayout->addStretch(1);
  titleLayout->addWidget(animationCheckBox);

  QSplitter *split = new QSplitter(Qt::Vertical,this);
  // stretch table only
  split->setStretchFactor(0,2);
  split->setStretchFactor(1,1);
  centerLayout->addWidget(split);

  // Table
  table = new FetObjectTableView(split);
  if(table){
    table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    //table->verticalHeader()->setStyle(new QWindowsStyle); // necessary if using QColorGradient on header
    table->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    table->setSelectionMode( QAbstractItemView::ContiguousSelection); // QAbstractItemView::SingleSelection
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
  }

  QSplitter *h_split = new QSplitter(Qt::Horizontal,split);
  chatWidget = new ProfetChatWidget(h_split);

  // Object Panel
  QWidget     *objectWidget      = new QWidget(h_split);
  QVBoxLayout *objectTitleLayout = new QVBoxLayout();

  objectTitleLayout->setMargin(0);
  objectTitleLayout->addWidget(new QLabel(tr("Objects")),0);

  QHBoxLayout * objectWidgetLayout = new QHBoxLayout();
  objectList                       = new FetObjectListView(this);
  QWidget *objectButtonWidget      = new QWidget();

  objectTitleLayout->addLayout(objectWidgetLayout,1);
  objectWidgetLayout->addWidget(objectList,1);
  objectWidgetLayout->addWidget(objectButtonWidget,0);

  QVBoxLayout *objectButtonWidgetLayout = new QVBoxLayout();

  viewObjectButton   = new QPushButton(tr("View"));
  viewObjectButton->setCheckable(true);
  autoZoomButton = new QPushButton(tr("Auto Zoom"));
  autoZoomButton->setCheckable(true);
  newObjectButton    = new QPushButton(tr("New"));
  editObjectButton   = new QPushButton(tr("Edit"));
  timesmoothButton   = new QPushButton(tr("Timesmooth"));
  deleteObjectButton = new QPushButton(tr("Delete"));

  objectButtonWidgetLayout->addWidget(viewObjectButton);
  objectButtonWidgetLayout->addWidget(autoZoomButton);
  objectButtonWidgetLayout->addWidget(newObjectButton);
  objectButtonWidgetLayout->addWidget(editObjectButton);
  objectButtonWidgetLayout->addWidget(timesmoothButton);
  objectButtonWidgetLayout->addWidget(deleteObjectButton);

  objectButtonWidget->setLayout(objectButtonWidgetLayout);
  objectWidget->setLayout(objectTitleLayout);

  // Buttons
  updateButton = new QPushButton(tr("Update"), this );
  updateButton->setDefault(false);

  reconnectButton = new QPushButton(tr("Reconnect..."),this);
  reconnectButton->setDefault(false);

  closeButton = new QPushButton(tr("Close"),  this );
  closeButton->setDefault(false);

  buttonLayout->addWidget(new QLabel("", this));
  buttonLayout->addWidget(updateButton);
  buttonLayout->addWidget(reconnectButton);
  buttonLayout->addWidget(new QLabel(""));
  buttonLayout->addWidget(closeButton);

  enableObjectButtons(true,false,true);
  connectSignals();
}

void FetObjectTableView::selectionChanged ( const QItemSelection & selected,
					    const QItemSelection & deselected ){
  QTableView::selectionChanged(selected,deselected);
  QList<QModelIndex> indices = selected.indexes();
  if ( indices.size() > 1 ){
    emit selectedMulti(indices);
  }
}


void ProfetSessionDialog::connectSignals(){
  if(table){
    connect(table,SIGNAL(clicked(const QModelIndex &)),
        this,SIGNAL(paramAndTimeChanged(const QModelIndex &)));
    connect(table,SIGNAL(selectedMulti(const QList<QModelIndex> & )),
	    this,SIGNAL(showObjectOverview(const QList<QModelIndex> & )));
  }
  connect(viewObjectButton,SIGNAL(toggled(bool)),
      this,SIGNAL(viewObjectToggled(bool)));
  connect(newObjectButton,SIGNAL(clicked()),
      this,SIGNAL(newObjectPerformed()));
  connect(editObjectButton,SIGNAL(clicked()),
      this,SIGNAL(editObjectPerformed()));
  connect(deleteObjectButton,SIGNAL(clicked()),
      this,SIGNAL(deleteObjectPerformed()));
  connect(timesmoothButton,SIGNAL(clicked()),
      this,SIGNAL(startTimesmooth()));
  connect(closeButton,SIGNAL(clicked()),
      this,SIGNAL(closePerformed()));
  connect(reconnectButton,SIGNAL(clicked()),
      this,SIGNAL(doReconnect()));
  connect(updateButton,SIGNAL(clicked()),
      this,SIGNAL(updateActionPerformed()));
  connect(objectList,SIGNAL(activated(const QModelIndex &)),
      this,SIGNAL(objectSelected(const QModelIndex &)));
  connect(chatWidget,SIGNAL(sendMessage(const QString &)),
      this,SIGNAL(sendMessage(const QString &)));
  connect(sessionComboBox,SIGNAL(activated(int)),
      this,SIGNAL(sessionSelected(int)));
}


void ProfetSessionDialog::customEvent(QEvent * e){
  if(e->type() == Profet::MESSAGE_EVENT){
    Profet::MessageEvent * me = (Profet::MessageEvent*) e;
    if(me->message.type == Profet::InstantMessage::FORCED_DISCONNECT_MESSAGE){
      emit forcedClosePerformed(true);
      QMessageBox::critical(this,"Disconnected", me->message.message.c_str());
    } else if(me->message.type == Profet::InstantMessage::WARNING_MESSAGE){
      QString qs = me->message.message.cStr();
      QString title = me->message.sender.cStr();
      QMessageBox::warning(0, title ,qs,
          QMessageBox::Ok,  QMessageBox::NoButton);
    }else {
      showMessage(me->message);
    }
  }
}

void ProfetSessionDialog::hideViewObjectDialog(){
  cerr << "ProfetSessionDialog::hideViewObjectDialog" << endl;
  viewObjectButton->setChecked(false);
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
  emit paramAndTimeChanged(table->model()->index(0,0));
}

void ProfetSessionDialog::selectParameterAndTime(const QModelIndex & index){
  emit paramAndTimeChanged(index);
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

void ProfetSessionDialog::enableObjectButtons(bool enableNewButton,
					      bool enableModifyButtons,
					      bool enableTable)
{
  newObjectButton->setEnabled(enableNewButton);

  editObjectButton->setEnabled(enableModifyButtons);
  deleteObjectButton->setEnabled(enableModifyButtons);
  timesmoothButton->setEnabled(enableModifyButtons);

  if(table){
    table->setEnabled(enableTable);
  }
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



