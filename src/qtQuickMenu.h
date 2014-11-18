/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2014 met.no

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

#include <diController.h>
#include <diQuickMenues.h>
#include <diCommonTypes.h>

#include <QDialog>
#include <QTimerEvent>

#include <vector>
#include <deque>

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QTextEdit;

/**
   \brief Quick menu
   
   Main quick menu viewer
   - show all menus for selection of items
   - autoviewer

*/

class QuickMenu : public QDialog {
  Q_OBJECT

private:
  enum { maxoptions= 20 };      // maximum options
  enum { maxplotsinstack= 100}; // size of history-stack

  QComboBox* menulist;     // main quickmenu-combobox
  QListWidget* list;          // list of plots in quickmenu
  bool optionsexist;       // defined options in quickmenu
  QComboBox* optionmenu[maxoptions]; // options for quickmenu
  QLabel* optionlabel[maxoptions];   // ..label for this
  QTextEdit* comedit;                // editor for command-text
  QLabel* comlabel;                  // ..label for this
  QPushButton* resetbut;             // reset static menu-item
  QPushButton* updatebut;            // update static menu-item

  Controller* contr;

  bool comset;        // command area text is set
  int activemenu;     // index to active quickmenu
  int timerinterval;  // for demo
  bool timeron;       // for demo
  int demoTimer;      // for demo

  std::vector<quickMenu> qm;        // datastructure for quickmenus
  std::vector<quickMenu> orig_qm;   // original copies of static menus
  std::vector<quickMenu> chng_qm;   // changed static menus

  bool browsing;       // user is browsing
  int prev_plotindex;  // last plotted command
  int prev_listindex;  // -- " --

  int firstcustom;     // first and last custom menus
  int lastcustom;
  int firststatic;     // first static menu
  bool instaticmenu;   // inside static menu

  void setCurrentMenu(int i);           // set active menu
  void fillPrivateMenus();              // read menus from $HOMEDIR
  void fillStaticMenus();              // read initial menus from setup
  void saveChanges(int,int);            // save command-text into qm
  void varExpand(std::vector<std::string>&);    // expand variables in command-text
  void fillMenuList();                  // fill main menu combobox
  void getCommand(std::vector<std::string>&);   // get command-text from comedit
  void timerEvent(QTimerEvent*);        // timer for demo-mode
  bool itemChanged(int menu, int item); // check changes in static menus
  void replaceDynamicOptions(std::vector<std::string>& oldCommand,
      std::vector<std::string>& newCommand);

Q_SIGNALS:
  void QuickHide();   // request hide for this dialog

protected:
  void closeEvent( QCloseEvent* );

public:

  enum {
    MAP,
    VCROSS,
    QMENU
  };

  QuickMenu(QWidget *parent, Controller* c);

  void start();

  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
  std::vector<std::string> writeLog();

  /// add command to history
  void pushPlot(const std::string& name,
      const std::vector<std::string>& pstr, int index=0);

  bool prevQPlot(); ///< previous QuickMenu plot
  bool nextQPlot(); ///< next QuickMenu plot
  bool prevHPlot(int index=0); ///< previous History plot
  bool nextHPlot(int index=0); ///< next History plot
  bool prevList();  ///< previous Menu
  bool nextList();  ///< next Menu

  bool applyItem(const std::string &list, const std::string &item); 

  void applyPlot();
  
  void getDetails(int& plotidx,
		  std::string& listname,
		  std::string& plotname);
  void startBrowsing();

  // quick-quick menu methods
  std::vector<std::string> getCustomMenus();
  bool addMenu(const std::string& name);
  bool addToMenu(const int idx);
  std::string getCurrentName();

Q_SIGNALS:
  void Apply(const std::vector<std::string>& s, bool); ///< send plot-commands
  void showsource(const std::string, const std::string=""); ///< activate help

private Q_SLOTS:
  void menulistActivate(int);       // quick-menu combobox activated
  void listClicked( QListWidgetItem *);       // single plot selected
  void listDoubleClicked( QListWidgetItem *); // single plot double-clicked
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
