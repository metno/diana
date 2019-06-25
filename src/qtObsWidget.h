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
#ifndef _obsWidget_h
#define _obsWidget_h

#include "diColour.h"
#include "diKVListPlotCommand.h"
#include "diObsDialogInfo.h"
#include "qtToggleButton.h"

#include <QColor>
#include <QLabel>
#include <QWidget>

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
  void setDialogInfo(const ObsDialogInfo::PlotType&);
  ///initialized?
  bool initialized() {return initOK;}
  ///return command strings
  KVListPlotCommand_cp getOKString(bool forLog= false);
  ///insert command strings
  void putOKString(const PlotCommand_cp& str);
  ///return short name of current commonad
  std::string getShortname();
  void readLog(const miutil::KeyValue_v& kvs);
  void setFalse();
  std::vector<std::string> getDataTypes();
  bool moreToggled(){return moreButton && moreButton->isChecked();}
  //Criteria
  ObsDialogInfo::CriteriaList getSavedCriteria(){return savedCriteria;}
  ObsDialogInfo::CriteriaList getCriteriaList();
  bool setCurrentCriteria(int i);
  int getCurrentCriteria(){return currentCriteria;}
  std::vector<std::string> getCriteriaNames();
  void saveCriteria(const std::vector<std::string>& criteria);
  bool saveCriteria(const std::vector<std::string>& criteria, const std::string& name);
  bool getCriteriaLimits(const std::string& name, int& low, int& high);

private Q_SLOTS:
  void priSelected( int index);
  void datatypeButtonClicked(int id);
  void displayDensity( int number );
  void displaySize( int number );
  void displayDiff( int number );
  void diffComboBoxSlot( int number);
  void devFieldChecked(bool on);
  void criteriaChecked(bool on );
  void onlyposChecked(bool on );
  void extensionSlot(bool on);
  void rightClickedSlot(std::string str);

Q_SIGNALS:
  void getTimes(bool);
  void rightClicked(const std::string&);
  void extensionToggled(bool);
  void criteriaOn();

private:
  bool initOK;

  std::vector<Colour::ColourInfo> cInfo;
  std::vector<std::string> markerName;

  ButtonLayout* datatypeButtons;
  ButtonLayout* parameterButtons;

  QCheckBox* tempPrecisionCheckBox;
  QCheckBox* unit_msCheckBox;
  QCheckBox* parameterNameCheckBox;
  QCheckBox* qualityCheckBox;
  QCheckBox* wmoCheckBox;
  QCheckBox* plotundefCheckBox;
  QCheckBox* devFieldCheckBox;
  QComboBox* devColourBox1;
  QComboBox* devColourBox2;
  QCheckBox* orientCheckBox;
  QCheckBox* alignmentCheckBox;
  QCheckBox* showposCheckBox;
  QCheckBox* onlyposCheckBox;
  QCheckBox* popupWindowCheckBox;
  QComboBox* markerBox;

  QComboBox* pressureComboBox;

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

  std::vector<ObsDialogInfo::Button> button;
  std::vector<int> time_slider2lcd;

  struct dialogVariables {
    std::string plotType;
    std::vector<std::string> data;
    std::vector<std::string> parameter;
    std::map<std::string,std::string> misc;
  };

  dialogVariables dVariables;
  void decodeString(const miutil::KeyValue_v&, dialogVariables&,bool fromLog=false);
  void updateDialog(bool setOn);
  miutil::KeyValue_v makeString();

  bool verticalLevels;
  std::map<std::string,int> levelMap;

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

  QVBoxLayout *vlayout;
  QVBoxLayout *vcommonlayout;
  QHBoxLayout *parameterlayout;
  QHBoxLayout *datatypelayout;
};

#endif
