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
#ifndef _diQuickMenues_h
#define _diQuickMenues_h


#include <miString.h>
#include <vector>
#include <deque>

using namespace std; 

/// options for plot-commands: Example: UTC= 0,6,12,18
struct quickMenuOption{
  miString key;             ///< name of key
  vector<miString> options; ///< available options
  miString def;             ///< default option
};

/// one menuitem: menuname + plotcommands
struct quickMenuItem{
  miString name;            ///< menuname
  vector<miString> command; ///< command strings
};

/// contents of one quickmenu
struct quickMenu{
  miString filename; ///< file containing menu definitions
  miString name;     ///< name of menuitem
  int plotindex;     ///< 
  vector<quickMenuOption> opt;   /// any quickMenuOption 
  deque<quickMenuItem> menuitems;/// all items in this menu
};

/// write a quick-menu to file
bool writeQuickMenu(const quickMenu& qm, bool newSyntax=false);
/// read quick-menu file, and fill struct
bool readQuickMenu(quickMenu& qm);
/// if old syntax, update
int  updateSyntax(miString& line);

#endif
