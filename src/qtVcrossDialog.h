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
#ifndef _vcrossdialog_h
#define _vcrossdialog_h

#include <QDialog>

#include <vector>

#include <puTools/miString.h>
#include <diCommonTypes.h>
#include <diCommandParser.h>
#include <diField/diColourShading.h>
#include <diField/diPattern.h>

using namespace std;

class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QSpinBox;
class QCheckBox;

class ToggleButton;
class VcrossManager;

/**

  \brief Dialogue for Vertical Crossection plotting

  Select model, field and plotting options.
  Can alter many, but not all, options from the setup file.
  Keeps user settings in the diana log file between sessions.
*/
class VcrossDialog: public QDialog
{
  Q_OBJECT

public:
  VcrossDialog( QWidget* parent, VcrossManager* vm);

  vector<miString> getOKString();
  void putOKString(const vector<miString>& vstr,
		   bool vcrossPrefix= true, bool checkOptions= true);

  vector<miString> writeLog();
  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion, const miString& logVersion);
  void cleanup();

protected:
  void closeEvent( QCloseEvent* );

private:

  struct SelectedField {
    miString model;
    miString field;
    miString fieldOpts;
    int      hourOffset;
  };

  void disableFieldOptions();
  void enableFieldOptions();
  void updateFieldOptions(const miString& name,
			  const miString& value, int valueIndex= 0);
  vector<miString> numberList( QComboBox* cBox, float number );
  miString baseList( QComboBox* cBox, float base, float ekv,
		     bool onoff=false );

  void showHistory(int step);
  miString checkFieldOptions(const miString& str, bool fieldPrefix);

  void highlightButton(QPushButton* button, bool on);

  //** PRIVATE SLOTS ********************
private slots:

  void modelboxClicked( QListWidgetItem * item );
  void fieldboxChanged(QListWidgetItem* item);
  void selectedFieldboxClicked( QListWidgetItem * item );

  void upField();
  void downField();
  void deleteSelected();
  void deleteAllSelected();
  void copySelectedField();
  void resetOptions();
  void changeModel();
  void historyBack();
  void historyForward();
  void historyOk();
  void colorCboxActivated( int index );
  void lineWidthCboxActivated( int index );
  void lineTypeCboxActivated( int index );
  void lineintervalCboxActivated( int index );
  void densityCboxActivated( int index );
  void vectorunitCboxActivated( int index );
  //void extremeTypeActivated(int index);

  void applyClicked();
  void applyhideClicked();
  void hideClicked();
  void helpClicked();

  void advancedToggled(bool on);

signals:
  void VcrossDialogApply(bool modelChange);
  void VcrossDialogHide();
  void showsource(const miString, const miString="");
  void emitVcrossTimes( vector<miTime> );

private:
  void toolTips();

  VcrossManager* vcrossm;

  bool m_advanced;

  vector<miString> models;  // all models
  vector<miString> fields;  // for current selected model

  CommandParser *cp;
  vector<ParsedCommand> vpcopt;

  // map<fieldname,fieldOpts>
  map<miString,miString> setupFieldOptions;
  map<miString,miString> fieldOptions;
  map<miString,bool> changedOptions;

  vector<SelectedField> selectedFields;

  vector<int> countSelected;

  vector<Colour::ColourInfo> colourInfo;
  vector<ColourShading::ColourShadingInfo> csInfo;
  vector<miString> twoColourNames;
  vector<miString> threeColourNames;
  vector<Pattern::PatternInfo> patternInfo;

  vector<miString> linetypes;
  vector<miString> lineintervals;
  QStringList      densityStringList;
  vector<miString> vectorunit;
  //vector<miString> extremeType;

  miString currentFieldOpts;

  vector< vector<miString> > commandHistory;

  QListWidget*  modelbox;
  QListWidget*  fieldbox;
  QListWidget*  selectedFieldbox;

  QPushButton*  upFieldButton;
  QPushButton*  downFieldButton;
  QPushButton*  Delete;
  QPushButton*  deleteAll;
  QPushButton*  copyField;
  QPushButton*  resetOptionsButton;
  QPushButton*  changeModelButton;

  QPushButton*  historyBackButton;
  QPushButton*  historyForwardButton;
  QPushButton*  historyOkButton;
  int           historyPos;

  QLabel*    colorlabel;
  QComboBox* colorCbox;
  QPixmap**  pmapColor;
  QPixmap**  pmapTwoColors;
  QPixmap**  pmapThreeColors;
  int        nr_colors;

  QLabel*    linewidthlabel;
  QComboBox* lineWidthCbox;
  int        nr_linewidths;

  QLabel*    linetypelabel;
  QComboBox* lineTypeCbox;
  int        nr_linetypes;

  QLabel*    lineintervallabel;
  QComboBox* lineintervalCbox;

  QLabel*    densitylabel;
  QComboBox* densityCbox;
  const char** cdensities;
  int        nr_densities;

  //QComboBox* extremeTypeCbox;

  QLabel*    vectorunitlabel;
  QComboBox* vectorunitCbox;

  QPushButton* fieldapply;
  QPushButton* fieldapplyhide;
  QPushButton* fieldhide;
  QPushButton* fieldhelp;

  ToggleButton* advanced;
  QPushButton* allTimeStepButton;

  void CreateAdvanced();



private slots:
  //void extremeSizeChanged(int value);
  //void extremeRadiusChanged(int value);
  void lineSmoothChanged(int value);
  void labelSizeChanged(int value);
  void hourOffsetChanged(int value);
  //void undefMaskingActivated(int index);
  //void undefColourActivated(int index);
  //void undefLinewidthActivated(int index);
  //void undefLinetypeActivated(int index);
  void zeroLineCheckBoxToggled(bool on);
  void valueLabelCheckBoxToggled(bool on);
  void tableCheckBoxToggled(bool on);
  void repeatCheckBoxToggled(bool on);
  void shadingChanged();
  void patternComboBoxToggled(int index);
  void patternColourBoxToggled(int index);
  void alphaChanged(int index);
  void zero1ComboBoxToggled(int index);
  void min1ComboBoxToggled(int index);
  void max1ComboBoxToggled(int index);
  void updatePaletteString();

private:
  QWidget* advFrame;
  //QSpinBox*  extremeSizeSpinBox;
  //QSpinBox*  extremeRadiusSpinBox;
  QSpinBox*  lineSmoothSpinBox;
  QSpinBox*  labelSizeSpinBox;
  QSpinBox*  hourOffsetSpinBox;
  //QComboBox* undefMaskingCbox;
  //QComboBox* undefColourCbox;
  //QComboBox* undefLinewidthCbox;
  //QComboBox* undefLinetypeCbox;
  QCheckBox* zeroLineCheckBox;
  QCheckBox* valueLabelCheckBox;
  QCheckBox* tableCheckBox;
  QCheckBox* repeatCheckBox;
  QComboBox* shadingComboBox;
  QComboBox* shadingcoldComboBox;
  QSpinBox*  shadingSpinBox;
  QSpinBox*  shadingcoldSpinBox;
  QComboBox* patternComboBox;
  QComboBox* patternColourBox;
  QSpinBox*  alphaSpinBox;
  QComboBox* zero1ComboBox;
  QComboBox* min1ComboBox;
  QComboBox* max1ComboBox;
  QComboBox* interval2ComboBox;
  QComboBox* linewidth1ComboBox;
  QComboBox* linetype1ComboBox;
  QComboBox* type1ComboBox;

  vector<miString> baseopts;
  vector<miString> undefMasking;

};

#endif
