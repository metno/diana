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
#ifndef _obsWidget_h
#define _obsWidget_h

#include <qcolor.h>
#include <qdialog.h>
#include <QLabel>
#include "diCommonTypes.h"
#include "diController.h"
#include "qtToggleButton.h"
#include <map>

class QSlider;
class QLabel;
class QComboBox;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QCheckBox;
class QLCDNumber;
class QRadioButton;
class QButtonGroup;

class ButtonLayout;

class QColor;

/**
  \brief Observation settings for one plot style

   Widget for selection of data types, parameters etc.
*/
class ObsWidget : public QWidget
{
  Q_OBJECT
public:

  ObsWidget(QWidget* parent );
  ///init dialog
  void setDialogInfo(Controller* ctrl, ObsDialogInfo,int index);
  ///initialized?
  bool initialized() {return initOK;}
  ///return command strings
  std::string getOKString(bool forLog= false);
  ///insert command strings
  void putOKString(const std::string& str);
  ///return short name of current commonad
  std::string getShortname();
  void readLog(const std::string& str);
  void setFalse();
  void setDatatype(const std::string&);
  std::vector<std::string> getDataTypes();
  bool moreToggled(){return moreButton->isChecked();}
  //Criteria
  ObsDialogInfo::CriteriaList getSavedCriteria(){return savedCriteria;}
  ObsDialogInfo::CriteriaList getCriteriaList();
  bool setCurrentCriteria(int i);
  int getCurrentCriteria(){return currentCriteria;}
  std::vector<std::string> getCriteriaNames();
  void saveCriteria(const std::vector<std::string>& criteria);
  bool saveCriteria(const std::vector<std::string>& criteria,
		    const std::string& name);
  bool getCriteriaLimits(const std::string& name, int& low, int& high);
  void markButton(const std::string& name, bool on);
  bool noButton(){return nobutton;}

private slots:
  void priSelected( int index);
  void outTopClicked( int id );
  void inTopClicked( int id );
  void displayDensity( int number );
  void displaySize( int number );
  void displayDiff( int number );
  void diffComboBoxSlot( int number);
  void devFieldChecked(bool on);
  void criteriaChecked(bool on );
  void onlyposChecked(bool on );
  void extensionSlot(bool on);
  void rightClickedSlot(std::string str);

signals:
  void getTimes();
  void rightClicked(std::string);
  void setRightClicked(std::string,bool);
  void extensionToggled(bool);
  void criteriaOn();
  void newCriteriaList(ObsDialogInfo::CriteriaList);

private:
  void ToolTip();

  bool initOK;

  std::vector<Colour::ColourInfo> cInfo;
  std::vector<std::string> markerName;

  ButtonLayout* datatypeButtons;
  ButtonLayout* parameterButtons;

  QPushButton* allButton;
  QPushButton* noneButton;
  QPushButton* defButton;
  QCheckBox* tempPrecisionCheckBox;
  QCheckBox* unit_msCheckBox;
  QCheckBox* parameterNameCheckBox;
  QCheckBox* moreTimesCheckBox;
  QCheckBox* qualityCheckBox;
  QCheckBox* wmoCheckBox;
  QCheckBox* devFieldCheckBox;
  QComboBox* devColourBox1;
  QComboBox* devColourBox2;
  QCheckBox* allAirepsLevelsCheckBox;
  QCheckBox* orientCheckBox;
  QCheckBox* alignmentCheckBox;
  QCheckBox* showposCheckBox;
  QCheckBox* onlyposCheckBox;
  QCheckBox* popupWindowCheckBox;
  QComboBox* markerBox;

  QComboBox* pressureComboBox;
  QComboBox* leveldiffComboBox;

  QLCDNumber* densityLcdnum;
  QLCDNumber* sizeLcdnum;
  QLCDNumber* diffLcdnum;
  QSlider* densitySlider;
  QSlider* sizeSlider;
  QSlider* diffSlider;
  QComboBox* diffComboBox;

  QComboBox* colourBox;
  QComboBox* pribox;    //List of priority files
  QCheckBox* pricheckbox;

  QCheckBox* criteriaCheckBox;
  ToggleButton* moreButton;

  QComboBox* sortBox;
  QButtonGroup* sortRadiogroup;
  QRadioButton* ascsortButton;
  QRadioButton* descsortButton;

  std::vector<ObsDialogInfo::PriorityList> priorityList;
  std::vector<ObsDialogInfo::CriteriaList> criteriaList;
  ObsDialogInfo::CriteriaList savedCriteria;
  int currentCriteria;

  int nr_dataTypes;
  std::vector<ObsDialogInfo::Button> button;
  std::vector<ObsDialogInfo::Button> dataTypeButton;
  std::vector<ObsDialogInfo::DataType> datatype;
  std::vector<int> time_slider2lcd;

  struct dialogVariables {
    std::string plotType;
    std::vector<std::string> data;
    std::vector<std::string> parameter;
    std::map<std::string,std::string> misc;
  };

  dialogVariables dVariables;

  void decodeString(const std::string&, dialogVariables&,bool fromLog=false);
  void updateDialog(bool setOn);
  std::string makeString(bool forLog=false);

  bool pressureLevels;
  std::map<std::string,int> levelMap;
  bool leveldiffs;
  std::map<std::string,int> leveldiffMap;

  // LCD <-> slider
  float scaledensity;
  float scalesize;
  float scalediff;
  int mindensity;
  int maxdensity;

  bool markerboxVisible;
  bool allObs;
  int selectedPressure;
  std::string timediff_minutes;
  int pri_selected;     // Wich priority file
  std::string plotType;
  std::string shortname;
  bool nobutton;

  QVBoxLayout *vlayout;
  QVBoxLayout *vcommonlayout;
  QHBoxLayout *parameterlayout;
  QHBoxLayout *datatypelayout;
};

#endif
