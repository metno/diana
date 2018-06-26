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
#ifndef _qtQuickMenu_h
#define _qtQuickMenu_h

#include "diPlotCommand.h"
#include "diQuickMenuTypes.h"
#include "diQuickMenues.h"

#include <QDialog>

#include <vector>
#include <deque>

class QComboBox;
class QTreeView;
class QSortFilterProxyModel;
class QStandardItemModel;
class QLineEdit;
class QLabel;
class QTextEdit;
class QItemSelection;

/*!
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

  QTreeView* menubox;
  QSortFilterProxyModel* menuFilter;
  QStandardItemModel* menuItems;
  QLineEdit* menuFilterEdit;

  QComboBox* optionmenu[maxoptions]; // options for quickmenu
  QLabel* optionlabel[maxoptions];   // ..label for this
  QTextEdit* comedit;                // editor for command-text
  QLabel* comlabel;                  // ..label for this
  QPushButton* updatebut;            // update static menu-item

  int timerinterval;  // for demo
  bool timeron;       // for demo
  int demoTimer;      // for demo

  bool updating_options;

  std::vector<quickMenu> qm;        // datastructure for quickmenus

  //! index to selected quickmenu in qm
  int selected_list;

  //! menu index used for last plot
  /*! Used together with plotted_item. Changed by apply, adding to history, and admin. */
  int plotted_list;

  //! item index used for last plot
  /*! Used together with plotted_list. */
  int plotted_item;

  int old_list;
  int old_item;

  void selectList(int i); //! set active menu
  void updateOptions();   //! update option gui after changing selecting active menu

  void fillHistoryMenus();                                       // read history menus from $HOMEDIR
  void fillPrivateMenus();                                       // read user menus from $HOMEDIR
  void fillStaticMenus(const std::vector<QuickMenuDefs>& qdefs); // read initial menus from setup

  bool isValidList(int menu) const;

  void saveChanges(int,int);            // save command-text into qm
  void varExpand(std::vector<std::string>&);    // expand variables in command-text
  std::vector<int> sortKeys();           // sort keys by length
  void fillMenuList(bool select = true); // fill main menu combobox
  void setCommand();
  void getCommand(std::vector<std::string>&);   // get command-text from comedit
  void timerEvent(QTimerEvent*) override;       // timer for demo-mode

  QString instanceNameSuffix() const;

Q_SIGNALS:
  void QuickHide();   // request hide for this dialog

protected:
  void closeEvent(QCloseEvent*) override;

public:
  enum { HISTORY_MAP, HISTORY_VCROSS };

  QuickMenu(QWidget* parent, const std::vector<QuickMenuDefs>& qdefs);

  void start();

  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);
  std::vector<std::string> writeLog();

  //! Push a new command on the history-stack
  /*! Keeps at most maxplotsinstack history items. */
  void addPlotToHistory(const std::string& name, const PlotCommand_cpv& pstr, int list = HISTORY_MAP);

  bool prevQPlot(); ///< previous QuickMenu plot
  bool nextQPlot(); ///< next QuickMenu plot
  bool stepQPlot(int delta);
  bool prevHPlot(int index=0); ///< previous History plot
  bool nextHPlot(int index=0); ///< next History plot
  bool stepHistoryPlot(int menu, int delta); ///< previous/next History plot
  bool prevList();  ///< previous Menu
  bool nextList();  ///< next Menu

  bool applyItem(const std::string &list, const std::string &item); 

  void applyPlot();

  void getDetails(int& item_index, std::string& listname, std::string& itemname);

  //! get list with names of user-defined quickmenues
  std::vector<std::string> getUserMenus();

  void addMenu(const std::string& name);
  bool addToMenu(const std::string& name);
  std::string getCurrentName();

Q_SIGNALS:
  void Apply(const PlotCommand_cpv&, bool); ///< send plot-commands
  void showsource(const std::string, const std::string=""); ///< activate help

private Q_SLOTS:
  void menuboxSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected); // list selected
  void menuboxItemActivated(const QModelIndex& index);                                            // list item double-clicked
  void filterMenus(const QString& filtertext);

  void comChanged();                // command-text changed callback
  void adminButton();               // start admin dialog
  void updateButton();              // update menu-item
  void plotActiveMenu();            // send current plotcommand
  void demoButton(bool on);         // start/stop demo-mode
  void intervalChanged(int value);  // demo-timer changed
  void comButton(bool on);          // show/hide command-text
  void helpClicked();               // help-button callback
  void optionChanged(int index);

  void onInstanceNameChanged(const QString& name);
};

#endif
