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
#ifndef QTPROFETOBJECTDIALOG_H_
#define QTPROFETOBJECTDIALOG_H_

#include <qdialog.h>
#include <q3groupbox.h>
#include <qstring.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <q3table.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <q3textedit.h>
#include <qcombobox.h>
#include <q3widgetstack.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <profetSQL/fetSession.h>
#include <profetSQL/fetParameter.h>
#include <profet/fetObject.h>
#include <profet/fetBaseObject.h>
#include <profet/fetDynamicGui.h>

#include <vector>
#include <map>
/// map of baseComboBox index and dynamic-gui widget
typedef map<int,fetDynamicGui*> DynamicGuiMap;
/// map of baseObject name and description
typedef map<QString,QString> DescriptionMap;

class ProfetObjectDialog: public QDialog{
  Q_OBJECT
public:
  enum AreaStatus {AREA_NOT_SELECTED,AREA_OK,AREA_NOT_VALID};
private:
  QLabel        *parameterLabel;
  QLabel        *sessionLabel;
  QLabel        *algDescriptionLabel;
  QLabel        *statisticLabel;
  QComboBox     *baseComboBox;
  QRadioButton  *customAreaButton;
  QRadioButton  *objectAreaButton;
  QRadioButton  *fileAreaButton;
  QComboBox     *objectAreaComboBox;
  QLineEdit     *fileTextEdit;
  QLabel        *areaInfoLabel;
  Q3TextEdit     *reasonText;
  QPushButton       *saveObjectButton;
  QPushButton       *cancelObjectButton;
  Q3WidgetStack  *widgetStack;
  Q3GroupBox     *algGroupBox;
  Q3GroupBox     *areaGroupBox;
  Q3GroupBox     *stackGroupBox;
  Q3GroupBox     *reasonGroupBox;
  Q3GroupBox     *statGroupBox;
  DynamicGuiMap  dynamicGuiMap;
  DescriptionMap descriptionMap;
  miString       selectedBaseObject;
  
  void           connectSignals();
  QString        getAreaStatusString(AreaStatus);
  
protected:
  void closeEvent( QCloseEvent* );  
  
public:
  ProfetObjectDialog(QWidget* parent);
  
  void addDymanicGui(vector<fetDynamicGui::GuiComponent> components);
  void setSession(const miTime & session);
  void setParameter(const miString & parameter);
  void setBaseObjects(const vector<fetBaseObject> & o);
  void setAreaStatus(AreaStatus);
  void setStatistics(map<miString,float>&);
  miString getSelectedBaseObject();
  miString getReason();
  void selectDefault();
  vector<fetDynamicGui::GuiComponent> getCurrentGuiComponents();
  void newObjectMode();
  void editObjectMode(const fetObject & obj,
      vector<fetDynamicGui::GuiComponent> components);
  bool showingNewObject(){ return baseComboBox->isEnabled();}

private slots:
  void baseObjectChanged(const QString&);
  
signals:
  void baseObjectSelected(miString name);
  void dynamicGuiChanged();
  void saveObjectClicked();
  void cancelObjectDialog();
};

#endif /*QTPROFETOBJECTDIALOG_H_*/
