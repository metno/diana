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
#ifndef _objectDialog_h
#define _objectDialog_h

#include "qtDataDialog.h"

#include "diObjTypes.h"
#include "util/diKeyValue.h"

#include <vector>

class PushButton;
class QListWidget;
class QListWidgetItem;
class QSlider;
class QLCDNumber;
class QCheckBox;
class QButtonGroup;
class QGroupBox;

class EditComment;
class ObjectManager;
class ToggleButton;

/**
  \brief Dialog for plotting weather objects
  Analyses, prognoses, sigcharts etc.
*/
class ObjectDialog : public DataDialog
{
  Q_OBJECT

public:
  ObjectDialog(QWidget* parent, Controller* llctrl);

  std::string name() const override;

  /// the plot info strings
  PlotCommand_cpv getOKString() override;

  /// set the dialogue elements from a plot info string
  void putOKString(const PlotCommand_cpv& vstr) override;

  /// creates a short name for the current settings (used in quick menues)
  std::string getShortname();

  /// read comment belonging to objects
  void commentUpdate();

  /// show dialog
  void showAll();

  /// hide dialog
  void hideAll();

  /// set mode to read files from archive
  void archiveMode( bool on );

  /**
   * \brief Variables to plot an object file
   * Name, file etc.
  */
  struct PlotVariables {
    //variables for each plot
    std::string objectname;
    std::string file,time;
    int totalminutes;
    float alphanr;
    std::map<std::string, bool> useobject;
    miutil::KeyValue_v external;
  };

public /*Q_SLOTS*/:
  void updateTimes() override;
  void updateDialog() override;

private:
  ObjectManager* m_objm;

  //max total time diff. between selected time and file time
  int m_totalminutes;
  float m_scalediff;

  float m_alphascale;
  float m_alphanr;

  bool useArchive;
  std::vector<std::string> objectnames;

  // Emitted to TimeSlider
  plottimes_t times;
  //list of object files currently selected
  std::vector <ObjFileInfo> files;
  //LB: current variables (only used for external)
  PlotVariables plotVariables;

  //update the list of files  (if refresh = true read from disk)
  void updateTimefileList(bool refresh);
  //updates the text that appears in the selectedFileList box
  void updateSelectedFileList();

  //decode part of OK string
  static PlotVariables decodeString(const miutil::KeyValue_v &tokens);
  // make string from plotVariables
  std::string makeOKString(PlotVariables & okVar);

  //************** q tWidgets that appear in the dialog  *******************

  // Combobox for selecting region name
  QListWidget * namebox;

  //3 Buttons for selecting "auto"/"tid"/"fil"
  QButtonGroup* timefileBut;
  ToggleButton* autoButton;
  ToggleButton* timeButton;
  ToggleButton* fileButton;

  //list of times/files
  QListWidget* timefileList; 

 // the box showing which files have been choosen
  QListWidget* selectedFileList;  
 

 //Check boxes for selecting fronts/symbols/areas
  QGroupBox * bgroupobjects; 
  QCheckBox *cbs0;
  QCheckBox *cbs1;
  QCheckBox *cbs2;
  QCheckBox *cbs3;



  //lCD number/slider for showing/selecting max time diff.
  QLCDNumber* diffLcdnum;
  QSlider* diffSlider;

  //lcd number/slider for showing alpha value of objects
  ToggleButton* alpha;
  QLCDNumber* alphalcd;
  QSlider* salpha;

  ToggleButton* commentbutton;

  EditComment* objcomment;

private slots:
  void nameListClicked(  QListWidgetItem * );
  void timefileClicked(int tt);
  void timefileListSlot( QListWidgetItem * item );
  void DeleteClicked();
  void doubleDisplayDiff( int number );
  void greyAlpha( bool on );
  void alphaDisplay( int number );
  void commentClicked(bool);
  void hideComment();
};

#endif
