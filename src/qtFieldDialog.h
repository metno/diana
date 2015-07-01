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
#ifndef _fielddialog_h
#define _fielddialog_h

#include <diCommandParser.h>
#include <diColourShading.h>
#include <diPattern.h>

#include <puTools/miTime.h>
#include <diField/diCommonFieldTypes.h>

#include <QDialog>

#include <vector>
#include <map>
#include <set>

class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSlider;
class QSpinBox;
class QCheckBox;
class QRadioButton;
class QLineEdit;

class ToggleButton;
class Controller;
class FieldColourDialog;

/**
  \brief Dialogue for field plotting

  Select model, field, level and plotting options.
  Can alter many, but not all, options from the setup file.
  The dialog displays all quckmenu commands for easy adjustment.
  Keeps user settings in the diana log file between sessions.
*/
class FieldDialog: public QDialog
{
  Q_OBJECT

private:

  struct SelectedField {
    bool inEdit;
    bool external;     // from QuickMenu,...
    int indexMGR; //index model group
    int indexM;   //index model
    int indexRefTime; //index reference time
    std::string modelName;
    std::string fieldName;
    std::string level;
    std::string idnum;
    int  hourOffset;
    int  hourDiff;
    std::string fieldOpts;
    std::vector<std::string> levelOptions;
    std::vector<std::string> idnumOptions;
    bool minus;
    std::string time;
    //Used in gridio
    std::string refTime;
    std::string zaxis;
    std::string extraaxis;
    std::string taxis;
    std::string grid;
    std::string unit;
    bool plotDefinition;
    bool levelmove;
    bool idnummove;
    SelectedField() : inEdit(false), external(false),
        hourOffset(0), hourDiff(0), minus(false),
        plotDefinition(true), levelmove(true), idnummove(true)
    {
    }
  };

  struct EnableWidget {
    bool contourWidgets;
    bool extremeWidgets;
    bool shadingWidgets;
    bool lineWidgets;
    bool fontWidgets;
    bool densityWidgets;
    bool unitWidgets;
  };

public:

  FieldDialog( QWidget* parent, Controller* lctrl);

  /// follows levelUp/levelDown in main window toolbar
  std::vector<std::string> changeLevel(int increment, int type = 0);

  void archiveMode(bool on);
  /// returns fiels command strings, one for each field
  std::vector<std::string> getOKString(bool resetLevelMove=true);
  /// return a short text for quickmenue
  std::string getShortname();
  bool levelsExists(bool up, int type=0);
  void putOKString(const std::vector<std::string>& vstr,
		   bool checkOptions=true, bool external=true);
  bool decodeString(const std::string& fieldstr, SelectedField& sf, bool& allTimeSteps);

  /// insert editoption values of <field,option> specified
  void getEditPlotOptions(std::map< std::string, std::map<std::string,std::string> >& po);
  /// make contents for the diana log file
  std::vector<std::string> writeLog();
  /// digest contents from the diana log file (a previous session)
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);

protected:
  void closeEvent( QCloseEvent* );

public slots:
  void advancedToggled(bool on);
  void fieldEditUpdate(std::string str);
  void addField(std::string str);
  void updateModels();

private:

  void updateModelBoxes();
  void setDefaultFieldOptions();
  void enableWidgets(std::string plottype);
  void enableFieldOptions();
  void enableType2Options(bool);
  void updateFieldOptions(const std::string& name,
			  const std::string& value, int valueIndex= 0);
  void updateTime();
  void setLevel();
  void setIdnum();
  void getFieldGroups(const std::string& model, const std::string& refTime, int& indexMGR, int& indexM,
		      bool plotDefinitions, std::vector<FieldGroupInfo>& vfg);
  std::string checkFieldOptions(const std::string& str);
  std::string getFieldOptions(const std::string& fieldName, bool reset, bool edit=false) const;

  bool fieldDifference(const std::string& str,
		       std::string& field1, std::string& field2) const;

  void toolTips();

  std::vector<std::string> numberList( QComboBox* cBox, float number, bool onoff= false );

  void baseList( QComboBox* cBox, float base, bool onoff= false );

  std::string getParamString(int i);

  Controller* m_ctrl;

  bool useArchive;

  bool levelInMotion;
  bool idnumInMotion;

  std::string lastFieldGroupName;

  CommandParser *cp;
  std::vector<ParsedCommand> vpcopt;

  std::string editName;  // replacing the modelName during editing

  std::map<std::string,std::string> fgTranslations;

  // map<fieldName,fieldOptions>
  std::map<std::string,std::string> setupFieldOptions;
  std::map<std::string,std::string> fieldOptions;
  std::map<std::string,std::string> editFieldOptions;

  // possible extensions of fieldnames (not found in setup)
  std::set<std::string> fieldPrefixes;
  std::set<std::string> fieldSuffixes;

  std::vector<SelectedField> selectedFields;
  int numEditFields;
  std::vector<SelectedField> selectedField2edit;
  std::vector<bool>          selectedField2edit_exists;

  std::vector<int> countSelected;

  std::vector< std::vector<std::string> > plottypes_dim;
  std::vector<std::string> plottypes;

  std::map<std::string, EnableWidget> enableMap;
  std::vector<ColourShading::ColourShadingInfo> csInfo;
  std::vector<Pattern::PatternInfo> patternInfo;

  std::vector<std::string> linetypes;
  std::vector<std::string> lineintervals;
  std::vector<std::string> lineintervals2;
  QStringList      densityStringList;
  std::vector<std::string> vectorunit;
  std::vector<std::string> extremeType;
  std::string currentFieldOpts;
  bool     currentFieldOptsInEdit;

  // info about selected model, fields, levels, idnums and plot options
  std::vector<FieldGroupInfo> vfgi;

  std::vector<FieldDialogInfo> m_modelgroup;
  std::vector<int>             indexMGRtable;

  std::string lastLevel;
  std::string lastIdnum;
  std::vector<std::string> currentLevels;
  std::vector<std::string> currentIdnums;

  QColor* color;

  QComboBox* modelGRbox;
  QListWidget*  modelbox;
  QComboBox* refTimeComboBox;
  QComboBox* fieldGRbox;
  QCheckBox* fieldGroupCheckBox;
  QListWidget*  fieldbox;
  QListWidget*  selectedFieldbox;

  QSlider* levelSlider;
  QLabel*  levelLabel;
  QSlider* idnumSlider;
  QLabel*  idnumLabel;

  QPushButton*  upFieldButton;
  ToggleButton* minusButton;
  QPushButton*  downFieldButton;
  QPushButton*  deleteButton;
  QPushButton*  deleteAll;
  QPushButton*  copyField;
  QPushButton*  resetOptionsButton;
  QPushButton*  changeModelButton;

  QLineEdit* unitLineEdit;
  QComboBox* plottypeComboBox;
  QComboBox* colorCbox;

  QComboBox* lineWidthCbox;
  QComboBox* lineTypeCbox;

  QComboBox* lineintervalCbox;

  QComboBox* densityCbox;
  const char** cdensities;
  int        nr_densities;

  QComboBox* extremeTypeCbox;

  QComboBox* vectorunitCbox;

  QPushButton* fieldapply;
  QPushButton* fieldapplyhide;
  QPushButton* fieldhide;
  QPushButton* fieldhelp;

  ToggleButton* advanced;
  ToggleButton* allTimeStepButton;

  void CreateAdvanced();

signals:
  void FieldApply();
  void FieldHide();
  void showsource(const std::string, const std::string="");
  void emitTimes(const std::string&, const std::vector<miutil::miTime>& );
  void fieldPlotOptionsChanged(std::map<std::string,std::string>&);

private slots:
  void modelGRboxActivated( int index );
  void modelboxClicked( QListWidgetItem * item );

  void updateFieldGroups();
  void fieldGRboxActivated( int index );
  void fieldboxChanged(QListWidgetItem*);

  void selectedFieldboxClicked( QListWidgetItem * item );

  void upField();
  void downField();
  void minusField( bool on );
  void deleteSelected();
  void deleteAllSelected();
  void copySelectedField();
  void resetOptions();
  void changeModel();
  void unitEditingFinished();
  void plottypeComboBoxActivated( int index );
  void colorCboxActivated( int index );
  void lineWidthCboxActivated( int index );
  void lineTypeCboxActivated( int index );
  void lineintervalCboxActivated( int index );
  void densityCboxActivated( int index );
  void vectorunitCboxActivated( int index );
  void extremeTypeActivated(int index);

  void levelChanged( int number );
  void updateLevel();
  void levelPressed();

  void idnumChanged( int number );
  void updateIdnum();
  void idnumPressed();

  void applyClicked();
  void applyhideClicked();
  void hideClicked();
  void helpClicked();

  void allTimeStepToggled( bool on );

  void extremeSizeChanged(int value);
  void extremeRadiusChanged(int value);
  void lineSmoothChanged(int value);
  void fieldSmoothChanged(int value);
  void labelSizeChanged(int value);
  void valuePrecisionBoxActivated( int index );
  void gridValueCheckBoxToggled(bool on);
  void gridLinesChanged(int value);
  //  void gridLinesMaxChanged(int value);
  void hourOffsetChanged(int value);
  void hourDiffChanged(int value);
  void undefMaskingActivated(int index);
  void undefColourActivated(int index);
  void undefLinewidthActivated(int index);
  void undefLinetypeActivated(int index);
  void frameCheckBoxToggled(bool on);
  void zeroLineCheckBoxToggled(bool on);
  void valueLabelCheckBoxToggled(bool on);
  void colour2ComboBoxToggled(int index);
  void tableCheckBoxToggled(bool on);
  void repeatCheckBoxToggled(bool on);
  void shadingChanged();
  void threeColoursChanged();
  void patternComboBoxToggled(int index);
  void patternColourBoxToggled(int index);
  void alphaChanged(int index);
  void interval2ComboBoxToggled(int index);
  void zero1ComboBoxToggled(int index);
  void zero2ComboBoxToggled(int index);
  void min1ComboBoxToggled(int index);
  void max1ComboBoxToggled(int index);
  void min2ComboBoxToggled(int index);
  void max2ComboBoxToggled(int index);
  void linevaluesFieldEdited();
  void linevaluesLogCheckBoxToggled(bool);
  void linewidth1ComboBoxToggled(int index);
  void linewidth2ComboBoxToggled(int index);
  void linetype1ComboBoxToggled(int index);
  void linetype2ComboBoxToggled(int index);
  void updatePaletteString();

private:

  QWidget*   advFrame;
  QSpinBox*  extremeSizeSpinBox;
  QSpinBox*  extremeRadiusSpinBox;
  QSpinBox*  lineSmoothSpinBox;
  QSpinBox*  fieldSmoothSpinBox;
  QSpinBox*  labelSizeSpinBox;
  QComboBox* valuePrecisionBox;
  QCheckBox* gridValueCheckBox;
  QSpinBox*  gridLinesSpinBox;
  QSpinBox*  hourOffsetSpinBox;
  QSpinBox*  hourDiffSpinBox;
  QComboBox* undefMaskingCbox;
  QComboBox* undefColourCbox;
  QComboBox* undefLinewidthCbox;
  QComboBox* undefLinetypeCbox;
  QCheckBox* frameCheckBox;
  QCheckBox* zeroLineCheckBox;
  QCheckBox* valueLabelCheckBox;
  QCheckBox* tableCheckBox;
  QCheckBox* repeatCheckBox;
  std::vector<QComboBox*> threeColourBox;
  QComboBox* shadingComboBox;
  QComboBox* shadingcoldComboBox;
  QSpinBox*  shadingSpinBox;
  QSpinBox*  shadingcoldSpinBox;
  QComboBox* patternComboBox;
  QComboBox* patternColourBox;
  QSpinBox*  alphaSpinBox;
  QComboBox* colour2ComboBox;
  QComboBox* zero1ComboBox;
  QComboBox* min1ComboBox;
  QComboBox* max1ComboBox;
  QLineEdit* linevaluesField;
  QCheckBox* linevaluesLogCheckBox;
  QComboBox* interval2ComboBox;
  QComboBox* zero2ComboBox;
  QComboBox* min2ComboBox;
  QComboBox* max2ComboBox;
  QComboBox* linewidth1ComboBox;
  QComboBox* linewidth2ComboBox;
  QComboBox* linetype1ComboBox;
  QComboBox* linetype2ComboBox;
  FieldColourDialog* colourLineDialog;

  std::vector<std::string> undefMasking;
};

#endif
