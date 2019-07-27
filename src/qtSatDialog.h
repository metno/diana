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
#ifndef _satdialog_h
#define _satdialog_h

#include "qtDataDialog.h"

#include "diSatDialogData.h"
#include "diSatPlotCommand.h"

#include <vector>
#include <map>

class QSlider;
class ToggleButton;
class PushButton;
class QButtonGroup;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLCDNumber;
class SatDialogAdvanced;
class SatDialogData;

/**
  \brief Dialogue for plotting satellite and radar pictures
*/
class SatDialog : public DataDialog
{
  Q_OBJECT

public:
  typedef std::map<std::string, SatPlotCommand_cp> subtypeoptions_t;
  typedef std::map<std::string, subtypeoptions_t> imageoptions_t;

  SatDialog(SatDialogData* sdd, QWidget* parent = 0);
  ~SatDialog();

  std::string name() const override;

  ///return command strings
  PlotCommand_cpv getOKString() override;

  //! called from MainWindow to put values into dialog
  void putOKString(const PlotCommand_cpv& vstr) override;

  ///return short name of current commonad
  std::string getShortname();

  /// set mode to read files from archive
  void archiveMode();

  std::vector<std::string> writeLog();

  /// read log string
  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);

  //! read log -- this is public so that it can be tested
  static void readSatOptionsLog(const std::vector<std::string>& vstr, imageoptions_t& satoptions);

public /*Q_SLOTS*/:
  void updateTimes() override;

  void updateDialog() override;

protected:
  void doShowMore(bool show) override;

private:
  void updateSubTypeList(int);
  void updateTimefileList(bool update);
  void updateChannelBox(bool select);
  void updatePictures(int index, bool updateAbove);
  void enableUpDownButtons();
  void emitSatTimes(bool update);
  int addSelectedPicture();

  void putOptions(SatPlotCommand_cp cmd);

  std::string pictureString(SatPlotCommand_cp cmd, bool);

private Q_SLOTS:
  void DeleteClicked();
  void DeleteAllClicked();
  void imageNameBoxActivated(int in);
  void timefileClicked(int tt);
  void timefileListSlot(QListWidgetItem * item);
  void subtypeNameListClicked(QListWidgetItem* item);
  void channelboxSlot(QListWidgetItem * item);
  void picturesSlot(QListWidgetItem * item);
  void doubleDisplayDiff( int number );
  void mosaicToggled(bool on);
  void advancedChanged();
  void upPicture();
  void downPicture();
  void updateColours();

private:
  std::unique_ptr<SatDialogData> sdd_;

  imageoptions_t satoptions;
  std::vector<SatPlotCommand_p> m_state; // pictures to plot

  std::string m_channelstr;
  miutil::miTime m_time;
  SatFile_v files;

  SatImage_v availableImages;

  QComboBox* imageNameBox;
  QListWidget* subtypeNameList;

  QLCDNumber* diffLcdnum;
  QSlider* diffSlider;

  ToggleButton* multiPicture;
  ToggleButton* mosaic;
  QPushButton* Delete;
  QPushButton* DeleteAll;
  QPushButton* upPictureButton;
  QPushButton* downPictureButton;

  QButtonGroup* timefileBut;
  ToggleButton* autoButton;
  ToggleButton* timeButton;
  ToggleButton* fileButton;
  QListWidget*  timefileList;
  QListWidget*  channelbox;
  QListWidget*  pictures;

  SatDialogAdvanced* sda;
  ToggleButton* advanced;
};

#endif
