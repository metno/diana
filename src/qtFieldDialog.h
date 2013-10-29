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

#include <QDialog>

#include <vector>
#include <map>
#include <set>

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diField/diCommonFieldTypes.h>
#include <diCommandParser.h>
#include <diColourShading.h>
#include <diPattern.h>

using namespace std;

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
    bool forecastSpec; // yet only if external
    bool editPlot; //true: old field edit/false: profet
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
    vector<std::string> levelOptions;
    vector<std::string> idnumOptions;
    bool minus;
    std::string time;
    //Used in gridio
    std::string refTime;
    std::string zaxis;
    std::string extraaxis;
    std::string taxis;
    std::string grid;
    std::string unit;
    bool cdmSyntax;
    bool plotDefinition;
    bool levelmove;
    bool idnummove;
    SelectedField() : inEdit(false), external(false), forecastSpec(false), editPlot(false),
        hourOffset(0), hourDiff(0), minus(false),
        cdmSyntax(true), plotDefinition(true), levelmove(true), idnummove(true)
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
  vector<std::string> getOKString_std(bool resetLevelMove=true);
  /// return a short text for quickmenue
  std::string getShortname();
  bool levelsExists(bool up, int type=0);
  void putOKString(const vector<std::string>& vstr,
		   bool checkOptions=true, bool external=true);
  bool decodeString_cdmSyntax(const std::string& fieldstr, SelectedField& sf, bool& allTimeSteps);
  bool decodeString_oldSyntax(const std::string& fieldstr, SelectedField& sf, bool& allTimeSteps);

  /// insert editoption values of <field,option> specified
  void getEditPlotOptions(map< miutil::miString, map<miutil::miString,miutil::miString> >& po);
  /// make contents for the diana log file
  vector<std::string> writeLog();
  /// digest contents from the diana log file (a previous session)
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);

protected:
  void closeEvent( QCloseEvent* );

public slots:
  void advancedToggled(bool on);
  void fieldEditUpdate(miutil::miString str);
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
		      bool plotDefinitions, vector<FieldGroupInfo>& vfg);
  std::string checkFieldOptions(const std::string& str, bool cdmSyntax);
  std::string getFieldOptions(const std::string& fieldName, bool reset, bool edit=false) const;

  bool fieldDifference(const std::string& str,
		       std::string& field1, std::string& field2) const;

  void toolTips();

  vector<std::string> numberList( QComboBox* cBox, float number, bool onoff= false );

  void baseList( QComboBox* cBox, float base, bool onoff= false );

  std::string getParamString(int i);

  Controller* m_ctrl;

  bool useArchive;
  bool profetEnabled;

  bool levelInMotion;
  bool idnumInMotion;

  std::string lastFieldGroupName;

  CommandParser *cp;
  vector<ParsedCommand> vpcopt;

  std::string editName;  // replacing the modelName during editing

  map<std::string,std::string> fgTranslations;

  // map<fieldName,fieldOptions>
  map<std::string,std::string> setupFieldOptions;
  map<std::string,std::string> fieldOptions;
  map<std::string,std::string> editFieldOptions;

  // possible extensions of fieldnames (not found in setup)
  set<std::string> fieldPrefixes;
  set<std::string> fieldSuffixes;

  vector<SelectedField> selectedFields;
  int numEditFields;
  vector<SelectedField> selectedField2edit;
  vector<bool>          selectedField2edit_exists;

  vector<int> countSelected;

  vector< vector<std::string> > plottypes_dim;
  vector<std::string> plottypes;

  map<std::string, EnableWidget> enableMap;
  vector<ColourShading::ColourShadingInfo> csInfo;
  vector<Pattern::PatternInfo> patternInfo;

  vector<std::string> linetypes;
  vector<std::string> lineintervals;
  vector<std::string> lineintervals2;
  QStringList      densityStringList;
  vector<std::string> vectorunit;
  vector<std::string> extremeType;
  std::string currentFieldOpts;
  bool     currentFieldOptsInEdit;

  // info about selected model, fields, levels, idnums and plot options
  vector<FieldGroupInfo> vfgi;

  vector<FieldDialogInfo> m_modelgroup;
  vector<int>             indexMGRtable;

  std::string lastLevel;
  std::string lastIdnum;
  vector<std::string> currentLevels;
  vector<std::string> currentIdnums;

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
  void emitTimes(const miutil::miString&, const std::vector<miutil::miTime>& );
  void fieldPlotOptionsChanged(map<std::string,std::string>&);

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
  vector<QComboBox*> threeColourBox;
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
  QComboBox* interval2ComboBox;
  QComboBox* zero2ComboBox;
  QComboBox* min2ComboBox;
  QComboBox* max2ComboBox;
  QComboBox* linewidth1ComboBox;
  QComboBox* linewidth2ComboBox;
  QComboBox* linetype1ComboBox;
  QComboBox* linetype2ComboBox;
  FieldColourDialog* colourLineDialog;

  vector<std::string> undefMasking;

};

#endif
