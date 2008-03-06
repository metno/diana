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
#ifndef _qtQuickMenu_h
#define _qtQuickMenu_h

#include <qdialog.h>
//Added by qt3to4:
#include <QLabel>
#include <QTimerEvent>
#include <miString.h>
#include <vector>
#include <deque>
#include <diController.h>
#include <diQuickMenues.h>
#include <diCommonTypes.h>

using namespace std; 

class QComboBox;
class Q3ListBox;
class QLabel;
class Q3TextEdit;

/**
   \brief Quick menu
   
   Main quick menu viewer
   - show all menues for selection of items
   - autoviewer

*/

class QuickMenu : public QDialog {
  Q_OBJECT
private:
  enum { maxoptions= 10 };      // maximum options
  enum { maxplotsinstack= 100}; // size of history-stack

  QComboBox* menulist;     // main quickmenu-combobox
  Q3ListBox* list;          // list of plots in quickmenu
  bool optionsexist;       // defined options in quickmenu
  QComboBox* optionmenu[maxoptions]; // options for quickmenu
  QLabel* optionlabel[maxoptions];   // ..label for this
  Q3TextEdit* comedit;                // editor for command-text
  QLabel* comlabel;                  // ..label for this
  QPushButton* resetbut;             // reset static menu-item
  QPushButton* updatebut;            // update static menu-item

  Controller* contr;

  bool comset;        // command area text is set
  int activemenu;     // index to active quickmenu
  int timerinterval;  // for demo
  bool timeron;       // for demo
  int demoTimer;      // for demo

  vector<quickMenu> qm;        // datastructure for quickmenues
  vector<quickMenu> orig_qm;   // original copies of static menues
  vector<quickMenu> chng_qm;   // changed static menues

  bool browsing;       // user is browsing
  int prev_plotindex;  // last plotted command
  int prev_listindex;  // -- " --

  int firstcustom;     // first and last custom menues
  int lastcustom;
  bool instaticmenu;   // inside static menu

  void setCurrentMenu(int i);           // set active menu
  void fillStaticMenues();              // read initial menues from setup
  void saveChanges(int,int);            // save command-text into qm
  void varExpand(vector<miString>&);    // expand variables in command-text
  void fillMenuList();                  // fill main menu combobox
  void getCommand(vector<miString>&);   // get command-text from comedit
  void timerEvent(QTimerEvent*);        // timer for demo-mode
  bool itemChanged(int menu, int item); // check changes in static menues
  void replaceDynamicOptions(vector<miString>& oldCommand,
			     vector<miString>& newCommand);


signals:
  void QuickHide();   // request hide for this dialog

public:
  QuickMenu(QWidget *parent, Controller* c,
	    Qt::WFlags f=0);
  bool close(bool alsoDelete);
  void start();

  void readLog(const vector<miString>& vstr,
	       const miString& thisVersion,
	       miString& logVersion);
  vector<miString> writeLog();

  /// add command to history
  void pushPlot(const miString& name,
		const vector<miString>& pstr);

  bool prevQPlot(); ///< previous QuickMenu plot
  bool nextQPlot(); ///< next QuickMenu plot
  bool prevHPlot(); ///< previous History plot
  bool nextHPlot(); ///< next History plot
  bool prevList();  ///< previous Menu
  bool nextList();  ///< next Menu

  bool applyItem(const miString &list, const miString &item); 

  void applyPlot();
  
  void getDetails(int& plotidx,
		  miString& listname,
		  miString& plotname);
  void startBrowsing();

  // quick-quick menu methods
  vector<miString> getCustomMenues();
  bool addMenu(const miString& name);
  bool addToMenu(const int idx);

signals:
  void Apply(const vector<miString>& s, bool); ///< send plot-commands
  void requestUpdate(const vector<miString>&,  ///< request static menu-item
		     vector<miString>&);       // update: old,new
  void showdoc(const miString); ///< activate help

private slots:
  void menulistActivate(int);       // quick-menu combobox activated
  void listHighlight(int);          // single plot selected
  void listSelect(int);             // single plot double-clicked
  void comChanged();                // command-text changed callback
  void adminButton();               // start admin dialog
  void resetButton();               // reset menu-item
  void updateButton();              // update menu-item
  void plotButton();                // send current plotcommand
  void demoButton(bool on);         // start/stop demo-mode
  void intervalChanged(int value);  // demo-timer changed
  void comButton(bool on);          // show/hide command-text
  void helpClicked();               // help-button callback
};

#endif
