/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: qtEditTimeDialog.h,v 2.16 2007/06/19 06:28:24 lisbethb Exp $

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

#ifndef QTPROFETFIELDDIALOG_H_
#define QTPROFETFIELDDIALOG_H_

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qwidgetstack.h>
#include <miSliderWidget.h>
#include <profet/fetObject.h>
#include <profet/fetBaseObject.h>
#include <map>

class ProfetFieldDialog: public QDialog {
	Q_OBJECT
public:
	/**
	 * Constructor for ProfetFieldDialog
	 * @param parent the parent widget for this dialog
	 */
	ProfetFieldDialog(QWidget* parent);
	/**
	 * @return the ID of the selected object
	 */
	miString getCurrentObjectId();
	/**
	 * @return the ID of the selected base-object
	 */
	miString getCurrentBaseObjectId();

private:
	QLabel* parameterLabel;

	QPushButton* createButton;
	QComboBox* baseObjectComboBox;
	QCheckBox* showAllBox;
	QComboBox* objectComboBox;
	QPushButton* copyButton;

	QWidgetStack* stackWidget;
	QCheckBox* heightMaskCheckBox;
	miSliderWidget* heightMaskSlider;
	QCheckBox* landMaskCheckBox;
	QPushButton* hideButton;
	QPushButton* deleteButton;

	bool newObj;
	int widgetId; //current stack
	
	map<int,miString> baseObjectIndexMap;
	map<int,miString> objectIndexMap;

	void newStack(fetBaseObject fobj);
	void enableStack(fetBaseObject& fobj);
	
private slots: // Handling GUI Actions
	void baseObjectComboBoxActivated(int index);
	void objectComboBoxActivated(int index);
	void createButtonClicked();
	void copyButtonClicked();
	void deleteButtonClicked();
	void widgetApplyPerformed();

signals: // ... from GUI Actions
	void baseObjectSelected(miString id);
	void objectSelected(miString id);
	void createObject(bool currentArea, miString baseObjectId);
	void deleteObject(miString id);
	void hideFieldDialog();
	void objectPropertiesChanged(miString id); //properties??
	
};

#endif /*QTPROFETFIELDDIALOG_H_*/
