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

#include "diKVListPlotCommand.h"
#include "diSatTypes.h"

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
class Controller;

/**
  \brief Dialogue for plotting satellite and radar pictures 
*/
class SatDialog : public DataDialog
{
    Q_OBJECT

  public:
    /**
      \brief struct state describing selected picture
    */
    struct state
    {
      miutil::KeyValue_v OKString;
      int iname;
      int iarea;
      int ifiletime;
      int ichannel;
      int iautotimefile;
      std::string name;            ///<satellite name
      std::string area;            ///>filetype
      miutil::miTime filetime;     ///<time
      std::string channel;         ///<selected channel
      std::string filename;        ///<filename
      miutil::KeyValue_v advanced; ///<plotting options
      miutil::KeyValue_v external; ///<nothing to do whith the dialog
      bool mosaic;                 ///<plot mosaic of pictures
      int totalminutes;            ///<timediff
  };

  SatDialog(QWidget* parent, Controller* llctrl);

  std::string name() const override;

  ///return command strings
  PlotCommand_cpv getOKString() override;

  ///insert command strings
  void putOKString(const PlotCommand_cpv& vstr) override;

  ///return short name of current commonad
  std::string getShortname();
  /// set mode to read files from archive
  void archiveMode(){emitSatTimes(true); updateTimefileList();}

  std::vector<std::string> writeLog();

  /// read log string
  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);

public /*Q_SLOTS*/:
  void updateTimes() override;

  void updateDialog() override;

protected:
  void doShowMore(bool show) override;

private:
  void updateFileListWidget(int);
  void updateTimefileList();
  void updateChannelBox(bool select);
  void updatePictures(int index, bool updateAbove);
  void emitSatTimes(bool update=false);
  int addSelectedPicture();
  //decode part of OK string
  state decodeString(const miutil::KeyValue_v &tokens);
  // make string from state
  miutil::KeyValue_v makeOKString(state & okVar);
  void putOptions(const state& okVar);

  std::string pictureString(state,bool);  

private Q_SLOTS:
  void DeleteClicked();
  void DeleteAllClicked();
  void nameActivated( int in );
  void timefileClicked(int tt);
  void timefileListSlot(QListWidgetItem * item);
  void fileListWidgetClicked(QListWidgetItem * item);
  void channelboxSlot(QListWidgetItem * item);
  void picturesSlot(QListWidgetItem * item);
  void doubleDisplayDiff( int number );
  void mosaicToggled(bool on);
  void advancedChanged();
  void upPicture();
  void downPicture();
  void updateColours();

private:
  typedef std::map<std::string, miutil::KeyValue_v> areaoptions_t;
  typedef std::map<std::string, areaoptions_t> satoptions_t;
  satoptions_t satoptions;
  std::vector<state> m_state; // pictures to plot
  plottimes_t times;          // emitted to TimeSlider

  int m_nr_image;
 
  std::string m_channelstr;
  miutil::miTime m_time;  
  std::vector<SatFileInfo> files;
  
  float m_scalediff;

  SatDialogInfo dialogInfo;
  
  QComboBox* namebox;
  QListWidget* fileListWidget;
  
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
