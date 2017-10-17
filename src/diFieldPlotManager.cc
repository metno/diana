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

#include "diFieldPlotManager.h"
#include "diFieldPlot.h"
#include "diPlotOptions.h"
#include "diKVListPlotCommand.h"
#include "miSetupParser.h"
#include "util/string_util.h"

#include "diField/diFieldFunctions.h"
#include "diField/diFlightLevel.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string.hpp>

#include <iomanip>
#include <memory>
#include <sstream>

#define MILOGGER_CATEGORY "diana.FieldPlotManager"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

FieldPlotManager::FieldPlotManager(FieldManager* fm) :
      fieldManager(fm)
{
}

FieldPlot* FieldPlotManager::createPlot(const PlotCommand_cp& pc)
{
  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pc);
  if (!cmd)
    return 0;
  const std::string plotName = extractPlotName(cmd->all());
  std::unique_ptr<FieldPlot> fp(new FieldPlot(this));
  if (fp->prepare(plotName, cmd))
    return fp.release();
  else
    return 0;
}

void FieldPlotManager::getAllFieldNames(vector<std::string>& fieldNames)
{
  for (unsigned int i = 0; i < vPlotField.size(); i++) {
    fieldNames.push_back(vPlotField[i].name);
  }
}

bool FieldPlotManager::parseSetup()
{
  if (!parseFieldPlotSetup())
    return false;
  if (!parseFieldGroupSetup())
    return false;
  return true;
}

bool FieldPlotManager::parseFieldPlotSetup()
{
  METLIBS_LOG_SCOPE();

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
  const std::string key_vc_type = "vc_type";

  // parse setup

  int nlines = lines.size();

  vector<std::string> vstr;
  std::string key, str;
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
        std::string inputstr;
        set<std::string> vcoord;
        FieldFunctions::VerticalType vctype = FieldFunctions::vctype_none;

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
                const miutil::KeyValue_v option1(1, miutil::KeyValue(key_plottype, vstr[j + 2]));
                if (!updateFieldPlotOptions(name, option1)) {
                  std::string errm = "|Unknown fieldplottype in plotcommand";
                  SetupParser::errorMsg(sect_name, i, errm);
                  break;
                }
                inputstr = vstr[j + 3].substr(1, vstr[j + 3].length() - 2);
                input = miutil::split(inputstr, ",", true);
                if (input.size() < 1 || input.size() > 5) {
                  std::string errm = "Bad specification of plot arguments";
                  SetupParser::errorMsg(sect_name, i, errm);
                  break;
                }

                const miutil::KeyValue_v option2(1, miutil::KeyValue("dim", miutil::from_number(int(input.size()))));
                if (!updateFieldPlotOptions(name, option2)) {
                  std::string errm = "|Unknown fieldplottype in plotcommand";
                  SetupParser::errorMsg(sect_name, i, errm);
                  break;
                }
              } else if (key == key_fieldgroup && vstr[j + 1] == "=") {
                fieldgroup = vstr[j + 2];
                if (fieldgroup[0] == '"' && fieldgroup[fieldgroup.length() - 1] == '"') {
                  fieldgroup = fieldgroup.substr(1, fieldgroup.length() - 2);
                }
              } else if (key == key_vcoord && vstr[j + 1] == "=") {
                vector<std::string> vcoordTokens = miutil::split(vstr[j+2], ",");
                for( size_t ii=0; ii<vcoordTokens.size(); ++ii ) {
                  vcoord.insert(vcoordTokens[ii]);
                }
              } else if (key == key_vc_type && vstr[j + 1] == "=") {
                vctype = FieldFunctions::getVerticalType(vstr[j+2]);
              } else if (vstr[j + 1] == "=") {
                // this should be a plot option
                const miutil::KeyValue_v option1(1, miutil::KeyValue(vstr[j], vstr[j + 2]));
                if (!updateFieldPlotOptions(name, option1)) {
                  SetupParser::errorMsg(sect_name, i, "Something wrong in plotoption specifications");
                  break;
                }
              } else {
                std::string errm = "Unknown keyword in field specifications: " + vstr[0];
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
            pf.inputstr = inputstr;
            pf.vcoord = vcoord;
            pf.vctype = vctype;
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
      const std::string& input = vPlotField[i].input[j];
      vector<std::string> vstr = miutil::split(input,":");
      paramSet.insert(vstr[0]);
    }
  }

  return vector<std::string>(paramSet.begin(), paramSet.end());
}

vector<miTime> FieldPlotManager::getFieldTime(const vector<miutil::KeyValue_v>& pinfos,
    bool updateSources)
{
  METLIBS_LOG_SCOPE();

  std::vector<FieldRequest> request;
  for (size_t i = 0; i < pinfos.size(); i++) {
    miutil::KeyValue_v fspec1,fspec2;
    if (!splitDifferenceCommandString(pinfos[i],fspec1,fspec2))
      // if difference, use first field
      fspec1 = pinfos[i];

    std::vector<FieldRequest> fieldrequest;
    std::string plotName;
    parsePin(fspec1, fieldrequest,plotName);
    request.insert(request.begin(),fieldrequest.begin(),fieldrequest.end());
  }

  if (request.empty())
    return std::vector<miutil::miTime>();

  return getFieldTime(request, updateSources);
}

miTime FieldPlotManager::getFieldReferenceTime(const miutil::KeyValue_v& pinfo)
{
  METLIBS_LOG_SCOPE();

  miutil::KeyValue_v fspec1,fspec2;

  // if difference, use first field
  if (!splitDifferenceCommandString(pinfo,fspec1,fspec2))
    fspec1 = pinfo;

  std::string plotName;
  FieldRequest request;
  vector<std::string> paramNames;
  parseString(pinfo, request, paramNames, plotName);

  std::string timestr = fieldManager->getBestReferenceTime(request.modelName, request.refoffset, request.refhour);
  return miTime(timestr);
}

void FieldPlotManager::getCapabilitiesTime(vector<miTime>& normalTimes,
    int& timediff, const PlotCommand_cp& pc)
{
  METLIBS_LOG_SCOPE(LOGVAL(pc->toString()));
  //Finding times from pinfo
  //TODO: find const time

  KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pc);
  if (!cmd)
    return;

  //finding timediff
  timediff = 0;
  for (const KeyValue& kv : cmd->all()) {
    if (kv.key() == "ignore_times" && kv.toBool() == true) {
      normalTimes.clear();
      return;
    }
    if (kv.key() == "timediff") {
      timediff = kv.toInt();
    }
  }

  //getting times
  normalTimes = getFieldTime(std::vector<miutil::KeyValue_v>(1, cmd->all()), true);

  METLIBS_LOG_DEBUG("no. of times"<<normalTimes.size());
}

vector<std::string> FieldPlotManager::getFieldLevels(const miutil::KeyValue_v& pin)
{
  vector<std::string> levels;
  vector<FieldRequest> vfieldrequest;
  std::string plotName;
  parsePin(pin, vfieldrequest,plotName);

  if ( !vfieldrequest.size() )
    return levels;

  map<std::string,FieldInfo> fieldInfo;
  fieldManager->getFieldInfo(vfieldrequest[0].modelName, vfieldrequest[0].refTime, fieldInfo);
  map<std::string,FieldInfo>::const_iterator ip =  fieldInfo.find(vfieldrequest[0].paramName);

  if ( ip != fieldInfo.end() ) {
    levels = ip->second.vlevels;
  }

  return levels;
}

vector<miTime> FieldPlotManager::getFieldTime(std::vector<FieldRequest>& request,
    bool updateSources)
{
  METLIBS_LOG_SCOPE();

  for (size_t i = 0; i <request.size(); ++i ) {
    if (request[i].plotDefinition) {
      std::vector<FieldRequest> fr = getParamNames(request[i].paramName, request[i]);
      if (!fr.empty()) {
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


bool FieldPlotManager::makeFields(const miutil::KeyValue_v& kvs,
    const miTime& const_ptime, vector<Field*>& vfout)
{
  METLIBS_LOG_SCOPE();

  // if difference
  miutil::KeyValue_v fspec1,fspec2;
  if (splitDifferenceCommandString(kvs,fspec1,fspec2)) {
    return makeDifferenceField(fspec1, fspec2, const_ptime, vfout);
  }

  vfout.clear();

  vector<FieldRequest> vfieldrequest;
  std::string plotName;
  parsePin(kvs, vfieldrequest, plotName);

  for (unsigned int i = 0; i < vfieldrequest.size(); i++) {

    if (vfieldrequest[i].ptime.undef())
      vfieldrequest[i].ptime = const_ptime;

    if (vfieldrequest[i].hourOffset != 0)
      vfieldrequest[i].ptime.addHour(vfieldrequest[i].hourOffset);
    if (vfieldrequest[i].minOffset != 0)
      vfieldrequest[i].ptime.addMin(vfieldrequest[i].minOffset);

    Field* fout = 0;
    if (!fieldManager->makeField(fout, vfieldrequest[i], FieldManager::READ_ALL))
      return false;

    makeFieldText(fout, plotName, vfieldrequest[i].flightlevel);
    vfout.push_back(fout);
  }

  return true;
}

void FieldPlotManager::makeFieldText(Field* fout, const std::string& plotName, bool flightlevel)
{
  std::string fieldtext = fout->modelName + " " + plotName;
  if (!fout->leveltext.empty()) {
    diutil::appendText(fieldtext, " ");
    if (flightlevel)
      diutil::appendText(fieldtext, FlightLevel::getFlightLevel(fout->leveltext));
    else
      diutil::appendText(fieldtext, fout->leveltext);
  }
  diutil::appendText(fieldtext, fout->idnumtext);

  if (!fout->analysisTime.undef() && !fout->validFieldTime.undef()) {
    fout->forecastHour = miutil::miTime::hourDiff(fout->validFieldTime, fout->analysisTime);
  }

  std::string progtext;
  if (!fout->analysisTime.undef() && fout->forecastHour != -32767) {
    std::ostringstream ostr;
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
  if (!fout->validFieldTime.undef()) {
    std::string sclock = fout->validFieldTime.isoClock();
    std::string shour = sclock.substr(0, 2);
    std::string smin = sclock.substr(3, 2);
    timetext = fout->validFieldTime.isoDate() + " " + shour;
    if (smin != "00")
      timetext += ":" + smin;
    timetext += " UTC";
  }

  fout->name = plotName;
  fout->text = fieldtext + " " + progtext;
  fout->fulltext = fieldtext + " " + progtext + " " + timetext;
  fout->fieldText = fieldtext;
  fout->progtext = progtext;
  fout->timetext = timetext;
}

void FieldPlotManager::freeFields(const std::vector<Field*>& fv)
{
  for (size_t i = 0; i < fv.size(); i++)
    fieldManager->freeField(fv[i]);
}

bool FieldPlotManager::makeDifferenceField(const miutil::KeyValue_v& fspec1,
    const miutil::KeyValue_v& fspec2, const miTime& const_ptime, vector<Field*>& fv)
{
  fv.clear();
  vector<Field*> fv1;
  vector<Field*> fv2;

  if (makeFields(fspec1, const_ptime, fv1)) {
    if (!makeFields(fspec2, const_ptime, fv2)) {
      freeFields(fv1);
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

  freeFields(fv1);

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

  if (diff[0])
    f1->modelName = "( " + text1[0] + " - " + text2[0] + " )";
  if (diff[1] && (diff[2] || diff[3])) {
    f1->name = "( " + text1[1];
    diutil::appendText(f1->name, text1[2]);
    diutil::appendText(f1->name, text1[3]);

    f1->name += " - " + text2[1];
    diutil::appendText(f1->name, text2[2]);
    diutil::appendText(f1->name, text2[3]);

    f1->name += " )";
    f1->leveltext.clear();
    if (diff[2] && diff[3])
      ndiff -= 2;
    else
      ndiff -= 1;
  } else {
    if (diff[1])
      f1->name = "( " + text1[1] + " - " + text2[1] + " )";
    if (diff[2])
      f1->leveltext = "( " + text1[2] + " - " + text2[2] + " )";
    if (diff[3])
      f1->idnumtext = "( " + text1[3] + " - " + text2[3] + " )";
  }
  if (diff[4])
    f1->progtext = "( " + text1[4] + " - " + text2[4] + " )";
  if (diff[5])
    f1->timetext = "( " + text1[5] + " - " + text2[5] + " )";
  if (ndiff == 1) {
    f1->fieldText = f1->modelName + " " + f1->name;
    diutil::appendText(f1->fieldText, f1->leveltext);
    f1->text = f1->fieldText + " " + f1->progtext;
    f1->fulltext = f1->text + " " + f1->timetext;
  } else {
    if (nbgn == 1 && nend <= 3) {
      diutil::appendText(text1[1], text1[2]);
      diutil::appendText(text1[1], text1[3]);
      diutil::appendText(text2[1], text2[2]);
      diutil::appendText(text2[1], text2[3]);

      text1[2].clear();
      text1[3].clear();
      text2[2].clear();
      text2[3].clear();
      nend = 1;
    }
    const int nmax[3] = { 5, 4, 3 };
    std::string ftext[3];
    for (int t = 0; t < 3; t++) {
      if (nbgn > nmax[t])
        nbgn = nmax[t];
      if (nend > nmax[t])
        nend = nmax[t];
      if (nbgn > 0) {
        ftext[t] = text1[0];
        for (int n = 1; n < nbgn; n++)
          ftext[t] += " " + text1[n];
      }
      diutil::appendText(ftext[t], "(");
      for (int n = nbgn; n <= nend; n++)
        diutil::appendText(ftext[t], text1[n]);

      ftext[t] += " -";

      for (int n = nbgn; n <= nend; n++)
        diutil::appendText(ftext[t], text2[n]);

      ftext[t] += " )";

      for (int n = nend + 1; n <= nmax[t]; n++)
        diutil::appendText(ftext[t], text1[n]);
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

  return fieldManager->makeDifferenceFields(fv, fv2);
}

void FieldPlotManager::getFieldGroups(const std::string& modelName, std::string refTime, bool plotdefinitions, vector<FieldGroupInfo>& vfgi)
{
  vfgi.clear();

  map<std::string,FieldInfo> fieldInfo;
  fieldManager->getFieldInfo(modelName, refTime, fieldInfo);
  //fieldManager->getFieldGroups(modelName, refTime, vfgi);
  map<std::string,FieldGroupInfo>mfgi;

  if (!plotdefinitions) {
    for (auto&& vi : fieldInfo) {
      const FieldInfo& plotInfo = vi.second;

      // add plot to FieldGroup
      mfgi[plotInfo.groupName].fieldNames.push_back(plotInfo.fieldName);
      mfgi[plotInfo.groupName].fields[plotInfo.fieldName]=plotInfo;
      mfgi[plotInfo.groupName].groupName = plotInfo.groupName;
      mfgi[plotInfo.groupName].plotDefinitions = false;
    }

  } else {

    for (unsigned int j = 0; j < vPlotField.size(); j++) {
      FieldInfo plotInfo;

      size_t ninput = 0; // number of input fields found
      for (size_t k = 0; k < vPlotField[j].input.size(); k++) {
        const std::string& fieldName = vPlotField[j].input[k];
        map<std::string,FieldInfo>::const_iterator ip;
        std::vector<std::string> tokens = miutil::split(fieldName,":");
        if ( tokens.size()==2 && tokens[1]=="standard_name") {
          ip = fieldInfo.begin();
          while ( ip != fieldInfo.end() && ip->second.standard_name != tokens[0])
            ip++;
        } else {
          ip =fieldInfo.find(fieldName);
        }

        if (ip == fieldInfo.end())
          break;
        if (vPlotField[j].vcoord.size() > 0 && !vPlotField[j].vcoord.count(ip->second.vcoord)  )
          break;

        if ( ninput == 0) {
          plotInfo = ip->second;
          plotInfo.fieldName = vPlotField[j].name;
          plotInfo.variableName = vPlotField[j].inputstr;
          if( !vPlotField[j].fieldgroup.empty() )
            plotInfo.groupName = vPlotField[j].fieldgroup;
        } else {
          if ( plotInfo.vlevels != ip->second.vlevels ) {
            // if the input parameters have different vlevels, but no. of levels are 0 or 1, ignore vlevels
            if ( plotInfo.vlevels.size() < 2 && ip->second.vlevels.size() < 2) {
                plotInfo.vlevels.clear();
                plotInfo.default_vlevel.clear();
                plotInfo.vcoord.clear();
              } else {
                break;
              }
          }
        }
        ninput++;
      }

      if (ninput == vPlotField[j].input.size()) {

        //add flightlevels
        if ( plotInfo.vcoord == "pressure" ) {
          FieldInfo plotInfo_fl = plotInfo;
          miutil::replace(plotInfo_fl.groupName,"pressure","flightlevel");
          plotInfo_fl.vcoord = "flightlevel";
          for (size_t i=0; i<plotInfo.vlevels.size(); ++i)
            plotInfo_fl.vlevels[i] = FlightLevel::getFlightLevel(plotInfo.vlevels[i]);
          if ( groupNames.count(plotInfo_fl.groupName))
            plotInfo_fl.groupName = groupNames[plotInfo_fl.groupName];

          mfgi[plotInfo_fl.groupName].fieldNames.push_back(plotInfo_fl.fieldName);
          mfgi[plotInfo_fl.groupName].fields[plotInfo_fl.fieldName]=plotInfo_fl;
          mfgi[plotInfo_fl.groupName].groupName = plotInfo_fl.groupName;
        }

        // add plot to FieldGroup
        if ( groupNames.count(plotInfo.groupName))
          plotInfo.groupName = groupNames[plotInfo.groupName];
        mfgi[plotInfo.groupName].fieldNames.push_back(plotInfo.fieldName);
        mfgi[plotInfo.groupName].fields[plotInfo.fieldName]=plotInfo;
        mfgi[plotInfo.groupName].groupName = plotInfo.groupName;
      }

    }
  }

  map<std::string,FieldGroupInfo>::iterator ifgi = mfgi.begin();
  for(;ifgi !=mfgi.end();ifgi++){
    vfgi.push_back(ifgi->second);
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

void FieldPlotManager::parseString(const miutil::KeyValue_v& pin,
    FieldRequest& fieldrequest,
    vector<std::string>& paramNames,
    std::string& plotName)
{
  METLIBS_LOG_SCOPE(LOGVAL(pin));

  for (const miutil::KeyValue& kv : pin) {
    if (!kv.value().empty()) {
      const std::string& key = kv.key();
      if (key == "model") {
        fieldrequest.modelName = kv.value();
      }else if (key == "parameter") {
        paramNames.push_back(kv.value());
        fieldrequest.plotDefinition=false;
      }else if (key == "plot") {
        plotName = kv.value();
        fieldrequest.plotDefinition=true;
      } else if (key == "vcoord") {
        fieldrequest.zaxis = kv.value();
      } else if (key == "tcoor") {
        fieldrequest.taxis = kv.value();
      } else if (key == "ecoord") {
        fieldrequest.eaxis = kv.value();
      } else if (key == "vlevel") {
        fieldrequest.plevel = kv.value();
      } else if (key == "elevel") {
        fieldrequest.elevel = kv.value();
      } else if (key == "unit" || key == "units") {
        fieldrequest.unit = kv.value();
      } else if (key == "vunit" && kv.value() == "FL") {
        fieldrequest.flightlevel=true;
      } else if (key == "time") {
        fieldrequest.ptime = miTime(kv.value());
      } else if (key == "reftime") {
        fieldrequest.refTime = kv.value();
      } else if (key == "refhour") {
        fieldrequest.refhour = kv.toInt();
      } else if (key == "refoffset") {
        fieldrequest.refoffset = kv.toInt();
      } else if (key == "hour.offset") {
        fieldrequest.hourOffset = kv.toInt();
      } else if (key == "min.offset") {
        fieldrequest.minOffset = kv.toInt();
      } else if (key == "hour.diff") {
        fieldrequest.time_tolerance = kv.toInt() * 60; //time_tolerance in minutes, hour.diff in hours
      } else if (key == "alltimesteps") {
        fieldrequest.allTimeSteps = kv.toBool();
      } else if (key == "file.palette") {
        fieldrequest.palette = kv.value();
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
      frq.plevel = FlightLevel::getPressureLevel(frq.plevel);
    }
  }
}

std::string FieldPlotManager::extractPlotName(const miutil::KeyValue_v& pin)
{
  std::string plotName;
  vector<FieldRequest> vfieldrequest;
  parsePin(pin, vfieldrequest, plotName);
  return plotName;
}

void FieldPlotManager::parsePin(const miutil::KeyValue_v& pin, vector<FieldRequest>& vfieldrequest, std::string& plotName)
{
  METLIBS_LOG_SCOPE(LOGVAL(pin));

  // if difference
  miutil::KeyValue_v fspec1,fspec2;
  if (splitDifferenceCommandString(pin,fspec1,fspec2)) {
    parsePin(fspec1, vfieldrequest, plotName);
    return;
  }

  FieldRequest fieldrequest;
  vector<std::string> paramNames;
  parseString(pin, fieldrequest, paramNames, plotName);

  //  //plotName -> fieldName
  if (fieldrequest.plotDefinition) {
    vfieldrequest = getParamNames(plotName,fieldrequest);
  } else {
    for (size_t i=0; i<paramNames.size(); i++) {
      fieldrequest.paramName = paramNames[i];
      vfieldrequest.push_back(fieldrequest);
    }
  }
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

  for (const PlotField& pf : vPlotField) {
    if ((pf.name == plotName)
        && ((pf.vcoord.empty() && pf.vctype == FieldFunctions::vctype_none)
            || pf.vcoord.count(fieldrequest.zaxis)
            || pf.vctype==FieldFunctions::Zaxis_info_map[fieldrequest.zaxis].vctype))
    {
      for (const std::string& inputIJ : pf.input) {
        const vector<std::string> vstr = miutil::split(inputIJ, ":");
        fieldrequest.paramName = vstr[0];
        fieldrequest.standard_name = (vstr.size() == 2 && vstr[1] == "standard_name");
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

bool FieldPlotManager::splitDifferenceCommandString(const miutil::KeyValue_v& pin, miutil::KeyValue_v& fspec1, miutil::KeyValue_v& fspec2)
{
  const size_t npos = size_t(-1);
  const size_t p1 = find(pin, "(");
  if (p1 == npos)
    return false;

  const size_t p2 = find(pin, "-", p1+1);
  if (p2 == npos)
    return false;

  const size_t p3 = find(pin, ")", p2+1);
  if (p3 == npos)
    return false;

  const miutil::KeyValue_v common_start(pin.begin(), pin.begin()+p1);
  const miutil::KeyValue_v common_end(pin.begin()+p3+1, pin.end());

  fspec1 = common_start;
  fspec1.insert(fspec1.end(), pin.begin()+p1+1, pin.begin()+p2);
  fspec1.insert(fspec1.end(), common_end.begin(), common_end.end());

  fspec2 = common_start;
  fspec2.insert(fspec2.end(), pin.begin()+p2+1, pin.begin()+p3);
  fspec2.insert(fspec2.end(), common_end.begin(), common_end.end());

  return true;
}

// ---------------- fieldPlotOptions management --------------------

std::map<std::string, PlotOptions> FieldPlotManager::fieldPlotOptions;
std::map<std::string, miutil::KeyValue_v> FieldPlotManager::fieldDataOptions;

// update static fieldplotoptions
bool FieldPlotManager::updateFieldPlotOptions(const std::string& name,
    const miutil::KeyValue_v& opts)
{
  return PlotOptions::parsePlotOption(opts, fieldPlotOptions[name], fieldDataOptions[name]);
}

void FieldPlotManager::getAllFieldOptions(const vector<std::string>& fieldNames,
    map<std::string, miutil::KeyValue_v>& fieldoptions)
{
  // The selected PlotOptions elements are used to activate elements
  // in the FieldDialog (any remaining will be used unchanged from setup)
  // Also return any field prefixes and suffixes used.

  fieldoptions.clear();

  for (const std::string& fn : fieldNames) {
    miutil::KeyValue_v fdo;
    PlotOptions po;
    getFieldPlotOptions(fn, po, fdo);

    miutil::KeyValue_v& fpo = fieldoptions[fn];
    fpo = po.toKeyValueList();
    fpo.insert(fpo.end(), fdo.begin(), fdo.end());
  }
}

void FieldPlotManager::getFieldPlotOptions(const std::string& name, PlotOptions& po, miutil::KeyValue_v& fdo)
{
  map<std::string,PlotOptions>::iterator p = fieldPlotOptions.find(name);
  if (p != fieldPlotOptions.end()) {
    po = p->second;
  } else {
    fieldPlotOptions[name]= po;
  }
  map<std::string,miutil::KeyValue_v>::iterator ido = fieldDataOptions.find(name);
  if (ido != fieldDataOptions.end()) {
    fdo = ido->second;
  } else {
    fieldDataOptions[name]= fdo;
  }
}

std::string FieldPlotManager::getFieldClassSpecs(const std::string& fieldplotname)
{
  map<std::string,PlotOptions>::iterator p = fieldPlotOptions.find(fieldplotname);
  if (p != fieldPlotOptions.end()) {
    return p->second.classSpecifications;;
  } else {
    return std::string();
  }
}
