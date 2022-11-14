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

#include "diana_config.h"

#include "diQuickMenues.h"
#include "diLocalSetupParser.h"
#include "util/charsets.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <regex>

#include <fstream>
#include <sstream>

#define MILOGGER_CATEGORY "diana.QuickMenues"
#include <miLogger/miLogging.h>

using namespace::miutil;

namespace { // anonymous

const std::string QM_DYNAMIC_OPTION_PREFIX = "@";

} // anonymous namespace

void replaceDynamicQuickMenuOptions(const std::vector<std::string>& oldCommand, std::vector<std::string>& newCommand)
{
  int nold = oldCommand.size();
  int nnew = newCommand.size();

  for (int i = 0; i < nold && i < nnew; i++) {
    if (not miutil::contains(oldCommand[i], QM_DYNAMIC_OPTION_PREFIX))
      continue;
    std::vector<std::string> token = miutil::split(oldCommand[i], 0, " ");
    int ntoken = token.size();
    for (int j = 0; j < ntoken; j++) {
      if (not miutil::contains(token[j], QM_DYNAMIC_OPTION_PREFIX))
        continue;
      std::vector<std::string> stoken = miutil::split(token[j], 0, "=");
      if (stoken.size() != 2 || not miutil::contains(stoken[1], QM_DYNAMIC_OPTION_PREFIX))
        continue;
      // found item to replace
      std::vector<std::string> newtoken = miutil::split(newCommand[i], 0, " ");
      int nnewtoken = newtoken.size();
      if (nnewtoken < 2 || token[0] != newtoken[0])
        continue;
      for (int k = 1; k < nnewtoken; k++) {
        std::vector<std::string> snewtoken = miutil::split(newtoken[k], "=");
        if (snewtoken.size() == 2 && snewtoken[0] == stoken[0]) {
          miutil::replace(newCommand[i], newtoken[k], token[j]);
        }
      }
    }
  }
}

quickMenu::quickMenu()
    : type(QM_USER)
    , item_index(0)
{
}

quickMenu::~quickMenu()
{
}

bool quickMenu::step_item(int delta)
{
  int pi = item_index + delta;
  if (valid_item() && valid_item(pi)) {
    item_index = pi;
    return true;
  } else {
    return false;
  }
}

const std::vector<std::string>& quickMenu::command() const
{
  static const std::vector<std::string> EMPTY;
  if (valid_item())
    return menuitems[item_index].command;
  else
    return EMPTY;
}

// sort keys by length - make index-list
std::vector<int> quickMenu::sorted_keys() const
{
  std::vector<int> keys;
  for (size_t i = 0; i < opt.size(); i++) {
    const size_t key_length = opt[i].key.length();
    std::vector<int>::iterator it = keys.begin();
    for (; it != keys.end() && key_length < opt[*it].key.length(); it++)
      ;
    keys.insert(it, i);
  }
  return keys;
}

std::set<int> quickMenu::used_options(const std::string& c) const
{
  METLIBS_LOG_SCOPE();
  std::set<int> used;
  std::string ts = c; // make a copy as we need to modify the string
  const std::vector<int> keys = sorted_keys();
  for (size_t i = 0; i < opt.size(); i++) {
    const int ki = keys[i];
    const std::string key = QM_DYNAMIC_OPTION_PREFIX + opt[ki].key;
    if (miutil::contains(ts, key)) {
      used.insert(ki);
      miutil::replace(ts, key, ""); // avoid finding it again with shorter key
    }
  }
  return used;
}

void quickMenu::expand_options(std::vector<std::string>& com) const
{
  for (const int sk : sorted_keys()) {
    const quickMenuOption& o = opt[sk];
    for (std::string& c : com)
      miutil::replace(c, QM_DYNAMIC_OPTION_PREFIX + o.key, o.def);
  }
}

bool quickMenu::write() const
{
  return writeQuickMenu(*this);
}

bool quickMenu::read()
{
  return readQuickMenu(*this);
}

bool writeQuickMenu(const quickMenu& qm)
{
  METLIBS_LOG_SCOPE();

  std::ofstream menufile(qm.filename.c_str());
  if (!menufile){
    METLIBS_LOG_WARN("Cannot write quick-menu file '" << qm.filename << "'");
    return false;
  }

  diutil::CharsetConverter_p converter = diutil::findConverter(diutil::CHARSET_INTERNAL(), diutil::CHARSET_WRITE());

  menufile << "# -*- coding: " << diutil::CHARSET_WRITE() << " -*-" << std::endl;
  menufile << "# Name and plot string of Quick-menu" << std::endl;
  menufile << "# '#' marks comments" << std::endl;
  menufile << "#------------------------------------------------" << std::endl;
  menufile << "#-- Name of quick-menu, given in the dialog" << std::endl;
  menufile << "# \"name\"" << std::endl;
  menufile << "#------------------------------------------------" << std::endl;
  menufile << "#-- Variable definitions" << std::endl;
  menufile << "# [XX=a,b,c,d,..]" << std::endl;
  menufile << "#------------------------------------------------" << std::endl;
  menufile << "#-- Plots" << std::endl;
  menufile << "# '>Plot name' is separator between the different plots" << std::endl;
  menufile << "#------------------------------------------------" << std::endl;
  menufile << std::endl;
  menufile << "# quickmenu name" << std::endl;

  menufile << "\"" << converter->convert(qm.name) << "\"" << std::endl;
  menufile << std::endl;

  menufile << "# variables" << std::endl;

  // write options
  for (const quickMenuOption& mo : qm.opt) {
    menufile << "[" << converter->convert(mo.key) << "=";
    bool first = true;
    for (const std::string& o : mo.options) {
      if (!first)
        menufile << ",";
      first = false;

      menufile << converter->convert(o);
    }
    menufile << std::endl;
  }

  menufile << std::endl;

  // the plots
  for (const quickMenuItem& mi : qm.menuitems) {
    menufile << "#" << std::endl;
    menufile << ">" << converter->convert(mi.name) << std::endl;
    for (const std::string& c : mi.command) {
      menufile << converter->convert(c) << std::endl;
    }
  }

  return true;
}

bool readQuickMenu(quickMenu& qm)
{
  std::ifstream menufile(qm.filename.c_str());
  if (!menufile) { // menufile not ok
    METLIBS_LOG_WARN("Could not open quickmenu file '" << qm.filename << "'");
    return false;
  }

  if (!readQuickMenu(qm, menufile))
    return false;

  menufile.close(); // close before writing

  bool updates = false;
  if (qm.name.empty()) {
    updates = true;
    qm.name = makeQuickMenuName(qm.filename);
  }

  for (quickMenuItem& qmi : qm.menuitems) {
    if (updateCommandSyntax(qmi.command))
      updates = true;
  }

  if (updates)
    qm.write();

  return true;
}

bool readQuickMenu(quickMenu& qm, std::istream& in)
{
  METLIBS_LOG_SCOPE();
  quickMenuItem mi;
  std::string value;

  qm.menuitems.clear();
  qm.opt.clear();
  qm.item_index = 0;

  diutil::GetLineConverter convertline("#");
  std::string line;
  while (convertline(in, line)) {
    miutil::trim(line);
    if (line.empty() || line[0] == '#')
      continue;

    if (line[0] == '"') {
      // name of quickmenu
      qm.name = diutil::start_end_mark_removed(line, '"', '"');

    } else if (line[0]=='['){
      // variable/options
      diutil::remove_start_end_mark(line, '[', ']');
      std::vector<std::string> tokens = miutil::split(line, 1, "=");
      if (tokens.size()>1){
        quickMenuOption op;
        op.key= tokens[0];
        value= tokens[1];
        op.options = miutil::split_protected(value, '"','"',",", false); //Do not skip blank enteries
        op.def= (op.options.size()>0 ? op.options[0] : "");
        // add a new option
        qm.opt.push_back(op);
      } else {
        METLIBS_LOG_ERROR("QuickMenu Error: defined option without items in file " << qm.filename);
        return false;
      }
    } else if (line[0]=='>'){
      // new menuitem
      qm.menuitems.push_back(quickMenuItem());
      qm.menuitems.back().name = line.substr(1, line.length() - 1);
    } else {
      // commands
      if (!qm.menuitems.empty()) {
        qm.menuitems.back().command.push_back(line);
      } else {
        METLIBS_LOG_ERROR("command line without defined menuitem in file '" << qm.filename << "'");
        return false;
      }
    }
  }
  return true;
}

std::string makeQuickMenuName(const std::string& qmfilename)
{
  size_t end = qmfilename.find_last_of(".");
  if (end == std::string::npos)
    end = qmfilename.size();
  size_t start = qmfilename.find_last_of("/", end);
  if (start == std::string::npos || start >= qmfilename.length() - 1)
    start = 0;
  else
    start += 1;
  return qmfilename.substr(start, end - start);
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
  if (p1 == std::string::npos)
    return false;

  const size_t p2 = f.find(" - ", p1 + 3);
  if (p2 == std::string::npos)
    return false;

  const size_t p3 = f.find(" ) ", p2 + 3);
  if (p3 == std::string::npos)
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

  miutil::replace(line, "St.nr(3)", "st.no");
  miutil::replace(line, "St.nr(5)", "st.no");

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

  static const std::regex RE_MAP_AREA("(MAP.*) area=([^ ]+)(.*)", std::regex::icase);
  for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
    std::string uline = updateLine(*it);

    std::smatch what;
    if (std::regex_match(uline, what, RE_MAP_AREA)) {
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

void readQuickMenuLog(std::vector<quickMenu>& qm, const std::vector<std::string>& loglines)
{
  quickMenu* q_name = 0;
  for (const std::string& ll : loglines) {
    const std::string line = miutil::trimmed(ll);
    if (line.empty() or line[0] == '#')
      continue;

    if (diutil::startswith(line, ">name=") || diutil::startswith(line, ">update=")) { // new menu name
      const std::string name = line.substr(line.find("=") + 1);
      q_name = 0;
      for (quickMenu& q : qm) {
        if (q.name == name) {
          q_name = &q;
          break;
        }
      }
    } else if (line[0] == '%') { // dynamic options
      if (q_name && line.length() > 1) {
        const std::vector<std::string> key_val = miutil::split(line.substr(1), "=");
        if (key_val.size() == 2) {
          const std::string &key = key_val[0], &val = key_val[1];
          for (quickMenuOption& o : q_name->opt) {
            if (key == o.key) {
              o.def = val;
              break;
            }
          }
        }
      }
    } else if (line[0] == '=') {
      q_name = 0;
    }
  }
}

std::vector<std::string> writeQuickMenuLog(const std::vector<quickMenu>& qm)
{
  std::vector<std::string> loglines;
  for (const quickMenu& q : qm) {
    if (q.is_history() || q.opt.empty())
      continue;

    // write defaults for dynamic options
    loglines.push_back(">name=" + q.name);
    for (const quickMenuOption& o : q.opt)
      loglines.push_back("%" + o.key + "=" + o.def);
    loglines.push_back("=======================");
  }
  return loglines;
}
