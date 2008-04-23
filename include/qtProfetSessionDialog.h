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

#ifndef QTPROFETSESSIONDIALOG_H_
#define QTPROFETSESSIONDIALOG_H_

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QCloseEvent>
#include <QPushButton>
#include <QListView>
#include <QTableView>

#include <profet/ProfetCommon.h>
#include <profet/fetModel.h>
#include <profet/fetSession.h>
#include <profet/fetParameter.h>
#include <profet/fetObject.h>
#include "qtProfetChatWidget.h"
#include "qtProfetSessionTable.h"

#include <vector>

class FetObjectListView;
class FetObjectTableView;

class ProfetSessionDialog: public QDialog{
  Q_OBJECT
  
private:
  QComboBox * sessionComboBox;
  QPushButton * updateButton;
  QPushButton * closeButton;
  FetObjectListView * objectList;
  QPushButton * newObjectButton;
  QPushButton * editObjectButton;
  QPushButton * deleteObjectButton;
  ProfetChatWidget * chatWidget;
  FetObjectTableView * table;
  
  miString currentObject;
  
  /// Connecting all signals and slots
  void connectSignals();
  /// Enables / disables gui components
  void lockedObjectSelected(bool locked);
//  bool setSelectedObject(const miString &);
  
protected:
  void closeEvent( QCloseEvent* );
  
public:
  ProfetSessionDialog(QWidget* parent);
  
  void setModel(const fetModel & model);
  void setSessionModel(QAbstractItemModel * sessionModel);
  void setUserModel(QAbstractItemModel * userModel);
  void setObjectModel(QAbstractItemModel * objectModel);
  void setTableModel(QAbstractItemModel * tableModel);
  void setSelectedObject(const QModelIndex & index);
  void setEditable(bool editable);

  void selectDefault();
  int getCurrentSessionIndex(){return sessionComboBox->currentIndex();}
  QModelIndex getCurrentObjectIndex();
  void setCurrentSession(const QModelIndex & index);

  void showMessage(const Profet::InstantMessage & msg);
  
public slots:
  void printSize(const QModelIndex &);
  
signals:
  void sessionSelected(int);
  void sendMessage(const QString &);
  void paramAndTimeChanged(const QModelIndex &);
  void objectSelected(const QModelIndex & index);
  void newObjectPerformed();
  void editObjectPerformed();
  void deleteObjectPerformed();
  void closePerformed();
};

class FetObjectListView: public QListView{
public:
  FetObjectListView(QWidget * parent) : QListView(parent){}
  void currentChanged ( const QModelIndex & current, 
      const QModelIndex & previous ){ emit activated(current); }
};

class FetObjectTableView: public QTableView{
  
public:
  FetObjectTableView(QWidget * parent) : QTableView(parent){}
  void currentChanged ( const QModelIndex & current, 
      const QModelIndex & previous ){
    if(current.isValid())
      emit activated(current); 
    }
};

class QTitleLabel: public QLabel{
public:
  QTitleLabel(const QString & text, QWidget * parent)
    : QLabel(text,parent){
    QFont f = font();
    f.setBold(true);
    setFont(f);
    setAlignment(Qt::AlignRight);
  }
};
class QDataLabel: public QLabel{
public:
  QDataLabel(const QString & text, QWidget * parent)
    : QLabel(text,parent){
    setAlignment(Qt::AlignLeft);
  }
};
#endif /*QTPROFETSESSIONDIALOG_H_*/
