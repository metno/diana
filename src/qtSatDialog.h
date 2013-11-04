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
#ifndef _satdialog_h
#define _satdialog_h

#include <diController.h>
#include <QDialog>

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

/**
  \brief Dialogue for plotting satellite and radar pictures 
*/
class SatDialog: public QDialog
{
    Q_OBJECT
public:


/**
  \brief struct state describing selected picture
*/  
  struct state{
    std::string OKString;
    int iname;
    int iarea;
    int ifiletime;
    int ichannel;
    int iautotimefile;
    std::string name; ///<satellite name
    std::string area; ///>filetype
    miutil::miTime filetime; ///<time
    std::string channel; ///<selected channel
    std::string filename; ///<filename
    std::string advanced; ///<plotting options
    std::string external; ///<nothing to do whith the dialog
    bool mosaic; ///<plot mosaic of pictures       
    int totalminutes;///<timediff
  };


  SatDialog( QWidget* parent, Controller* llctrl );
  ///return command strings
  std::vector<std::string> getOKString();
  ///insert command strings
  void putOKString(const std::vector<std::string>& vstr);
  ///return short name of current commonad
  std::string getShortname();
  /// refresh list of files in timefilelist
  void RefreshList();
  /// set mode to read files from archive
  void archiveMode(){emitSatTimes(true); updateTimefileList();}
  std::vector<std::string> writeLog() ;
  /// read log string
  void readLog(const std::vector<std::string>& vstr,
	       const std::string& thisVersion, const std::string& logVersion);
  ///called when the dialog is closed by the window manager
protected:
  void closeEvent( QCloseEvent* );

private:

  std::map< std::string, std::map< std::string,std::string > > satoptions;
  std::vector<state> m_state; //pictures to plot
  std::vector<miutil::miTime> times;    //emitted to TimeSlider 
  static miutil::miTime ztime;


  void updateFileListWidget(int);
  void updateTimefileList();
  void updateChannelBox(bool select);
  void updatePictures(int index, bool updateAbove);
  void emitSatTimes(bool update=false);
  int addSelectedPicture();
  //decode part of OK string
  state decodeString(const std::vector<std::string> & tokens);
  // make string from state
  std::string makeOKString(state & okVar);
  void putOptions(const state okVar);

  std::string pictureString(state,bool);  
  // get the time string on the form yyyymmddhhmn from time
  std::string stringFromTime(const miutil::miTime& t);
  //get time from string
  miutil::miTime timeFromString(const std::string & timeString);

private slots:
  void DeleteClicked();
  void DeleteAllClicked();
  void nameActivated( int in );
  void timefileClicked(int tt);
  void Refresh();
  void timefileListSlot(QListWidgetItem * item);
  void fileListWidgetClicked(QListWidgetItem * item);
  void channelboxSlot(QListWidgetItem * item);
  void picturesSlot(QListWidgetItem * item);
  void doubleDisplayDiff( int number );
  void mosaicToggled(bool on);
  void helpClicked();
  void applyhideClicked();
  void advancedtoggled( bool on );
  void hideClicked();
  void advancedChanged();
  void upPicture();
  void downPicture();
  void updateColours();

signals:
  void SatApply();
  void SatHide();
  void showsource(const std::string, const std::string="");
  void emitTimes(const std::string&, const std::vector<miutil::miTime>&,bool );

private:

  Controller* m_ctrl;
  
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
  
  QPushButton* refresh;
  //  ToggleButton* onoff;
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
