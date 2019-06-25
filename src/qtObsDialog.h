/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "qtDataDialog.h"

#include "diColour.h"
#include "diObsDialogInfo.h"
#include "util/diKeyValue.h"

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
class ObsDialog : public DataDialog
{
  Q_OBJECT

public:
  ObsDialog(QWidget* parent, Controller* llctrl);

  std::string name() const override;

  /// update dialog after re-reading setupfile
  void updateDialog() override;

  ///return command strings
  PlotCommand_cpv getOKString() override;

  ///insert command strings
  void putOKString(const PlotCommand_cpv& vstr) override;

  ///return short name of current command
  std::string getShortname();

  void archiveMode(bool on);

  std::vector<std::string> writeLog();

  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);

public /*Q_SLOTS*/:
  void updateTimes() override;

public Q_SLOTS:
  void getTimes(bool update = true);

protected:
  void doShowMore(bool show) override;

private Q_SLOTS:
  void plotSelected(int index);
  void multiplotClicked(bool);
  void extensionToggled(bool);
  void criteriaOn();
  void rightButtonClicked(const std::string&);
  void criteriaListSelected(int);
  void criteriaSelected(QListWidgetItem* );
  void signSlot(int);
  void sliderSlot(int);
  void stepSlot(int);
  void changeCriteriaString();
  void deleteSlot();
  void deleteAllSlot();
  void saveSlot();

Q_SIGNALS:
  void setCriteria( std::string, bool );

private:
  void loadDialogInfo();
  void makeExtension();
  std::string makeCriteriaString();
  bool newCriteriaString();
  void updateExtension();
  int findPlotnr(const miutil::KeyValue_v&);

private:
  int nr_plot;
  std::vector<miutil::KeyValue_v> savelog;
  int m_selected;
  QComboBox* plotbox;
  QStackedWidget* stackedWidget;
  std::vector<ObsWidget*> obsWidget;
  bool multiplot;
  ToggleButton* multiplotButton;

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
  bool freeze;

  ObsDialogInfo dialog;
};

#endif
