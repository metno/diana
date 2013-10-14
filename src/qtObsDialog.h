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
#ifndef _obsdialog_h
#define _obsdialog_h

#include <QDialog>

#include <diController.h>
#include <puTools/miString.h>

class QComboBox;
class ObsWidget;
class QPushButton;
class QLabel;
class ToggleButton;
class AdvancedButton;
class QListWidget;
class QListWidgetItem;
class QColor;
class QLCDNumber;
class QSlider;
class QCheckBox;
class QRadioButton;
class QButtonGroup;
class QLineEdit;
class QStackedWidget;

/**

  \brief Observation dialogue

   Dialogue for selection of plot styles, data types, parameters etc.

*/
class ObsDialog: public QDialog
{
  Q_OBJECT
public:

  ObsDialog( QWidget* parent, Controller* llctrl );
  /// update dialog after re-reading setupfile
  void updateDialog();
  ///return command strings
  vector<miutil::miString> getOKString();
  ///insert command strings
  void putOKString(const vector<miutil::miString>& vstr);
  ///return short name of current command
  miutil::miString getShortname();
  ///change plottype
  bool setPlottype(const miutil::miString& name, bool on);

  vector<miutil::miString> writeLog();
  void readLog(const vector<miutil::miString>& vstr,
               const miutil::miString& thisVersion, const miutil::miString& logVersion);
///called when the dialog is closed by the window manager
protected:
  void closeEvent( QCloseEvent* );

public slots:
  void archiveMode( bool on );
  void getTimes();

private slots:
  void plotSelected( int index , bool sendTimes=true);
  void applyhideClicked();
  void helpClicked();
  void multiplotClicked(bool);
  void extensionToggled(bool);
  void criteriaOn();
  void rightButtonClicked(miutil::miString);
  void criteriaListSelected(int);
  void criteriaSelected(QListWidgetItem* );
  void signSlot(int);
  void sliderSlot(int);
  void stepSlot(int);
  void changeCriteriaString();
  void deleteSlot();
  void deleteAllSlot();
  void saveSlot();

signals:
  void ObsApply();
  void ObsHide();
  void showsource(const std::string, const std::string="");
  void emitTimes(const miutil::miString&, const std::vector<miutil::miTime>& );
  void setCriteria( miutil::miString, bool );

private:
  void makeExtension();
  void markButton(miutil::miString& );
  miutil::miString makeCriteriaString();
  bool newCriteriaString();
  void updateExtension();
  void numberList( QComboBox* cBox, float number );
  int findPlotnr(const miutil::miString&);

 //ATTRIBUTES
  int nr_plot;
  vector<miutil::miString> m_name;
  vector<miutil::miString> savelog;
  int m_selected;
  QColor* colour;
  QComboBox* plotbox;
  QStackedWidget* stackedWidget;
  vector<ObsWidget*> obsWidget;
  miutil::miString parameterSelected;
  QLabel * label;
  bool multiplot;
  ToggleButton* multiplotButton;
  QPushButton* obsapply;
  QPushButton* obshide;
  QPushButton* obsapplyhide;
  QPushButton* obsrefresh;
  QPushButton* obshelp;

  //Extension
  QWidget* extension;
  //  QLabel* parameterLabel;
  QComboBox* criteriaBox;
  QListWidget* criteriaListbox;
  miutil::miString parameter;
  QLCDNumber* limitLcd;
  QSlider* limitSlider;
  QComboBox* stepComboBox;
  QComboBox* signBox;
  QButtonGroup* radiogroup;
  QRadioButton* plotButton;
  QRadioButton* colourButton;
  QRadioButton* totalColourButton;
  QRadioButton* markerButton;
  QComboBox* colourBox;
  QComboBox* markerBox;
  QLineEdit* lineedit;
  vector<Colour::ColourInfo> cInfo;
  vector<miutil::miString> markerName;
  Controller* m_ctrl;
  bool freeze;
};





#endif
