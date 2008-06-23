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

#include <qdialog.h>
#include <qfont.h>
#include <qpalette.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QPixmap>
#include <QFrame>
#include <QVBoxLayout>

#include <vector>
#include <map>
#include <set>

#include <miString.h>
#include <miTime.h>
#include <diCommonFieldTypes.h>
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

public:

  FieldDialog( QWidget* parent, Controller* lctrl);

  /// follows levelUp/levelDown in main window toolbar
  void changeLevel(const miString& level);
  /// follows idnumUp/idnumDown (EPS clusters etc) in mainwindow toolbar
  void changeIdnum(const miString& idnum);
  /// switch on/off access to archives
  void archiveMode(bool on);
  /// switch on/off access to profet fields
  void enableProfet(bool on);
  /// returns fiels command strings, one for each field
  vector<miString> getOKString();
  /// return a short text for quickmenue
  miString getShortname();
  /// return current changable level
  void getOKlevels(vector<miString>& levelList, miString& levelSpec);
  /// return current changable idnum (type/class/cluster/member/...)
  void getOKidnums(vector<miString>& idnumList, miString& idnumSpec);
  /// sets the dialogue according to (quickmenu) command strings
  void putOKString(const vector<miString>& vstr,
		   bool checkOptions=true, bool external=true);
  /// returns checked command string to quickmenu
  void requestQuickUpdate(const vector<miString>& oldstr,
                                vector<miString>& newstr);

  /// make contents for the diana log file
  vector<miString> writeLog();
  /// digest contents from the diana log file (a previous session)
  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion, const miString& logVersion);
  bool close(bool alsoDelete);

public slots:
  void advancedToggled(bool on);
  void fieldEditUpdate(miString str);
  void addField(miString str);
  void updateModels();

private:

  struct SelectedField {
    bool inEdit;
    bool external;     // from QuickMenu,...
    bool forecastSpec; // yet only if external
    bool editPlot; //true: old field edit/false: profet
    int  indexMGR;
    int  indexM;
    miString modelName;
    miString fieldName;
    miString level;
    miString idnum;
    int  hourOffset;
    int  hourDiff;
    miString fieldOpts;
    vector<miString> levelOptions;
    vector<miString> idnumOptions;
    bool minus;
    miString time;
  };

  void updateModelBoxes();
  void disableFieldOptions(int type=0);
  void enableFieldOptions();
  void enableType2Options(bool);
  void updateFieldOptions(const miString& name,
			  const miString& value, int valueIndex= 0);
  void updateTime();
  void setLevel();
  void setIdnum();
  void getFieldGroups(const miString& model, int& indexMGR, int& indexM,
		      vector<FieldGroupInfo>& vfg);
  void showHistory(int step);
  miString checkFieldOptions(const miString& str);
  miString getFieldOptions(const miString& fieldName, bool reset) const;

  bool fieldDifference(const miString& str,
		       miString& field1, miString& field2) const;

  void highlightButton(QPushButton* button, bool on);

  void toolTips();

  static vector<miString> numberList( QComboBox* cBox, float number );

  miString baseList( QComboBox* cBox, float base, float ekv, bool onoff= false );

  Controller* m_ctrl;

  bool useArchive;
  bool profetEnabled;

  bool levelInMotion;
  bool idnumInMotion;

  miString lastFieldGroupName;

  CommandParser *cp;
  vector<ParsedCommand> vpcopt;

  miString editName;  // replacing the modelName during editing

  map<miString,miString> fgTranslations;

  // map<fieldName,fieldOptions>
  map<miString,miString> setupFieldOptions;
  map<miString,miString> fieldOptions;

  // possible extensions of fieldnames (not found in setup)
  set<miString> fieldPrefixes;
  set<miString> fieldSuffixes;

  vector<SelectedField> selectedFields;
  int numEditFields;
  vector<SelectedField> selectedField2edit;
  vector<bool>          selectedField2edit_exists;

  miString levelOKspec;
  vector<miString> levelOKlist;

  miString idnumOKspec;
  vector<miString> idnumOKlist;

  vector<int> countSelected;

  vector<Colour::ColourInfo> colourInfo;
  vector<ColourShading::ColourShadingInfo> csInfo;
  vector<Pattern::PatternInfo> patternInfo;

  vector<miString> linetypes;
  vector<miString> lineintervals;
  QStringList      densityStringList;
  vector<miString> vectorunit;
  vector<miString> extremeType;
  miString currentFieldOpts;
  bool     currentFieldOptsInEdit;

  // info about selected model, fields, levels, idnums and plot options
  vector<FieldGroupInfo> vfgi;

  vector<vector<miString> > commandHistory;

  vector<FieldDialogInfo> m_modelgroup;
  vector<int>             indexMGRtable;

  miString lastLevel;
  miString lastIdnum;
  vector<miString> currentLevels;
  vector<miString> currentIdnums;

  QColor* color;

  QComboBox* modelGRbox;
  QListWidget*  modelbox;
  QComboBox* fieldGRbox;
  QListWidget*  fieldbox;
  QListWidget*  selectedFieldbox;

  QSlider* levelSlider;
  QLabel*  levelLabel;
  QSlider* idnumSlider;
  QLabel*  idnumLabel;

  QPushButton*  upFieldButton;
  ToggleButton* minusButton;
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

  QComboBox* colorCbox;

  QComboBox* lineWidthCbox;
  int        nr_linewidths;

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

  // layout
  QVBoxLayout* v1layout;
  QHBoxLayout* v1h4layout;
  QGridLayout* optlayout;
  QVBoxLayout* levellayout;
  QHBoxLayout* idnumlayout;
  QHBoxLayout* h4layout;
  QHBoxLayout* h5layout;
  QHBoxLayout* h6layout;

  //toplayout
  QVBoxLayout* vlayout;

signals:
  void FieldApply();
  void FieldHide();
  void showsource(const miString, const miString="");
  void emitTimes( const miString& ,const vector<miTime>& );

private slots:
  void modelGRboxActivated( int index );
  void modelboxClicked( QListWidgetItem * item );

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
  void historyBack();
  void historyForward();
  void historyOk();
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
  void gridValueCheckBoxToggled(bool on);
  void gridLinesChanged(int value);
  //  void gridLinesMaxChanged(int value);
  void baseoptionsActivated( int index );
  void hourOffsetChanged(int value);
  void hourDiffChanged(int value);
  void undefMaskingActivated(int index);
  void undefColourActivated(int index);
  void undefLinewidthActivated(int index);
  void undefLinetypeActivated(int index);
  void zeroLineCheckBoxToggled(bool on);
  void valueLabelCheckBoxToggled(bool on);
  void colour1ComboBoxToggled(int index);
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
  QCheckBox* gridValueCheckBox;
  QSpinBox*  gridLinesSpinBox;
  //  QSpinBox*  gridLinesMaxSpinBox;
  QComboBox* baseoptionsCbox;
  QSpinBox*  hourOffsetSpinBox;
  QSpinBox*  hourDiffSpinBox;
  QComboBox* undefMaskingCbox;
  QComboBox* undefColourCbox;
  QComboBox* undefLinewidthCbox;
  QComboBox* undefLinetypeCbox;
  QCheckBox* zeroLineCheckBox;
  QComboBox* zeroLineColourCBox;
  QCheckBox* valueLabelCheckBox;
  QCheckBox* tableCheckBox;
  QCheckBox* repeatCheckBox;
  //  QCheckBox* threeColoursCheckBox;
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
  QComboBox* type1ComboBox;
  QComboBox* type2ComboBox;
  FieldColourDialog* colourLineDialog;

  vector<miString> undefMasking;

};

#endif
