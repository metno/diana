/*
 Diana - A Free Meteorological Visualisation Tool

 $Id: diPlotOptions.cc 369 2007-11-02 08:55:24Z lisbethb $

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

#include <diFieldPlotManager.h>
#include <diPlotOptions.h>
#include <diField/FieldSpecTranslation.h>
#include <diField/diFieldFunctions.h>
#include <puTools/miSetupParser.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.FieldPlotManager"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

FieldPlotManager::FieldPlotManager(FieldManager* fm) :
      fieldManager(fm)
{
}

void FieldPlotManager::getAllFieldNames(vector<std::string>& fieldNames)
{

  for (unsigned int i = 0; i < vPlotField.size(); i++) {
    fieldNames.push_back(vPlotField[i].name);
  }

}

bool FieldPlotManager::parseSetup()
{

  if ( !parseFieldPlotSetup() ) {
    return false;
  }
  if ( !parseFieldGroupSetup() ) {
    return false;
  }
  return true;
}

bool FieldPlotManager::parseFieldPlotSetup()
{

  METLIBS_LOG_DEBUG("bool FieldPlotManager::parseSetup");

  std::string sect_name = "FIELD_PLOT";
  vector<std::string> lines;

  if (!SetupParser::getSection(sect_name, lines)) {
    METLIBS_LOG_ERROR(sect_name << " section not found");
    return true;
  }

  vPlotField.clear();
  const std::string key_loop = "loop";
  const std::string key_field = "field";
  const std::string key_endfield = "end.field";
  const std::string key_fieldgroup = "fieldgroup";
  const std::string key_plot = "plot";
  const std::string key_plottype = "plottype";
  const std::string key_vcoord = "vcoord";

  // parse setup

  int nlines = lines.size();

  vector<std::string> vstr;
  std::string key, str, str2, option;
  vector<std::string> vpar;

  vector<std::string> loopname;
  vector<vector<std::string> > loopvars;
  map<std::string,int>::const_iterator pfp;
  int firstLine = 0,lastLine,nv;
  bool waiting= true;

  for (int l = 0; l < nlines; l++) {

    vstr = splitComStr(lines[l], true);
    int n = vstr.size();
    key = miutil::to_lower(vstr[0]);

    if (waiting) {
      if (key == key_loop && n >= 4) {
        vpar.clear();
        for (unsigned int i = 3; i < vstr.size(); i++) {
          if (vstr[i][0] == '"') {
            vpar.push_back(vstr[i].substr(1, vstr[i].length() - 2));
          } else {
            vpar.push_back(vstr[i]);
          }
        }
        loopname.push_back(vstr[1]);
        loopvars.push_back(vpar);
      } else if (key == key_field) {
        firstLine = l;
        waiting = false;
      }
    } else if (key == key_endfield) {
      lastLine = l;

      unsigned int nl = loopname.size();
      if (nl > loopvars.size()) {
        nl = loopvars.size();
      }
      unsigned int ml = 1;
      if (nl > 0) {
        ml = loopvars[0].size();
        for (unsigned int il = 0; il < nl; il++) {
          if (ml > loopvars[il].size()) {
            ml = loopvars[il].size();
          }
        }
      }

      for (unsigned int m = 0; m < ml; m++) {
        std::string name;
        std::string fieldgroup;
        vector<std::string> input;
        set<std::string> vcoord;

        for (int i = firstLine; i < lastLine; i++) {
          str = lines[i];
          for (unsigned int il = 0; il < nl; il++) {
            miutil::replace(str, loopname[il], loopvars[il][m]);
          }
          if (i == firstLine) {
            // resplit to keep names with ()
            vstr = miutil::split(str, "=", false);
            if (vstr.size() < 2) {
              std::string errm = "Missing field name";
              SetupParser::errorMsg(sect_name, i, errm);
              continue;
            }
            name = vstr[1];
            if (name[0] == '"' && name[name.length() - 1] == '"') {
              name = name.substr(1, name.length() - 2);
            }
          } else {
            vstr = splitComStr(str, false);
            nv = vstr.size();
            int j = 0;
            while (j < nv - 2) {
              key = miutil::to_lower(vstr[j]);
              if (key == key_plot && vstr[j + 1] == "=" && j < nv - 3) {
                option = key_plottype + "=" + vstr[j + 2];
                if (!PlotOptions::updateFieldPlotOptions(name, option)) {
                  std::string errm = "|Unknown fieldplottype in plotcommand";
                  SetupParser::errorMsg(sect_name, i, errm);
                  break;
                }
                str2 = vstr[j + 3].substr(1, vstr[j + 3].length()
                    - 2);
                input = miutil::split(str2, ",", true);
                if (input.size() < 1 || input.size() > 5) {
                  std::string errm = "Bad specification of plot arguments";
                  SetupParser::errorMsg(sect_name, i, errm);
                  break;
                }

                option = "dim=" + miutil::from_number(int(input.size()));

                if (!PlotOptions::PlotOptions::updateFieldPlotOptions(name, option)){
                  std::string errm = "|Unknown fieldplottype in plotcommand";
                                    SetupParser::errorMsg(sect_name, i, errm);
                                    break;
                }
              } else if (key == key_fieldgroup && vstr[j + 1] == "=") {
                fieldgroup = vstr[j + 2];
                if (fieldgroup[0] == '"' && fieldgroup[fieldgroup.length() - 1]
                                                       == '"') {
                  fieldgroup = fieldgroup.substr(1, fieldgroup.length() - 2);
                }
              } else if (key == key_vcoord && vstr[j + 1] == "=") {
                vector<std::string> vcoordTokens = miutil::split(vstr[j+2], ",");
                for( size_t ii=0; ii<vcoordTokens.size(); ++ii ) {
                  vcoord.insert(vcoordTokens[ii]);
                }
              } else if (vstr[j + 1] == "=") {
                // this should be a plot option
                option = vstr[j] + "=" + vstr[j + 2];

                if (!PlotOptions::updateFieldPlotOptions(name, option)) {
                  std::string errm =
                      "Something wrong in plotoption specifications";
                  SetupParser::errorMsg(sect_name, i, errm);
                  break;
                }
              } else {
                std::string errm = "Unknown keyword in field specifications: "
                    + vstr[0];
                SetupParser::errorMsg(sect_name, i, errm);
                break;
                //j-=2;
              }
              j += 3;
            }
          }
        }

        if (!name.empty() && !input.empty()) {
            PlotField pf;
            pf.name = name;
            pf.fieldgroup = fieldgroup;
            pf.input = input;
            pf.vcoord = vcoord;
            vPlotField.push_back(pf);
        }
      }

      loopname.clear();
      loopvars.clear();
      waiting = true;
    }
  }

  return true;
}

bool FieldPlotManager::parseFieldGroupSetup()
{

  std::string sect_name = "FIELD_GROUPS";
  vector<std::string> lines;

  if (!SetupParser::getSection(sect_name, lines)) {
    METLIBS_LOG_ERROR(sect_name << " section not found");
    return true;
  }

  const std::string key_name = "name";
  const std::string key_group = "group";

  int nlines = lines.size();

    for (int l = 0; l < nlines; l++) {
    vector<std::string> tokens= miutil::split_protected(lines[l], '"','"');
    if ( tokens.size()== 2 ) {
      vector<std::string> stokens= miutil::split_protected(tokens[0], '"','"',"=",true);
      if (stokens.size() == 2 && stokens[0] == key_name ){
        std::string name = stokens[1];
        stokens= miutil::split_protected(tokens[1], '"','"',"=",true);
        if (stokens.size() == 2 && stokens[0] == key_group ){
          groupNames[stokens[1]] = name;
        }
      }
    }
  }

  return true;
}

vector<std::string> FieldPlotManager::splitComStr(const std::string& s, bool splitall)
{
  // split commandstring into tokens.
  // split on '=', ',' and multiple blanks, keep blocks within () and ""
  // split on ',' only if <splitall> is true

  vector<std::string> tmp;

  int i = 0, j = 0, n = s.size();
  if (n) {
    while (i < n && s[i] == ' ') {
      i++;
    }
    j = i;
    for (; i < n; i++) {
      if (s[i] == '=') { // split on '=', but keep it
        if (i - j > 0) {
          tmp.push_back(s.substr(j, i - j));
        }
        tmp.push_back("=");
        j = i + 1;
      } else if (s[i] == ',' && splitall) { // split on ','
        if (i - j > 0) {
          tmp.push_back(s.substr(j, i - j));
        }
        j = i + 1;
      } else if (s[i] == '(') { // keep () blocks
        if (i - j > 0) {
          tmp.push_back(s.substr(j, i - j));
        }
        j = i;
        i++;
        while (i < n && s[i] != ')') {
          i++;
        }
        tmp.push_back(s.substr(j, (i < n) ? (i - j + 1) : (i - j)));
        j = i + 1;
      } else if (s[i] == '"') { // keep "" blocks
        if (i - j > 0) {
          tmp.push_back(s.substr(j, i - j));
        }
        j = i;
        i++;
        while (i < n && s[i] != '"') {
          i++;
        }
        tmp.push_back(s.substr(j, (i < n) ? (i - j + 1) : (i - j)));
        j = i + 1;
      } else if (s[i] == ' ') { // split on (multiple) blanks
        if (i - j > 0) {
          tmp.push_back(s.substr(j, i - j));
        }
        while (i < n && s[i] == ' ') {
          i++;
        }
        j = i;
        i--;
      } else if (i == n - 1) {
        tmp.push_back(s.substr(j, i - j + 1));
      }
    }
  }

  return tmp;
}

vector<std::string> FieldPlotManager::getFields()
{

  set<std::string> paramSet;
  for (unsigned int i = 0; i < vPlotField.size(); i++) {
    for (unsigned int j = 0; j < vPlotField[i].input.size(); j++) {
      //remove extra info like ":standard_name"
      std::string input = vPlotField[i].input[j];
      vector<std::string> vstr = miutil::split(input,":");
      paramSet.insert(vstr[0]);
      paramSet.insert(vstr[0]);
    }
  }

  vector<std::string> param;
  set<std::string>::iterator p = paramSet.begin();
  for (; p != paramSet.end(); p++) {
    param.push_back(*p);
  }

  return param;

}

vector<miTime> FieldPlotManager::getFieldTime(const vector<string>& pinfos,
    bool updateSources)
{
  METLIBS_LOG_SCOPE();
  vector<miTime> fieldtime;

  int numf = pinfos.size();

  vector<FieldRequest> request;
  std::string modelName, modelName2, fieldName;

  for (int i = 0; i < numf; i++) {

    // if difference, use first field
    std::string fspec1,fspec2;
    if (!splitDifferenceCommandString(pinfos[i],fspec1,fspec2)) {
      fspec1 = pinfos[i];
    }

    vector<FieldRequest> fieldrequest;
    std::string plotName;
    parsePin(fspec1, fieldrequest,plotName);
    request.insert(request.begin(),fieldrequest.begin(),fieldrequest.end());

  }


  if (request.size() == 0) {
    return fieldtime;
  }

  return getFieldTime(request, updateSources);
}

void FieldPlotManager::getCapabilitiesTime(vector<miTime>& normalTimes,
    int& timediff, const std::string& pinfo, bool updateSources)
{
  //Finding times from pinfo
  //TODO: find const time

  METLIBS_LOG_INFO(" getCapabilitiesTime: "<<pinfo);
  vector<string> pinfos;
  pinfos.push_back(pinfo);

  //finding timediff
  timediff = 0;
  vector<std::string> tokens = miutil::split_protected(pinfo, '"', '"');
  for (unsigned int j = 0; j < tokens.size(); j++) {
    vector<std::string> stokens = miutil::split(tokens[j], "=");
    if (stokens.size() == 2 && miutil::to_lower(stokens[0]) == "ignore_times" && miutil::to_lower(stokens[1]) == "true") {
      normalTimes.clear();
      return;
    }
    if (stokens.size() == 2 && miutil::to_lower(stokens[0]) == "timediff") {
      timediff = miutil::to_int(stokens[1]);
    }
  }

  //getting times
  normalTimes = getFieldTime(pinfos, updateSources);

  METLIBS_LOG_DEBUG("FieldPlotManager::getCapabilitiesTime: no. of times"<<normalTimes.size());
}

vector<std::string> FieldPlotManager::getFieldLevels(const std::string& pinfo)
{

  vector<std::string> levels;

  vector<std::string> tokens = miutil::split(pinfo, " ");
  if (tokens.size() < 3 || tokens[0] != "FIELD") {
    return levels;
  }

  vector<FieldGroupInfo> vfgi;
  std::string refTime;
  getFieldGroups(tokens[1], refTime, true, vfgi);
  for (unsigned int i = 0; i < vfgi.size(); i++) {
    levels.push_back(vfgi[i].groupName);
    int k = 0;
    int n = vfgi[i].fieldNames.size();
    while (k < n && vfgi[i].fieldNames[k] != tokens[2]) {
      k++;
    }
    if (k < n) {
      //      levels.push_back("Levelgroup:" + vfgi[i].groupName);
      for (unsigned int j = 0; j < vfgi[i].levelNames.size(); j++) {
        levels.push_back(vfgi[i].levelNames[j]);
      }
    }
  }

  return levels;

}

vector<miTime> FieldPlotManager::getFieldTime(
    vector<FieldRequest>& request, bool updateSources)

{
  METLIBS_LOG_SCOPE();

  vector<miTime> vtime;
  for (size_t i = 0; i <request.size(); ++i ) {
    if (request[i].plotDefinition ) {
      vector<FieldRequest> fr = getParamNames(request[i].paramName,request[i]);
      if ( fr.size()>0 ) {
        request[i].paramName = fr[0].paramName;
        request[i].standard_name = fr[0].standard_name;
      }
    }
    flightlevel2pressure(request[i]);
  }

  return fieldManager->getFieldTime(request, updateSources);
}

bool FieldPlotManager::addGridCollection(const std::string fileType,
    const std::string& modelName,
    const std::vector<std::string>& filenames,
    const std::vector<std::string>& format,
    std::vector<std::string> config,
    const std::vector<std::string>& option)
{


  return fieldManager->addGridCollection(fileType, modelName, filenames,
      format,config, option);

}


bool FieldPlotManager::makeFields(const std::string& pin_const,
    const miTime& const_ptime, vector<Field*>& vfout, bool toCache)
{
  METLIBS_LOG_SCOPE(LOGVAL(pin_const));

  // if difference
  std::string fspec1,fspec2;
  if (splitDifferenceCommandString(pin_const,fspec1,fspec2)) {
    return makeDifferenceField(fspec1, fspec2, const_ptime, vfout);
  }

  vfout.clear();

  vector<FieldRequest> vfieldrequest;
  std::string plotName;
  std::string pin = pin_const;
  parsePin(pin, vfieldrequest, plotName);



  bool ok = false;
  for (unsigned int i = 0; i < vfieldrequest.size(); i++) {

    if (vfieldrequest[i].ptime.undef()) {
      vfieldrequest[i].ptime = const_ptime;
    }

    if (vfieldrequest[i].hourOffset != 0 || vfieldrequest[i].minOffset != 0) {
      vfieldrequest[i].ptime.addHour(vfieldrequest[i].hourOffset);
      vfieldrequest[i].ptime.addMin(vfieldrequest[i].minOffset);
    }
    Field* fout = NULL;
    // we must try to use the cache, if specified...
    int cacheoptions = FieldManager::READ_ALL;
    if (toCache) {
      cacheoptions = cacheoptions | FieldManager::WRITE_ALL;
    }
    //    ok = fieldManager->makeField(fout, modelName, fieldName[i], ptime,
    //        levelName, idnumName, hourDiff, cacheoptions);

    ok = fieldManager->makeField(fout, vfieldrequest[i],cacheoptions);
    if (!ok) {
      return false;
    }

    makeFieldText(fout, plotName, vfieldrequest[i].flightlevel);
    vfout.push_back(fout);

  }

  return true;

}

void FieldPlotManager::makeFieldText(Field* fout, const std::string& plotName, bool flightlevel)
{

  std::string fieldtext = fout->modelName + " " + plotName;
  if (!fout->leveltext.empty()) {
    if ( flightlevel ) {
      fieldtext += " " + FieldFunctions::pLevel2flightLevel[fout->leveltext];
    } else {
      fieldtext += " " + fout->leveltext;
    }
  }
  if (!fout->idnumtext.empty()) {
    fieldtext += " " + fout->idnumtext;
  }

  if( !fout->analysisTime.undef() && !fout->validFieldTime.undef()) {
    fout->forecastHour = miutil::miTime::hourDiff(fout->validFieldTime, fout->analysisTime);
  }

  std::string progtext;
  if( !fout->analysisTime.undef() && fout->forecastHour != -32767 ) {
    ostringstream ostr;
    ostr.width(2);
    ostr.fill('0');
    ostr << fout->analysisTime.hour()<<" ";
    if (fout->forecastHour >= 0) {
      ostr << "+" << fout->forecastHour;
    } else {
      ostr << fout->forecastHour;
    }
    progtext = "(" + ostr.str() + ")";
  }

  std::string timetext;
  if( !fout->validFieldTime.undef() ) {
    std::string sclock = fout->validFieldTime.isoClock();
    std::string shour = sclock.substr(0, 2);
    std::string smin = sclock.substr(3, 2);
    if (smin == "00") {
      timetext = fout->validFieldTime.isoDate() + " " + shour + " UTC";
    } else {
      timetext = fout->validFieldTime.isoDate() + " " + shour + ":" + smin
          + " UTC";
    }
  }

  fout->name = plotName;
  fout->text = fieldtext + " " + progtext;
  fout->fulltext = fieldtext + " " + progtext + " " + timetext;
  fout->fieldText = fieldtext;
  fout->progtext = progtext;
  fout->timetext = timetext;
}

bool FieldPlotManager::makeDifferenceField(const std::string& fspec1,
    const std::string& fspec2, const miTime& const_ptime, vector<Field*>& fv)
{

  fv.clear();
  vector<Field*> fv1;
  vector<Field*> fv2;

  if (makeFields(fspec1, const_ptime, fv1)) {
    if (!makeFields(fspec2, const_ptime, fv2)) {

      for (unsigned int i = 0; i < fv1.size(); i++) {
        fieldManager->fieldcache->freeField(fv1[i]);
        fv1[i] = NULL;
      }
      return false;
    }
  } else {
    return false;
  }

  //make copy of fields, do not change the field cache
  for (unsigned int i = 0; i < fv1.size(); i++) {
    Field* ff = new Field();
    fv.push_back(ff);
    *fv[i] = *fv1[i];
  }

  for (unsigned int i = 0; i < fv1.size(); i++) {
    fieldManager->fieldcache->freeField(fv1[i]);
    fv1[i] = NULL;
  }

  //make Difference Field text
  Field* f1 = fv[0];
  Field* f2 = fv2[0];

  const int mdiff = 6;
  std::string text1[mdiff], text2[mdiff];
  bool diff[mdiff];
  text1[0] = f1->modelName;
  text1[1] = f1->name;
  text1[2] = f1->leveltext;
  text1[3] = f1->idnumtext;
  text1[4] = f1->progtext;
  text1[5] = f1->timetext;
  text2[0] = f2->modelName;
  text2[1] = f2->name;
  text2[2] = f2->leveltext;
  text2[3] = f2->idnumtext;
  text2[4] = f2->progtext;
  text2[5] = f2->timetext;
  int nbgn = -1;
  int nend = 0;
  int ndiff = 0;
  for (int n = 0; n < mdiff; n++) {
    if (text1[n] != text2[n]) {
      diff[n] = true;
      if (nbgn < 0) {
        nbgn = n;
      }
      nend = n;
      ndiff++;
    } else {
      diff[n] = false;
    }
  }

  if (ndiff == 0) {
    // may happen due to level/idnum up/down change or equal difference,
    // make an explaining text
    if (!f1->leveltext.empty()) {
      diff[2] = true;
    } else if (!f1->idnumtext.empty()) {
      diff[3] = true;
    } else {
      diff[1] = true;
    }
    ndiff = 1;
  }

  if (diff[0]) {
    f1->modelName = "( " + text1[0] + " - " + text2[0] + " )";
  }
  if (diff[1] && (diff[2] || diff[3])) {
    f1->name = "( " + text1[1];
    if (!text1[2].empty()) {
      f1->name += " " + text1[2];
    }
    if (!text1[3].empty()) {
      f1->name += " " + text1[3];
    }
    f1->name += " - " + text2[1];
    if (!text2[2].empty()) {
      f1->name += " " + text2[2];
    }
    if (!text2[3].empty()) {
      f1->name += " " + text2[3];
    }
    f1->name += " )";
    f1->leveltext.clear();
    if (diff[2] && diff[3]) {
      ndiff -= 2;
    } else {
      ndiff--;
    }
  } else {
    if (diff[1]) {
      f1->name = "( " + text1[1] + " - " + text2[1] + " )";
    }
    if (diff[2]) {
      f1->leveltext = "( " + text1[2] + " - " + text2[2] + " )";
    }
    if (diff[3]) {
      f1->idnumtext = "( " + text1[3] + " - " + text2[3] + " )";
    }
  }
  if (diff[4]) {
    f1->progtext = "( " + text1[4] + " - " + text2[4] + " )";
  }
  if (diff[5]) {
    f1->timetext = "( " + text1[5] + " - " + text2[5] + " )";
  }
  if (ndiff == 1) {
    f1->fieldText = f1->modelName + " " + f1->name;
    if (!f1->leveltext.empty()) {
      f1->fieldText += " " + f1->leveltext;
    }
    f1->text = f1->fieldText + " " + f1->progtext;
    f1->fulltext = f1->text + " " + f1->timetext;
  } else {
    if (nbgn == 1 && nend <= 3) {
      if (!text1[2].empty()) {
        text1[1] += " " + text1[2];
      }
      if (!text1[3].empty()) {
        text1[1] += " " + text1[3];
      }
      if (!text2[2].empty()) {
        text2[1] += " " + text2[2];
      }
      if (!text2[3].empty()) {
        text2[1] += " " + text2[3];
      }
      text1[2].clear();
      text1[3].clear();
      text2[2].clear();
      text2[3].clear();
      nend = 1;
    }
    int nmax[3] =
    { 5, 4, 3 };
    std::string ftext[3];
    for (int t = 0; t < 3; t++) {
      if (nbgn > nmax[t]) {
        nbgn = nmax[t];
      }
      if (nend > nmax[t]) {
        nend = nmax[t];
      }
      bool first = true;
      for (int n = 0; n < nbgn; n++) {
        if (first) {
          ftext[t] = text1[n];
        } else {
          ftext[t] += " " + text1[n];
        }
        first = false;
      }
      if (first) {
        ftext[t] = "(";
      } else {
        ftext[t] += " (";
      }
      for (int n = nbgn; n <= nend; n++) {
        if (!text1[n].empty()) {
          ftext[t] += " " + text1[n];
        }
      }
      ftext[t] += " -";
      for (int n = nbgn; n <= nend; n++) {
        if (!text2[n].empty()) {
          ftext[t] += " " + text2[n];
        }
      }
      ftext[t] += " )";
      for (int n = nend + 1; n <= nmax[t]; n++) {
        if (!text1[n].empty()) {
          ftext[t] += " " + text1[n];
        }
      }
    }
    f1->fulltext = ftext[0];
    f1->text = ftext[1];
    f1->fieldText = ftext[2];
  }

  METLIBS_LOG_DEBUG("F1-F2: validFieldTime: "<<f1->validFieldTime);
  METLIBS_LOG_DEBUG("F1-F2: analysisTime:   "<<f1->analysisTime);
  METLIBS_LOG_DEBUG("F1-F2: name:           "<<f1->name);
  METLIBS_LOG_DEBUG("F1-F2: text:           "<<f1->text);
  METLIBS_LOG_DEBUG("F1-F2: fulltext:       "<<f1->fulltext);
  METLIBS_LOG_DEBUG("F1-F2: modelName:      "<<f1->modelName);
  METLIBS_LOG_DEBUG("F1-F2: fieldText:      "<<f1->fieldText);
  METLIBS_LOG_DEBUG("F1-F2: leveltext:      "<<f1->leveltext);
  METLIBS_LOG_DEBUG("F1-F2: idnumtext:      "<<f1->idnumtext);
  METLIBS_LOG_DEBUG("F1-F2: progtext:       "<<f1->progtext);
  METLIBS_LOG_DEBUG("F1-F2: timetext:       "<<f1->timetext);
  METLIBS_LOG_DEBUG("-----------------------------------------------------");
  bool ok = fieldManager->makeDifferenceFields(fv, fv2);
  if (!ok) {
    return false;
  }
  return true;

}

void FieldPlotManager::getFieldGroups(const std::string& modelName, std::string refTime, bool plotGroups, vector<FieldGroupInfo>& vfgi)
{
  //METLIBS_LOG_DEBUG(__FUNCTION__);

  fieldManager->getFieldGroups(modelName, refTime, vfgi);

  //Return vfgi whith parameter names from file + computed parameters
  if(!plotGroups) {
    return;
  }

  //replace parameters in vfgi with plots defined in setup
  size_t nvfgi = vfgi.size();

  //replace fieldnames with plotnames
  for (size_t i = 0; i < nvfgi; i++) {

    std::map<std::string, std::vector<std::string> > plotNameLevels;

    std::string zaxis = vfgi[i].zaxis;
    if( zaxis.empty() ) {
      zaxis = "none";
    }
//    //Make copy with field names from file
//    if(fieldManager->isGridCollection(modelName) && !plotGroups) {
//      vfgi.push_back(vfgi[i]);
//      vfgi[vfgi.size()-1].plotDefinitions = false;
//    }

    //use groupname from setup if defined
    if ( groupNames.count(vfgi[i].groupName)) {
      vfgi[i].groupName = groupNames[vfgi[i].groupName];
    }

    //sort fieldnames
    set<std::string> set_fieldName;
    set<std::string> set_standardName;
    for (unsigned int l = 0; l < vfgi[i].fieldNames.size(); l++) {
      set_fieldName.insert(vfgi[i].fieldNames[l]);
    }

    for (unsigned int l = 0; l < vfgi[i].standard_names.size(); l++) {
      set_standardName.insert(vfgi[i].standard_names[l]);
    }

    //find plotNames
    vector<std::string> plotNames;
    for (unsigned int j = 0; j < vPlotField.size(); j++) {
        std::string plotName = vPlotField[j].name;
        int ninput = 0; // number of input fields found
        if (vPlotField[j].vcoord.size() > 0 && !vPlotField[j].vcoord.count(zaxis)  ) {
          continue;
        }
        std::vector<std::string> levels;
        for (unsigned int k = 0; k < vPlotField[j].input.size(); k++) {
          std::string fieldName = vPlotField[j].input[k];
          vector<std::string> vstr = miutil::split(fieldName,":");
          fieldName = vstr[0];
          bool standard_name = false;
          if (vstr.size() == 2 && vstr[1] == "standard_name" ){
            standard_name = true;
            if ( !set_standardName.count(fieldName) ) {
              break;
            }
            levels = vfgi[i].levelNames;
            ninput++;
          } else {
            if (!set_fieldName.count(fieldName)) {
              break;
            }
            if ( vfgi[i].levels.count(fieldName) ) {
              levels = vfgi[i].levels[fieldName];
            }
            ninput++;
          }
        }

        if (ninput >= vPlotField[j].input.size()) {
          plotNames.push_back(plotName);
          plotNameLevels[plotName] = levels;
        }

      }

    vfgi[i].fieldNames = plotNames;
    vfgi[i].levels = plotNameLevels;

  }

  //remove fieldgroups with no plots
  vector<FieldGroupInfo>::iterator p = vfgi.begin();
  for (; p != vfgi.end(); p++) {
    if ( (*p).fieldNames.size() == 0 ) {
      p = vfgi.erase(p);
      if (p == vfgi.end())
        break;
    }
  }

}

std::string FieldPlotManager::getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour)
{
  return fieldManager->getBestReferenceTime(model, refOffset, refHour);
}

gridinventory::Grid FieldPlotManager::getFieldGrid(const std::string& model)
{
  return fieldManager->getGrid(model);
}

void FieldPlotManager::parseString( std::string& pin,
    FieldRequest& fieldrequest,
    vector<std::string>& paramNames,
    std::string& plotName )
{

  METLIBS_LOG_DEBUG("parseString PIN: "<<pin);


  std::vector<std::string> tokens;
  //NB! what about ""
  boost::algorithm::split(tokens, pin, boost::algorithm::is_space());
  size_t n = tokens.size();
  std::string str, key;


  for (size_t k = 1; k < n; k++) {
    std::vector<std::string> vtoken;
    boost::algorithm::split(vtoken, tokens[k], boost::algorithm::is_any_of(std::string("=")));
    if (vtoken.size() >= 2) {
      key = boost::algorithm::to_lower_copy(vtoken[0]);
      if (key == "model") {
        fieldrequest.modelName = vtoken[1];
      }else if (key == "parameter") {
        paramNames.push_back(vtoken[1]);
        fieldrequest.plotDefinition=false;
      }else if (key == "plot") {
        plotName = vtoken[1];
        fieldrequest.plotDefinition=true;
      } else if (key == "vcoord") {
        fieldrequest.zaxis = vtoken[1];
      } else if (key == "tcoor") {
        fieldrequest.taxis = vtoken[1];
      } else if (key == "ecoord") {
        fieldrequest.eaxis = vtoken[1];
      } else if (key == "vlevel") {
        fieldrequest.plevel = vtoken[1];
      } else if (key == "elevel") {
        fieldrequest.elevel = vtoken[1];
      } else if (key == "grid") {
        fieldrequest.grid = vtoken[1];
      } else if (key == "unit") {
        fieldrequest.unit = vtoken[1];
      } else if (key == "vunit" && vtoken[1] == "FL") {
        fieldrequest.flightlevel=true;
      } else if (key == "time") {
        fieldrequest.ptime = miTime(vtoken[1]);
      } else if (key == "reftime") {
        fieldrequest.refTime = vtoken[1];
      } else if (key == "refhour") {
        fieldrequest.refhour = atoi(vtoken[1].c_str());
      } else if (key == "refoffset") {
        fieldrequest.refoffset = atoi(vtoken[1].c_str());
      } else if (key == "hour.offset") {
        fieldrequest.hourOffset = atoi(vtoken[1].c_str());
      } else if (key == "min.offset") {
        fieldrequest.minOffset = atoi(vtoken[1].c_str());
      } else if (key == "hour.diff") {
        fieldrequest.time_tolerance = atoi(vtoken[1].c_str()) * 60; //time_tolerance in minutes, hour.diff in hours
      } else if (key == "alltimesteps") {
        if (vtoken[1] == "1" || vtoken[1] == "on" || vtoken[1] == "true") {
          fieldrequest.allTimeSteps = true;
        }
      } else if (key == "file.palette") {
        fieldrequest.palette = vtoken[1];
      }
    }
  }

  flightlevel2pressure(fieldrequest);
  METLIBS_LOG_DEBUG(LOGVAL(fieldrequest.zaxis) << LOGVAL(fieldrequest.plevel));
}

void FieldPlotManager::flightlevel2pressure(FieldRequest& frq)
{
  if ( frq.zaxis == "flightlevel") {
    frq.zaxis = "pressure";
    frq.flightlevel=true;
    if ( miutil::contains(frq.plevel,"FL") ) {
      frq.plevel = FieldFunctions::getPressureLevel(frq.plevel);
    }
  }
}

bool FieldPlotManager::parsePin( std::string& pin, vector<FieldRequest>& vfieldrequest, std::string& plotName)
{

  METLIBS_LOG_DEBUG("parsePin - PIN: "<<pin);

  // if difference
  std::string fspec1,fspec2;
  if (splitDifferenceCommandString(pin,fspec1,fspec2)) {
    return parsePin(fspec1, vfieldrequest, plotName);
  }

  std::string  origPin = pin;
  bool oldSyntax = (pin.find("model=") == std::string::npos);
  if ( oldSyntax ) {
    pin = FieldSpecTranslation::getNewFieldString(pin);
  }
//  METLIBS_LOG_DEBUG("PIN NEW: "<<pin);

  FieldRequest fieldrequest;
  vector<std::string> paramNames;
  parseString(pin, fieldrequest, paramNames, plotName);

  // Try to parse old syntax once more, parse modelName
  if ( oldSyntax && !fieldManager->modelOK(fieldrequest.modelName) ) {
    METLIBS_LOG_WARN("Old syntax: try to split modelName and refhour from modelName: "<<fieldrequest.modelName);
    pin = FieldSpecTranslation::getNewFieldString(origPin, true);
    paramNames.clear();
    parseString(pin, fieldrequest, paramNames, plotName);
    METLIBS_LOG_WARN("New modelName: " <<fieldrequest.modelName);
  }

  //  //plotName -> fieldName
  if ( fieldrequest.plotDefinition) {
    vfieldrequest = getParamNames(plotName,fieldrequest);
  } else {
    for ( size_t i=0; i<paramNames.size();i++ ) {
      fieldrequest.paramName = paramNames[i];
      vfieldrequest.push_back(fieldrequest);
    }
  }


  return true;

}

bool FieldPlotManager::writeField(FieldRequest fieldrequest, const Field* field)
{
  return fieldManager->writeField(fieldrequest, field);
}

vector<FieldRequest> FieldPlotManager::getParamNames(const std::string& plotName, FieldRequest fieldrequest)
{

  //search through vPlotField
  //if plotName and vcoord ok -> use it
  //else use fieldname= plotname

  vector<FieldRequest> vfieldrequest;

  for ( size_t i=0; i<vPlotField.size(); ++i ) {
    if ( vPlotField[i].name == plotName && (vPlotField[i].vcoord.size()==0 || vPlotField[i].vcoord.count(fieldrequest.zaxis))) {
      for (size_t j = 0; j < vPlotField[i].input.size(); ++j ) {
        std::string inputName = vPlotField[i].input[j];
        vector<std::string> vstr = miutil::split(inputName, ":");
        inputName = vstr[0];
        if (vstr.size() == 2 && vstr[1] == "standard_name" ){
          fieldrequest.standard_name = true;
        } else {
          fieldrequest.standard_name = false;
        }
        fieldrequest.paramName = inputName;

        vfieldrequest.push_back(fieldrequest);
      }

      return vfieldrequest;

    }

  }

  //If plotName not defined, use plotName as fieldName
  fieldrequest.paramName = plotName;
  vfieldrequest.push_back(fieldrequest);
  return vfieldrequest;
}

bool FieldPlotManager::splitDifferenceCommandString(const std::string& pin, std::string& fspec1, std::string& fspec2)
{
  const size_t p1 = pin.find(" ( ", 0);
  if (p1 == string::npos)
    return false;

  const size_t p2 = pin.find(" - ", p1 + 3);
  if (p2 == string::npos)
    return false;

  const size_t p3 = pin.find(" ) ", p2 + 3);
  if (p3 == string::npos)
    return false;

  const std::string common_start = pin.substr(0, p1),
      common_end = pin.substr(p3+2);
  fspec1 = common_start + pin.substr(p1 + 2, p2 - p1 - 2) + common_end;
  fspec2 = common_start + pin.substr(p2 + 2, p3 - p2 - 2) + common_end;
  return true;
}
