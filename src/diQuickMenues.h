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
#ifndef _diQuickMenues_h
#define _diQuickMenues_h

#include <deque>
#include <set>
#include <string>
#include <vector>

/// options for plot-commands: Example: UTC= 0,6,12,18
struct quickMenuOption{
  std::string key;             ///< name of key
  std::vector<std::string> options; ///< available options
  std::string def;             ///< default option
};

/// one menuitem: menuname + plotcommands
struct quickMenuItem{
  std::string name;            ///< menuname
  std::vector<std::string> command; ///< command strings
};

/// contents of one quickmenu
struct quickMenu
{
  enum Type { QM_SHARED, QM_USER, QM_HISTORY_MAIN, QM_HISTORY_VCROSS };

  std::string filename; ///< file containing menu definitions
  std::string name;     ///< name of menuitem
  Type type;
  int item_index;                     ///< index of the "current" item for this quickMenu
  std::vector<quickMenuOption> opt;   /// any quickMenuOption
  std::deque<quickMenuItem> menuitems;/// all items in this menu

  quickMenu();
  ~quickMenu();

  bool is_history() const { return type == QM_HISTORY_MAIN || type == QM_HISTORY_VCROSS; }

  const quickMenuItem& item(int i) const { return menuitems[i]; }

  quickMenuItem& item(int i) { return menuitems[i]; }

  const quickMenuItem& item() const { return item(item_index); }

  quickMenuItem& item() { return item(item_index); }

  bool step_item(int delta);

  bool valid_item(int pi) const { return pi >= 0 && pi < (int)menuitems.size(); }

  bool valid_item() const { return valid_item(item_index); }

  const std::vector<std::string>& command() const;

  //! check if any variables are used in c
  std::set<int> used_options(const std::string& c) const;

  //! replace variables in com with "def" value
  void expand_options(std::vector<std::string>& com) const;

  bool write() const;
  bool read();

  // private:
  //! sort keys by length - make index-list
  std::vector<int> sorted_keys() const;
};

/// write a quick-menu to file
bool writeQuickMenu(const quickMenu& qm);

/// read quick-menu file, and fill struct
bool readQuickMenu(quickMenu& qm);

//! write quickmenu log
/*! Log contains option values. */
std::vector<std::string> writeQuickMenuLog(const std::vector<quickMenu>& qm);

//! read quickmenu log
/*! Log as written by writeQuickMenuLog. */
void readQuickMenuLog(std::vector<quickMenu>& qm, const std::vector<std::string>& loglines);

/// if old syntax, update
bool updateCommandSyntax(std::vector<std::string>& lines);

void replaceDynamicQuickMenuOptions(const std::vector<std::string>& oldCommand, std::vector<std::string>& newCommand);

#endif
