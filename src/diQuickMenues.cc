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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diQuickMenues.h>
#include <diLocalSetupParser.h>
#include <fstream>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.QuickMenues"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

// write a quick-menu to file
bool writeQuickMenu(const quickMenu& qm, bool newSyntax)
{
  std::string filename;

  if (not miutil::contains(qm.filename, "/")) {
    filename = LocalSetupParser::basicValue("homedir") + "/";
  }
  filename += qm.filename;
  ofstream menufile(filename.c_str());

  if (!menufile){
    if(newSyntax){
      METLIBS_LOG_WARN("QuickMenu Warning: Old syntax in quick-menu file:"
          << filename);
    }else {
      METLIBS_LOG_WARN("QuickMenu Error: Could not write quick-menu file:"
          << filename);
    }
    return false;
  }

  menufile << "# Name and plot string of Quick-menu" << endl;
  menufile << "# '#' marks comments" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << "#-- Name of quick-menu, given in the dialog" << endl;
  menufile << "# \"name\"" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << "#-- Variable definitions" << endl;
  menufile << "# [XX=a,b,c,d,..]" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << "#-- Plots" << endl;
  menufile << "# '>Plot name' is separator between the different plots" << endl;
  menufile << "#------------------------------------------------" << endl;
  menufile << endl;
  menufile << "# quickmenu name" << endl;

  menufile << "\"" << qm.name << "\"" << endl;
  menufile << endl;

  menufile << "# variables" << endl;

  // write options
  int m,n= qm.opt.size();
  for (int i=0; i<n; i++){
    menufile << "[" << qm.opt[i].key << "=";
    m= qm.opt[i].options.size();
    for (int j=0; j<m; j++){
      menufile << qm.opt[i].options[j];
      if (j<m-1) menufile << ",";
    }
    menufile << endl;
  }

  menufile << endl;

  // the plots
  n= qm.menuitems.size();
  for (int i=0; i<n; i++){
    menufile << "#" << endl;
    menufile << ">" << qm.menuitems[i].name << endl;
    m= qm.menuitems[i].command.size();
    for (int j=0; j<m; j++){
      menufile << qm.menuitems[i].command[j] << endl;
    }
  }

  return true;
}


// parse quick-menu-file
bool readQuickMenu(quickMenu& qm)
{
  quickMenuItem mi;
  quickMenuOption op;
  std::string value;
  vector<std::string> tokens, stokens;
  int updates=0;

  std::string filename= qm.filename;
  ifstream menufile(filename.c_str());
  int numitems=0;
  qm.menuitems.clear();
  qm.opt.clear();
  qm.plotindex= 0;

  if (!menufile){ // menufile not ok
    METLIBS_LOG_WARN("QuickMenu Warning: Could not open quickmenu file "
        << filename);
    return false;
  }


  std::string line;
  while (getline(menufile,line)){
    miutil::trim(line);
    if (line.length()==0) continue;
    if (line[0]=='#') continue;

    if (line[0]=='"'){
      // name of quickmenu
      if (line[line.length()-1]=='"')
        qm.name = line.substr(1,line.length()-2);
      else
        qm.name = line.substr(1,line.length()-1);

    } else if (line[0]=='['){
      // variable/options
      if (line[line.length()-1]==']')
        line = line.substr(1,line.length()-2);
      else
        line = line.substr(1,line.length()-1);
      tokens= miutil::split(line, "=");
      if (tokens.size()>1){
        op.key= tokens[0];
        value= tokens[1];
        op.options= miutil::split(value, ",", false); //Do not skip blank enteries
        op.def= (op.options.size()>0 ? op.options[0] : "");
        // add a new option
        qm.opt.push_back(op);
      } else {
        METLIBS_LOG_ERROR("QuickMenu Error: defined option without items in file "
            << filename);
        menufile.close();
        return false;
      }
    } else if (line[0]=='>'){
      // new menuitem
      qm.menuitems.push_back(mi);
      qm.menuitems[numitems].name= line.substr(1,line.length()-1);
      qm.menuitems[numitems].command.clear();
      numitems++;
    } else {
      // commands
      if (numitems>0) {
        updates += updateSyntax(line);
        qm.menuitems[numitems-1].command.push_back(line);
      } else {
        METLIBS_LOG_ERROR("QuickMenu Error: command line without defined menuitem in file:"
            << filename);
        menufile.close();
        return false;
      }
    }
  }
  menufile.close();
  if (qm.name.empty())
    qm.name= "Udefinert navn";

  //if old syntax changed, update file
  if(updates > 0)
    writeQuickMenu(qm,true);

  return true;
}

int updateSyntax(string& line)
{

  if( miutil::contains(line, "_3_farger")){
    miutil::replace(line, "_3_farger","");
    return 1;
  }

  if( miutil::contains(line, "test.contour.shading=1")){
    miutil::replace(line, "test.contour.shading=1","palettecolours=standard");
    return 1;
  }

  if( miutil::contains(line, "st.nr")){
    if( miutil::contains(line, "st.nr("))
      miutil::replace(line, "st.nr","st.no");
    else
      miutil::replace(line, "st.nr","st.no(5)");
    return 1;
  }

  if( miutil::contains(line, "(74,504)")){
    miutil::replace(line, "(74,504)","(uk)");
    return 1;
  }
  if( miutil::contains(line, "(74,533)")){
    miutil::replace(line, "(74,533)","(uk)");
    return 1;
  }
  if( miutil::contains(line, "(74,604)")){
    miutil::replace(line, "(74,604)","(uk)");
    return 1;
  }
  if( miutil::contains(line, "font=Helvetica")){
    miutil::replace(line, "font=Helvetica","font=BITMAPFONT");
    return 1;
  }

  return 0;
}
