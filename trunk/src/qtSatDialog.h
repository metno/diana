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

#include <QDialog>

#include <puTools/miString.h>
#include <vector>
#include <map>
#include <diController.h>

using namespace std;

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
    miutil::miString OKString;
    int iname;
    int iarea;
    int ifiletime;
    int ichannel;
    int iautotimefile;
    miutil::miString name; ///<satellite name
    miutil::miString area; ///>filetype
    miutil::miTime filetime; ///<time
    miutil::miString channel; ///<selected channel
    miutil::miString filename; ///<filename
    miutil::miString advanced; ///<plotting options
    miutil::miString external; ///<nothing to do whith the dialog
    bool mosaic; ///<plot mosaic of pictures       
    int totalminutes;///<timediff
  };


  SatDialog( QWidget* parent, Controller* llctrl );
  ///return command strings
  vector<miutil::miString> getOKString();
  ///insert command strings
  void putOKString(const vector<miutil::miString>& vstr);
  ///return short name of current commonad
  miutil::miString getShortname();
  ///check command strings, and return legal command strings
  void requestQuickUpdate(const vector<miutil::miString>& , vector<miutil::miString>& );
  /// refresh list of files in timefilelist
  void RefreshList();
  /// set mode to read files from archive
  void archiveMode(){emitSatTimes(true); updateTimefileList();}
  vector<miutil::miString> writeLog() ;
  /// read log string
  void readLog(const vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion, const miutil::miString& logVersion);
  ///called when the dialog is closed by the window manager
protected:
  void closeEvent( QCloseEvent* );

private:

  map< miutil::miString,map< miutil::miString,miutil::miString > > satoptions;
  vector<state> m_state; //pictures to plot
  vector<miutil::miTime> times;    //emitted to TimeSlider 
  static miutil::miTime ztime;


  void updateFileListWidget(int);
  void updateTimefileList();
  void updateChannelBox(bool select);
  void updatePictures(int index, bool updateAbove);
  void emitSatTimes(bool update=false);
  int addSelectedPicture();
  //decode part of OK string
  state decodeString(const vector <miutil::miString> & tokens);
  // make string from state
  bool compareStates(const state & oldOKVar,const state &newOkVar);
  miutil::miString makeOKString(state & okVar);
  void putOptions(const state okVar);

  miutil::miString pictureString(state,bool);  
  // get the time string on the form yyyymmddhhmn from time
  miutil::miString stringFromTime(const miutil::miTime& t);
  //get time from string
  miutil::miTime timeFromString(const miutil::miString & timeString);

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
  void showsource(const miutil::miString, const miutil::miString="");
  void emitTimes( const miutil::miString& ,const vector<miutil::miTime>&,bool );

private:

  Controller* m_ctrl;
  
  int m_nr_image;
 
  miutil::miString m_channelstr;
  miutil::miTime m_time;  
  vector<SatFileInfo> files;
  
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
