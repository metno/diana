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
#include <diField/diPlotOptions.h>
#include <diField/FieldSpecTranslation.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace miutil;

FieldPlotManager::FieldPlotManager(FieldManager* fm) :
      fieldManager(fm)
{
}

void FieldPlotManager::getAllFieldNames(vector<miString>& fieldNames)
{

  for (unsigned int i = 0; i < vPlotField.size(); i++) {
    fieldNames.push_back(vPlotField[i].name);
  }

}

bool FieldPlotManager::parseSetup(SetupParser &sp)
{

  if ( !parseFieldPlotSetup(sp) ) {
    return false;
  }
  if ( !parseFieldGroupSetup(sp) ) {
    return false;
  }
  return true;
}

bool FieldPlotManager::parseFieldPlotSetup(SetupParser &sp)
{

  //   cerr <<"bool FieldPlotManager::parseSetup"<<endl;

  fieldManager->getPrefixandSuffix(fieldprefixes, fieldsuffixes);

  miString sect_name = "FIELD_PLOT";
  vector<miString> lines;

  if (!sp.getSection(sect_name, lines)) {
    cerr << sect_name << " section not found" << endl;
    return true;
  }

  const miString key_loop = "loop";
  const miString key_field = "field";
  const miString key_endfield = "end.field";
  const miString key_fieldgroup = "fieldgroup";
  const miString key_plot = "plot";

  const miString key_plottype = "plottype";

  // parse setup

  int nlines = lines.size();

  vector<miString> vstr;
  miString key, str, str2, option;
  vector<miString> vpar;

  vector<miString> loopname;
  vector<vector<miString> > loopvars;
  map<miString,int>::const_iterator pfp;
  int firstLine = 0,lastLine,nv;
  bool waiting= true;

  for (int l = 0; l < nlines; l++) {

    vstr = splitComStr(lines[l], true);
    int n = vstr.size();
    key = vstr[0].downcase();

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
        miString name;
        miString fieldgroup;
        vector<miString> input;
        for (int i = firstLine; i < lastLine; i++) {
          str = lines[i];
          for (unsigned int il = 0; il < nl; il++) {
            str.replace(loopname[il], loopvars[il][m]);
          }
          if (i == firstLine) {
            // resplit to keep names with ()
            vstr = str.split('=', false);
            if (vstr.size() < 2) {
              miString errm = "Missing field name";
              sp.errorMsg(sect_name, i, errm);
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
              key = vstr[j].downcase();
              if (key == key_plot && vstr[j + 1] == "=" && j < nv - 3) {
                option = key_plottype + "=" + vstr[j + 2];
                if (!PlotOptions::updateFieldPlotOptions(name, option)) {
                  miString errm = "|Unknown fieldplottype in plotcommand";
                  sp.errorMsg(sect_name, i, errm);
                  break;
                }
                str2 = vstr[j + 3].downcase().substr(1, vstr[j + 3].length()
                    - 2);
                input = str2.split(',', true);
                if (input.size() < 1 || input.size() > 5) {
                  miString errm = "Bad specification of plot arguments";
                  sp.errorMsg(sect_name, i, errm);
                  break;
                }

                option = "dim=" + miString(int(input.size()));

                if (!PlotOptions::PlotOptions::updateFieldPlotOptions(name, option)){
                  miString errm = "|Unknown fieldplottype in plotcommand";
                                    sp.errorMsg(sect_name, i, errm);
                                    break;
                }
                for (unsigned int k = 0; k < input.size(); k++) {
                  input[k] = input[k].downcase();
                }
              } else if (key == key_fieldgroup && vstr[j + 1] == "=") {
                fieldgroup = vstr[j + 2];
                if (fieldgroup[0] == '"' && fieldgroup[fieldgroup.length() - 1]
                                                       == '"') {
                  fieldgroup = fieldgroup.substr(1, fieldgroup.length() - 2);
                }
              } else if (vstr[j + 1] == "=") {
                // this should be a plot option
                option = vstr[j] + "=" + vstr[j + 2];

                if (!PlotOptions::updateFieldPlotOptions(name, option)) {
                  miString errm =
                      "Something wrong in plotoption specifications";
                  sp.errorMsg(sect_name, i, errm);
                  break;
                }
              } else {
                miString errm = "Unknown keyword in field specifications: "
                    + vstr[0];
                sp.errorMsg(sect_name, i, errm);
                break;
                //j-=2;
              }
              j += 3;
            }
          }
        }

        if (!name.empty() && !input.empty()) {
          unsigned int i = 0;
          while (i < vPlotField.size() && vPlotField[i].name.downcase()
              != name.downcase())
            i++;
          if (i < vPlotField.size()) {
            cerr << "  replacing plot specs. for field " << name << endl;
            vPlotField[i].input = input;
          } else {
            PlotField pf;
            pf.name = name;
            pf.fieldgroup = fieldgroup;
            pf.input = input;
            vPlotField.push_back(pf);
          }
          mapPlotField[name.downcase()] = vPlotField[i];
        }
      }

      loopname.clear();
      loopvars.clear();
      waiting = true;
    }
  }

  return true;
}

bool FieldPlotManager::parseFieldGroupSetup(SetupParser &sp)
{

  miString sect_name = "FIELD_GROUPS";
  vector<miString> lines;

  if (!sp.getSection(sect_name, lines)) {
    cerr << sect_name << " section not found" << endl;
    return true;
  }

  const miString key_name = "name";
  const miString key_group = "group";

  int nlines = lines.size();

    for (int l = 0; l < nlines; l++) {
    vector<miString> tokens= lines[l].split('"','"');
    if ( tokens.size()== 2 ) {
      vector<miString> stokens= tokens[0].split('"','"',"=",true);
      if (stokens.size() == 2 && stokens[0] == key_name ){
        miString name = stokens[1];
        stokens= tokens[1].split('"','"',"=",true);
        if (stokens.size() == 2 && stokens[0] == key_group ){
          groupNames[stokens[1]] = name;
        }
      }
    }
  }

  return true;
}

vector<miString> FieldPlotManager::splitComStr(const miString& s, bool splitall)
{
  // split commandstring into tokens.
  // split on '=', ',' and multiple blanks, keep blocks within () and ""
  // split on ',' only if <splitall> is true

  vector<miString> tmp;

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

vector<miString> FieldPlotManager::getFields()
{

  set<miString> paramSet;
  for (unsigned int i = 0; i < vPlotField.size(); i++) {
    for (unsigned int j = 0; j < vPlotField[i].input.size(); j++) {
      paramSet.insert(vPlotField[i].input[j]);
    }
  }

  vector<miString> param;
  set<miString>::iterator p = paramSet.begin();
  for (; p != paramSet.end(); p++) {
    param.push_back(*p);
  }

  return param;

}

vector<miTime> FieldPlotManager::getFieldTime(const vector<miString>& pinfos,
    bool& constTimes)
{
  vector<miTime> fieldtime;

  int numf = pinfos.size();

  vector<FieldRequest> request;
  miString modelName, modelName2, fieldName;

  for (int i = 0; i < numf; i++) {

    // if difference, use first field
    miString fspec1,fspec2;
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

  return getFieldTime(request, constTimes);
}

void FieldPlotManager::getCapabilitiesTime(vector<miTime>& normalTimes,
    miTime& constTimes, int& timediff, const miString& pinfo)
{
  //Finding times from pinfo
  //TODO: find const time

  vector<miString> pinfos;
  pinfos.push_back(pinfo);

  //finding timediff
  timediff = 0;
  vector<miString> tokens = pinfo.split('"', '"');
  for (unsigned int j = 0; j < tokens.size(); j++) {
    vector<miString> stokens = tokens[j].split("=");
    if (stokens.size() == 2 && stokens[0].downcase() == "timediff") {
      timediff = stokens[1].toInt();
    }
  }

  //getting times
  bool constT;
  normalTimes = getFieldTime(pinfos, constT);
  if (constT) {
    if (normalTimes.size()) {
      constTimes = normalTimes[0];
    }
    normalTimes.clear();
  }

}

vector<miString> FieldPlotManager::getFieldLevels(const miString& pinfo)
{

  vector<miString> levels;

  vector<miString> tokens = pinfo.split(" ");
  if (tokens.size() < 3 || tokens[0] != "FIELD") {
    return levels;
  }

  vector<FieldGroupInfo> vfgi;
  miString name;
  miTime refTime;
  getFieldGroups(tokens[1], name, refTime, vfgi);
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

vector<miString> FieldPlotManager::getPlotFields()
{

  vector<miString> param;
  for (unsigned int i = 0; i < vPlotField.size(); i++) {
    param.push_back(vPlotField[i].name);
  }

  return param;

}

vector<miTime> FieldPlotManager::getFieldTime(
    vector<FieldRequest>& request, bool& constTimes)

{

  vector<miTime> vtime;
  for (size_t i = 0; i <request.size(); ++i ) {
    vector<std::string> name = getParamNames(request[i].paramName);
    if ( name.size()>0 ) {
      request[i].paramName = name[0];
    }
  }
  return fieldManager->getFieldTime(request, constTimes);
}

bool FieldPlotManager::makeFields(const miString& pin_const,
    const miTime& const_ptime, vector<Field*>& vfout, bool toCache)
{

  // if difference
  miString fspec1,fspec2;
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

    if (vfieldrequest[i].hourOffset != 0) {
      vfieldrequest[i].ptime.addHour(vfieldrequest[i].hourOffset);
    }
    Field* fout;
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

    makeFieldText(fout, plotName);
    vfout.push_back(fout);

  }

  return true;

}

void FieldPlotManager::makeFieldText(Field* fout, const miString& plotName)
{

  miString fieldtext = fout->modelName + " " + plotName;
  if (!fout->leveltext.empty()) {
    fieldtext += " " + fout->leveltext;
  }
  if (!fout->idnumtext.empty()) {
    fieldtext += " " + fout->idnumtext;
  }

  ostringstream ostr;
  if (fout->forecastHour >= 0) {
    ostr << "+" << fout->forecastHour;
  } else {
    ostr << fout->forecastHour;
  }
  miString progtext = "(" + ostr.str() + ")";

  miString sclock = fout->validFieldTime.isoClock();
  miString shour = sclock.substr(0, 2);
  miString smin = sclock.substr(3, 2);
  miString timetext;
  if (smin == "00") {
    timetext = fout->validFieldTime.isoDate() + " " + shour + " UTC";
  } else {
    timetext = fout->validFieldTime.isoDate() + " " + shour + ":" + smin
        + " UTC";
  }
  fout->name = plotName;
  fout->text = fieldtext + " " + progtext;
  fout->fulltext = fieldtext + " " + progtext + " " + timetext;
  fout->fieldText = fieldtext;
  fout->progtext = progtext;
  fout->timetext = timetext;

}

bool FieldPlotManager::makeDifferenceField(const miString& fspec1,
    const miString& fspec2, const miTime& const_ptime, vector<Field*>& fv)
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
  miString text1[mdiff], text2[mdiff];
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
    if (f1->leveltext.exists()) {
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
    miString ftext[3];
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

#ifdef DEBUGFDIFF
  cerr<<"F1-F2: validFieldTime: "<<f1->validFieldTime<<endl;
  cerr<<"F1-F2: analysisTime:   "<<f1->analysisTime<<endl;
  cerr<<"F1-F2: name:           "<<f1->name<<endl;
  cerr<<"F1-F2: text:           "<<f1->text<<endl;
  cerr<<"F1-F2: fulltext:       "<<f1->fulltext<<endl;
  cerr<<"F1-F2: modelName:      "<<f1->modelName<<endl;
  cerr<<"F1-F2: fieldText:      "<<f1->fieldText<<endl;
  cerr<<"F1-F2: leveltext:      "<<f1->leveltext<<endl;
  cerr<<"F1-F2: idnumtext:      "<<f1->idnumtext<<endl;
  cerr<<"F1-F2: progtext:       "<<f1->progtext<<endl;
  cerr<<"F1-F2: timetext:       "<<f1->timetext<<endl;
  cerr<<"-----------------------------------------------------"<<endl;
#endif
  bool ok = fieldManager->makeDifferenceFields(fv, fv2);
  if (!ok) {
    return false;
  }
  return true;

}

void FieldPlotManager::getFieldGroups(const miString& modelNameRequest,
    miString& modelName, miTime refTime, vector<FieldGroupInfo>& vfgi)
{

  fieldManager->getFieldGroups(modelNameRequest, modelName, refTime, vfgi);

  size_t nvfgi = vfgi.size();

  //replace fieldnames with plotnames
  for (size_t i = 0; i < nvfgi; i++) {

    //Make copy with filed names from file
    if(fieldManager->isGridCollection(modelName)) {
      vfgi.push_back(vfgi[i]);
      vfgi[vfgi.size()-1].plotDefinitions = false;
    }

    //use groupname from setup if defined
    if ( groupNames.count(vfgi[i].groupName)) {
      vfgi[i].groupName = groupNames[vfgi[i].groupName];
    }

    //sort fieldnames and suffixes
    map<miString, vector<miString> > fieldName_suffix;
    for (unsigned int l = 0; l < vfgi[i].fieldNames.size(); l++) {
      miString suffix;
      miString fieldName = vfgi[i].fieldNames[l];
      splitSuffix(fieldName, suffix);
      fieldName_suffix[fieldName].push_back(suffix);
    }
//    for (unsigned int l = 0; l < vfgi[i].standard_names.size(); l++) {
//      miString suffix;
//      miString fieldName = vfgi[i].standard_names[l];
//      fieldName_suffix[fieldName].push_back(suffix);
//    }

    //find plotNames
    vector<miString> plotNames;
    for (unsigned int j = 0; j < vPlotField.size(); j++) {
      miString plotName = vPlotField[j].name;
      //check that all fields needed exist with same suffix
      map<std::string, unsigned int> suffixmap;
      for (unsigned int k = 0; k < vPlotField[j].input.size(); k++) {
        std::string fieldName = std::string(vPlotField[j].input[k]);
        if (!fieldName_suffix.count(fieldName)) {
          break;
        }
        for (unsigned int l = 0; l < fieldName_suffix[fieldName].size(); l++) {
          suffixmap[fieldName_suffix[fieldName][l]] += 1;
        }
      }

      //add plotNames without suffix
      if (suffixmap[""] >= vPlotField[j].input.size()) {
        plotNames.push_back(plotName);
      }

      //add plotNames with suffix
      set<std::string>::iterator p;
      for (p = fieldsuffixes.begin(); p != fieldsuffixes.end(); p++) {
        if (suffixmap[*p] >= vPlotField[j].input.size()) {
          miString pN = plotName + *p;
          plotNames.push_back(pN);
        }
      }
    }
    vfgi[i].fieldNames = plotNames;

  }

}

void FieldPlotManager::getAllFieldNames(vector<miString> & fieldNames,
    set<std::string>& fprefixes, set<std::string>& fsuffixes)
{

  fieldNames = getPlotFields();
  fprefixes = fieldprefixes;
  fsuffixes = fieldsuffixes;

}

bool FieldPlotManager::splitSuffix(std::string& plotName, std::string& suffix)
{

  set<std::string>::const_iterator ps = fieldsuffixes.begin();
  for (; ps != fieldsuffixes.end(); ps++) {
    if (plotName.find(*ps) != std::string::npos) {
      suffix = *ps;
      plotName.erase(plotName.find(*ps));
      return true;
    }
  }

  return false;
}

bool FieldPlotManager::parsePin( std::string& pin, vector<FieldRequest>& vfieldrequest, std::string& plotName)


//    miString& modelName,
//    miString& plotName, vector<miString>& fieldName, miString& levelName,
//    miString& idnumName, int& hourOffset, int& hourDiff, miTime& time)
{
 // cerr <<"PIN: "<<pin<<endl;

  if (pin.find("model=") == std::string::npos ) {
    pin = FieldSpecTranslation::getNewFieldString(pin);
  }

  std::vector<std::string> tokens;
  //NB! what about ""
  boost::algorithm::split(tokens, pin, boost::algorithm::is_space());

  size_t n = tokens.size();
  std::string str, key;
  FieldRequest fieldrequest;
  vector<std::string> paramNames;

  for (size_t k = 1; k < n; k++) {
    std::vector<std::string> vtoken;
    boost::algorithm::split(vtoken, tokens[k], boost::algorithm::is_any_of(std::string("=")));
    if (vtoken.size() >= 2) {
      key = boost::algorithm::to_lower_copy(vtoken[0]);
      if (key == "model") {
        fieldrequest.modelName = vtoken[1];
      }else if (key == "parameter") {
        plotName = vtoken[1];
        paramNames.push_back(vtoken[1]);
      }else if (key == "plot") {
        plotName = vtoken[1];
        paramNames = getParamNames(vtoken[1]);
      } else if (key == "vcoor") {
        fieldrequest.zaxis = vtoken[1];
      } else if (key == "tcoor") {
        fieldrequest.taxis = vtoken[1];
      } else if (key == "ecoor") {
        fieldrequest.eaxis = vtoken[1];
      } else if (key == "vlevel") {
        fieldrequest.plevel = vtoken[1];
      } else if (key == "elevel") {
        fieldrequest.elevel = vtoken[1];
      } else if (key == "grid") {
        fieldrequest.grid = vtoken[1];
      } else if (key == "time") {
        fieldrequest.ptime = miTime(vtoken[1]);
      } else if (key == "reftime") {
        fieldrequest.refTime = miTime(vtoken[1]);
      } else if (key == "refhour") {
        fieldrequest.refhour = atoi(vtoken[1].c_str());
      } else if (key == "refoffset") {
        fieldrequest.refoffset = atoi(vtoken[1].c_str());
      } else if (key == "hour.offset") {
        fieldrequest.hourOffset = atoi(vtoken[1].c_str());
      } else if (key == "hour.diff") {
        fieldrequest.time_tolerance = atoi(vtoken[1].c_str());
      } else if (key == "alltimesteps") {
        if (vtoken[1] == "1" || vtoken[1] == "on" || vtoken[1] == "true") {
          fieldrequest.allTimeSteps = true;
        }
      } else if (key == "forecast.hour") {
       std::vector<std::string> values;
       boost::algorithm::split(values,vtoken[1], boost::algorithm::is_any_of(std::string(",")));
       for (unsigned int i = 0; i < values.size(); i++) {
         fieldrequest.forecast.push_back(atoi(values[i].c_str()));
       }
       fieldrequest.forecastSpec = 1;
     } else if (vtoken[0] == "forecast.hour.loop") {
       std::vector<std::string> values;
       boost::algorithm::split(values,vtoken[1], boost::algorithm::is_any_of(std::string(",")));
       if (values.size() == 3) { // first,last,step
         for (int i = 0; i < 3; i++) {
           fieldrequest.forecast.push_back(atoi(values[i].c_str()));
         }
         fieldrequest.forecastSpec = 2;
       }
     }
    }
  }

  //  //plotName -> fieldName

  for (size_t i = 0; i < paramNames.size(); i++) {
    fieldrequest.paramName = paramNames[i];
    vfieldrequest.push_back(fieldrequest);
  }

  return true;

}

vector<std::string> FieldPlotManager::getParamNames(std::string plotName)
{

  vector<std::string> paramNames;
  std::string suffix;
  splitSuffix(plotName, suffix);

  std::locale::global(std::locale(""));
  boost::algorithm::to_lower(plotName, locale());
  //  std::cout << std::locale().name() << std::endl;

  std::locale::global(std::locale("C"));

//If plotName not defined, use plotName as fieldName
  if (!mapPlotField.count(plotName)) {
    paramNames.push_back(plotName);
    return paramNames;
  }

  for (size_t i = 0; i < mapPlotField[plotName].input.size(); i++) {
    std::string inputName = boost::algorithm::to_lower_copy(std::string(mapPlotField[plotName].input[i]));
    inputName += suffix;
    paramNames.push_back(inputName);
  }

  return paramNames;
//  plotName += suffix;

}

bool FieldPlotManager::splitDifferenceCommandString(miString pin, miString& fspec1, miString& fspec2)
{

  //if difference, split pin and return true
  if (pin.contains(" ( ") && pin.contains(" - ") && pin.contains(" ) ")) {
    size_t p1 = pin.find(" ( ", 0);
    size_t p2 = pin.find(" - ", p1 + 3);
    size_t p3 = pin.find(" ) ", p2 + 3);
    if (p1 != string::npos && p2 != string::npos && p3 != string::npos) {
      fspec1 = pin.substr(0, p1) + pin.substr(p1 + 2, p2 - p1
          - 2);
      fspec2 = pin.substr(0, p1) + pin.substr(p2 + 2, p3 - p2
          - 2);
      return true;
    }
  }

  // if not difference, return false
  return false;

}
