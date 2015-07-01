/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "diQuickMenues.h"
#include "diLocalSetupParser.h"
#include "diUtilities.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

#include <fstream>

#define MILOGGER_CATEGORY "diana.QuickMenues"
#include <miLogger/miLogging.h>

using namespace::miutil;
using namespace std;

bool writeQuickMenu(const quickMenu& qm)
{
  METLIBS_LOG_SCOPE();

  std::ofstream menufile(qm.filename.c_str());
  if (!menufile){
    METLIBS_LOG_WARN("Cannot write quick-menu file '" << qm.filename << "'");
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

bool readQuickMenu(quickMenu& qm)
{
  METLIBS_LOG_SCOPE();
  quickMenuItem mi;
  quickMenuOption op;
  std::string value;
  vector<std::string> tokens, stokens;
  bool updates = false;

  std::string filename= qm.filename;
  std::ifstream menufile(qm.filename.c_str());
  int numitems=0;
  qm.menuitems.clear();
  qm.opt.clear();
  qm.plotindex= 0;

  if (!menufile){ // menufile not ok
    METLIBS_LOG_WARN("Could not open quickmenu file '" << qm.filename << "'");
    return false;
  }

  std::string line;
  while (std::getline(menufile, line)){
    miutil::trim(line);
    if (line.length()==0)
      continue;
    if (line[0]=='#')
      continue;

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
        op.options = miutil::split_protected(value, '"','"',",", false); //Do not skip blank enteries
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
      if (!qm.menuitems.empty()) {
        if (updateCommandSyntax(qm.menuitems.back().command))
          updates = true;
      }
      // new menuitem
      qm.menuitems.push_back(mi);
      qm.menuitems[numitems].name= line.substr(1,line.length()-1);
      qm.menuitems[numitems].command.clear();
      numitems++;
    } else {
      // commands
      if (numitems>0) {
        qm.menuitems[numitems-1].command.push_back(line);
      } else {
        METLIBS_LOG_ERROR("command line without defined menuitem in file '" << filename << "'");
        menufile.close();
        return false;
      }
    }
  }
  menufile.close();
  if (qm.name.empty())
    qm.name= "Udefinert navn";

  if (!qm.menuitems.empty()) {
    if (updateCommandSyntax(qm.menuitems.back().command))
      updates = true;
  }

  //if old syntax changed, update file
  if (updates)
    writeQuickMenu(qm);

  return true;
}

namespace {

std::string FieldSpecTranslation__getVcoorFromLevel(const std::string& level)
{
  if (level.find("hPa") != std::string::npos)
    return "pressure";
  if (level.find("E.") != std::string::npos)
    return "k";
  if (level.find("L.") != std::string::npos)
    return "model";
  if (level.find("FL") != std::string::npos)
    return "flightlevel";
  if (level.find("k") != std::string::npos)
    return "theta";
  if (level.find("pvu") != std::string::npos)
    return "pvu";
  if (level.find("C") != std::string::npos)
    return "temperature";
  if (level.find("m") != std::string::npos)
    return "depth";
  return "";
}

std::vector<std::string> split_boost(const std::string& pin)
{
  boost::tokenizer<boost::escaped_list_separator<char> > t(pin, boost::escaped_list_separator<char>("\\", " ", "\""));
  return std::vector<std::string>(t.begin(), t.end());
}

std::string FieldSpecTranslation__getNewFieldString(const std::string& pin, bool withModel)
{
  const std::vector<std::string> tokens = split_boost(pin);

  const size_t n = tokens.size();
  if (n < 2)
    return "";

  std::ostringstream ost;
  size_t k = 0;
  if (withModel) {
    size_t imodel;
    if (n >= 3 && tokens[0] == "FIELD") {
      //FIELD <modelName> <plotName>
      ost << "FIELD";
      imodel = 1;
    } else if (n >= 3 && tokens[0].empty()) {
      imodel = 1;
    } else {
      // <modelName> <plotName>
      imodel = 0;
    }
    while (imodel < n && tokens[imodel].empty())
      imodel += 1;
    if (imodel+1 < n)
      ost << " model=" << tokens[imodel]
          << " plot="  << tokens[imodel+1];
    k = imodel + 2;
  }

  for (; k < n; k++) {
    std::vector<std::string> vtoken;
    boost::algorithm::split(vtoken, tokens[k], boost::algorithm::is_any_of("="));
    if (vtoken.size() >= 2) {
      std::string key = boost::algorithm::to_lower_copy(vtoken[0]);
      if (key == "level" ) {
        if (vtoken[1] == "0m")
          continue;
        ost << " vcoord=" << FieldSpecTranslation__getVcoorFromLevel(vtoken[1])
            << " vlevel=" << vtoken[1];
      } else if (key == "idnum") {
        ost << " ecoord=eps"
            << " elevel=" << vtoken[1];
      } else {
        ost << " " <<tokens[k];
      }
    }
  }

  return ost.str();
}

std::string update1Field(const std::string& in, bool withModel)
{
  return FieldSpecTranslation__getNewFieldString(in, withModel);
}

// this is a modified version of FieldPlotManager::splitDifferenceCommandString
bool splitDifferenceCommandString(std::string& f)
{
  const size_t p1 = f.find(" ( ", 0);
  if (p1 == string::npos)
    return false;

  const size_t p2 = f.find(" - ", p1 + 3);
  if (p2 == string::npos)
    return false;

  const size_t p3 = f.find(" ) ", p2 + 3);
  if (p3 == string::npos)
    return false;

  const std::string common_start = f.substr(0, p1),
      f1 = update1Field(f.substr(p1 + 2, p2 - p1 - 2), true),
      f2 = update1Field(f.substr(p2 + 2, p3 - p2 - 2), true),
      common_end = update1Field(f.substr(p3+2), false);

  f = common_start + " (" + f1 + " -" + f2 + " )" + common_end;
  return true;
}

void updateField(std::string& f)
{
    if (!splitDifferenceCommandString(f))
      f = update1Field(f, true);
}

std::string updateLine(std::string line)
{
  // these were present before diana 3.39
  miutil::replace(line, "_3_farger", "");
  miutil::replace(line, "test.contour.shading=1", "palettecolours=standard");
  miutil::replace(line, " test.contour.shading=0", "");

  if (miutil::contains(line, "st.nr("))
    miutil::replace(line, "st.nr", "st.no");
  else if (miutil::contains(line, "st.nr"))
    miutil::replace(line, "st.nr","st.no(5)");

  miutil::replace(line, "(74,504)", "(uk)");
  miutil::replace(line, "(74,533)", "(uk)");
  miutil::replace(line, "(74,604)", "(uk)");

  miutil::replace(line, "font=Helvetica","font=BITMAPFONT");

  // these have been added in diana 3.39
  miutil::replace(line, "modell/sat-omr\xE5""de", "model/sat-area");
  miutil::replace(line, "AREA areaname=", "AREA name=");
  if (diutil::startswith(line, "FIELD") && !miutil::contains(line, "model="))
    updateField(line);

  return line;
}
} // namespace

bool updateCommandSyntax(std::vector<std::string>& lines)
{
  std::vector<std::string> updated;

  static const boost::regex RE_MAP_AREA("(MAP.*) area=([^ ]+)(.*)", boost::regex::icase);
  for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
    std::string uline = updateLine(*it);

    boost::smatch what;
    if (boost::regex_match(uline, what, RE_MAP_AREA)) {
      if (updated.empty())
        updated.insert(updated.end(), lines.begin(), it);
      updated.push_back("AREA name=" + what.str(2));
      uline = what.str(1) + what.str(3);
    }

    if (uline != *it || !updated.empty()) {
      if (updated.empty())
        updated.insert(updated.end(), lines.begin(), it);
      updated.push_back(uline);
    }
  }

  if (updated.empty()) {
    return false;
  } else {
    std::swap(lines, updated);
    return true;
  }
}
