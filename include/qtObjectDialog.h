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
#ifndef _objectDialog_h
#define _objectDialog_h

#include <qdialog.h>
#include <qfont.h>
#include <miString.h>
#include <vector>
#include <miString.h>
#include <diController.h>

class PushButton;
class QListBox;
class QComboBox;
class QVBoxLayout;
class QHBoxLayout;
class QVButtonGroup;
class QLabel;
class QSlider;
class QLCDNumber;
class QCheckBox;
class QButtonGroup;
class EditComment;
class AddtoDialog;
class ObjectManager;
class ToggleButton;

/**

  \brief Dialogue for plotting weather objects
   
   Analyses, prognoses, sigcharts etc.

*/
class ObjectDialog: public QDialog
{
  Q_OBJECT

public:

  //the constructor
  ObjectDialog( QWidget* parent, Controller* llctrl );
  /// the plot info strings
  vector<miString> getOKString();
  /// set the dialogue elements from a plot info string
  void putOKString(const vector<miString>& vstr);
  /// creates a short name for the current settings (used in quick menues)
  miString getShortname();
  void requestQuickUpdate(const vector<miString>& , vector<miString>& );
  /// read comment belonging to objects
  void commentUpdate();
  /// show dialog
  void showAll();
  /// hide dialog
  void hideAll();
  /// set mode to read files from archive
  void archiveMode( bool on );
  ///called when the dialog is closed by the window manager
  bool close(bool alsoDelete);

/**

  \brief Variables to plot an object file
   
   Name, file etc.

*/
  struct PlotVariables {
    //variables for each plot
    miString objectname;
    miString file,time;
    int totalminutes;
    float alphanr;
    map <miString,bool> useobject;    
    miString external;
  };
  
private:

  Controller* m_ctrl;
  ObjectManager* m_objm;

  //max total time diff. between selected time and file time
  int m_totalminutes;
  float m_scalediff;

  float m_alphascale;
  float m_alphanr;

  bool useArchive;
  vector<miString> objectnames;
  //HK ??? what to do here?
  map<miString,PlotOptions> mapPlotOptions;

  //index of currently selected file
  int m_timefileListIndex;
  //index of namebox
  int m_nameboxIndex; //HK ??? necessary ?
  //Emitted to TimeSlider 
  vector<miTime> times;  
  //list of object files currently selected
  vector <ObjFileInfo> files;
  //LB: current variables (only used for external)
  PlotVariables plotVariables;

  //update the list of files  (if refresh = true read from disk)
  void updateTimefileList(bool refresh);
  //updates the text that appears in the filenames box
  void updateFilenames();

  //decode part of OK string
  PlotVariables decodeString(const vector <miString> & tokens);
  // make string from plotVariables
  miString makeOKString(PlotVariables & okVar);
  // get the time string on the form yyyymmddhhmn from time
  miString stringFromTime(const miTime& t);


//************** q tWidgets that appear in the dialog  *******************

  // Combobox for selecting region name
  QListBox * namebox;

  //3 Buttons for selecting "auto"/"tid"/"fil"
  QButtonGroup* timefileBut;
  ToggleButton* autoButton;
  ToggleButton* timeButton;
  ToggleButton* fileButton;

  //list of times/files
  QListBox* timefileList; 


 // the box showing which files have been choosen
  QLabel* filesLabel;
  QListBox* filenames;  
 

 //Check boxes for selecting fronts/symbols/areas
  QVButtonGroup * bgroupobjects; 
  QCheckBox *cbs0;
  QCheckBox *cbs1;
  QCheckBox *cbs2;
  QCheckBox *cbs3;



  //lCD number/slider for showing/selecting max time diff.
  QLabel* diffLabel;
  QLCDNumber* diffLcdnum;
  QSlider* diffSlider;

  //lcd number/slider for showing alpha value of objects
  ToggleButton* alpha;
  QLCDNumber* alphalcd;
  QSlider* salpha;

  //delete and refresh buttons
  QPushButton* Delete;
  QPushButton* refresh;

  //push buttons for apply/hide/help
  QPushButton* objapply;
  QPushButton* objapplyhide;
  QPushButton* objhide;

  //buttons for showing comments and help
  ToggleButton* commentbutton;
  QPushButton* objhelp; 

  //QPushButton* newfilebutton;
  //QPushButton* addtodialogbutton;
  
  //layouts for placing buttons
  QVBoxLayout* v3layout;
  QVBoxLayout* v5layout;
  QHBoxLayout* hlayout;
  QHBoxLayout* difflayout;
  QHBoxLayout* alphalayout;
  QHBoxLayout* hlayout2;
  QHBoxLayout* hlayout3;
  //QHBoxLayout* hlayout4;
  QVBoxLayout* vlayout;


  EditComment* objcomment;
  AddtoDialog * atd;
  

private slots:
  void DeleteClicked();
  void nameActivated();
  void timefileClicked(int tt);
  void Refresh();
  void timefileListSlot( int index );
  void doubleDisplayDiff( int number );
  void applyhideClicked();
  void helpClicked();
  void greyAlpha( bool on );
  void alphaDisplay( int number );
  void commentClicked(bool);
  void hideComment();
  void newfileClicked();
  void addtodialogClicked();

signals:
  void ObjHide();
  void ObjApply();
  void showdoc(const miString);
  void emitTimes( const miString&, const vector<miTime>& ,bool );

};

#endif




