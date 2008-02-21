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

#include <qdialog.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <profet/ProfetCommon.h>
#include <profet/fetModel.h>
#include <profet/fetSession.h>
#include <profet/fetParameter.h>
#include <profet/fetObject.h>
#include "qtProfetChatWidget.h"
#include "qtProfetSessionTable.h"

#include <vector>

class QButton;
class Q3ListBox;
class Q3HBox;
class QTabWidget;
class Q3TextEdit;
class QLineEdit;

class ProfetSessionDialog: public QDialog{
  Q_OBJECT
  
private:
  QLabel  * sessionLabel;
  QLabel  * modelLabel;
  QPushButton * updateButton;
  QPushButton * closeButton;
  Q3HBox   * objectBox;
  Q3ListBox * userList;
  Q3ListBox * objectList;
  QLabel  * objectAlgLabel;
  QLabel  * objectOwnerLabel;
  QLabel  * objectStatusLabel;
  QPushButton * newObjectButton;
  QPushButton * editObjectButton;
  QPushButton * deleteObjectButton;
  QTabWidget * tabWidget;
  Q3TextEdit * chatOutput;
  QLineEdit * chatInput;
  ProfetChatWidget * chatWidget;
  ProfetSessionTable * table;
  
  miString currentObject;
  
  /// Connecting all signals and slots
  void connectSignals();
  /// Enables / disables gui components
  void lockedObjectSelected(bool locked);
  bool setSelectedObject(const miString &);
  
protected:
  void closeEvent( QCloseEvent* );
  
public:
  ProfetSessionDialog(QWidget* parent);
  
  void setModel(const fetModel & model);
  void setUserList(const vector<Profet::PodsUser> & users);
  void setObjectList(const vector<fetObject> & objects);
  bool setCurrentObject(const fetObject &);
  void setEditable(bool editable);
  void noObjectSelected();

  miString getSelectedParameter() const;
  miTime getSelectedTime() const;
  void selectDefault();
  miString getSelectedObject();

  void initializeTable( const vector<fetParameter> & parameters,
			    const fetSession & session); 

  void setObjectSignatures( vector<fetObject::Signature> s);
  void showMessage(const Profet::InstantMessage & msg);

private slots:
  void objectListChanged(const QString &);  

signals:
  void sendMessage(const QString &);
  void paramAndTimeChanged(miString param, miTime time);
  void objectSelected(miString id);
  void newObjectPerformed();
  void editObjectPerformed();
  void deleteObjectPerformed();
  void closePerformed();
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
