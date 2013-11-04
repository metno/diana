/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include <diController.h>
#include <QDialog>

class QComboBox;
class ObsWidget;
class QPushButton;
class QLabel;
class ToggleButton;
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
  std::vector<std::string> getOKString();
  ///insert command strings
  void putOKString(const std::vector<std::string>& vstr);
  ///return short name of current command
  std::string getShortname();
  ///change plottype
  bool setPlottype(const std::string& name, bool on);

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
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
  void rightButtonClicked(std::string);
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
  void emitTimes(const std::string&, const std::vector<miutil::miTime>& );
  void setCriteria( std::string, bool );

private:
  void makeExtension();
  void markButton(std::string& );
  std::string makeCriteriaString();
  bool newCriteriaString();
  void updateExtension();
  void numberList( QComboBox* cBox, float number );
  int findPlotnr(const std::string&);

 //ATTRIBUTES
  int nr_plot;
  std::vector<std::string> m_name;
  std::vector<std::string> savelog;
  int m_selected;
  QColor* colour;
  QComboBox* plotbox;
  QStackedWidget* stackedWidget;
  std::vector<ObsWidget*> obsWidget;
  std::string parameterSelected;
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
  std::string parameter;
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
  std::vector<Colour::ColourInfo> cInfo;
  std::vector<std::string> markerName;
  Controller* m_ctrl;
  bool freeze;
};

#endif
