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


#include <diObsAscii.h>
#include <diObsPlot.h>
#include <diObsMetaData.h>

#include <puTools/miStringFunctions.h>

#include <curl/curl.h>

#include <fstream>

#define MILOGGER_CATEGORY "diana.ObsAscii"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

ObsAscii::ObsAscii(const string& filename, const string& headerfile,
    const vector<string> headerinfo)
{
  fileOK = false;
  knots = false;
  readHeaderInfo(filename,headerfile, headerinfo);
  decodeHeader();
}

ObsAscii::ObsAscii(const string& filename, const string& headerfile,
    const vector<string>& headerinfo, const miTime &filetime,
    ObsPlot *oplot)
{
  fileOK = false;
  knots = false;
  readHeaderInfo(filename,headerfile, headerinfo);
  decodeHeader();
  oplot->setLabels(labels);
  oplot->columnName =columnName;

  plotTime = oplot->getObsTime();
  timeDiff= oplot->getTimeDiff();
  fileTime = filetime;
  if ( !headerfile.empty() || headerinfo.size() ) {
    readData(filename);
  }
  decodeData();

  oplot->addObsData(vObsData);
}

ObsAscii::ObsAscii(const string &filename, const string &headerfile,
    const vector<string>& headerinfo, ObsMetaData *metaData)
{
  fileOK = false;
  knots = false;
  readHeaderInfo(filename,headerfile, headerinfo);
  decodeHeader();

  if ( !headerfile.empty() || headerinfo.size() ) {
    readData(filename);
  }
  decodeData();

  metaData->setObsData(mObsData);
}

bool ObsAscii::getFromFile(const string& filename, vector<string>& lines)
{
  cerr <<"getFromFile: "<<filename<<endl;
  // open filestream
  ifstream file(filename.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ObsAscii: " << filename << " not found");
    return false;
  }

  std::string str;
  while (getline(file, str)) {
    lines.push_back(str);
  }

  file.close();
  return true;
}

size_t write_dataa(void *buffer, size_t size, size_t nmemb, void *userp)
{
  string tmp =(char *)buffer;
  (*(string*) userp)+=tmp.substr(0,nmemb);
  return (size_t)(size *nmemb);
}



bool ObsAscii::getFromHttp(const string &url, vector<string>& lines)
{
  CURL *curl = NULL;
  string data;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_dataa);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    std::string mdata=data;
    vector<std::string> result;
    result = miutil::split(mdata, "\n");
    lines.insert(lines.end(),result.begin(),result.end());

    return (res == 0);
  }

  return false;
}

void ObsAscii::readHeaderInfo(const string& filename, const string& headerfile,
    const vector<string>& headerinfo)
{

  //####################################################################
//  METLIBS_LOG_DEBUG("ObsAscii::readFile  filename= "<<filename);
//  METLIBS_LOG_DEBUG("Headerfiles:"<<headerfile);
//  for(size_t i = 0; i < headerinfo.size(); i++) {
//      METLIBS_LOG_DEBUG("headerinfo: " << headerinfo[i]);
//    }
  //####################################################################


  if (headerinfo.size() > 0 ) {
    lines = headerinfo;
  } else {

    std::string name;
    if ( headerfile.empty()) {
      name = filename;
    } else {
      name = headerfile;
    }

    if (miutil::contains(name, "http")) {
      getFromHttp(name, lines);
    } else {
      getFromFile(name, lines);
    }

  }

}

void ObsAscii::readData(const std::string &filename)
{

  if (miutil::contains(filename, "http")) {
    getFromHttp(filename, lines);
  } else {
    getFromFile(filename, lines);
  }

}

void ObsAscii::decodeHeader()
{
//METLIBS_LOG_DEBUG(__FUNCTION__);
  vector<string> vstr,pstr;
  string str;
  size_t p;

  for (size_t i = 0; i < lines.size(); ++i ) {
    miutil::trim(lines[i]);
    if (not lines[i].empty()) {
      p= lines[i].find('#');
      if (p==string::npos) {
        if (lines[i]=="[DATA]") break;  // end of header, start data
        vstr.push_back(lines[i]);
      } else if (p>0) {
        pstr= miutil::split(lines[i], "#");
        if (pstr[0]=="[DATA]")
          break;  // end of header, start data
        vstr.push_back(pstr[0]);
      }
    }
  }



  fileOK=   false;
  asciiColumn.clear();
  asciiSkipDataLines= 0;

  columnName.clear();
  columnType.clear();
  asciiColumnUndefined.clear();

  // parse header

  const string key_columns=   "COLUMNS";
  const string key_undefined= "UNDEFINED";
  const string key_skiplines= "SKIP_DATA_LINES";
  const string key_label=     "LABEL";
  const string key_splitchar=     "SEPARATOR";

  bool ok= true;
  int n= vstr.size();
  //####################################################################
//  METLIBS_LOG_DEBUG("HEADER:");
//  for (int  j=0; j<n; j++)
//    METLIBS_LOG_DEBUG(vstr[j]);
//  METLIBS_LOG_DEBUG("-----------------");
  //####################################################################
  size_t p1,p2;
  int i= 0;

  while (i<n) {
    str= vstr[i];
    p1= str.find('[');
    if (p1!=string::npos) {
      p2= str.find(']');
      i++;
      while (p2==string::npos && i<n) {
        str+=(" " + vstr[i]);
        p2= str.find(']');
        i++;
      }
      if (p2==string::npos) {
        ok= false;
        break;
      }
      str= str.substr(p1+1,p2-p1-1);
      pstr= miutil::split_protected(str, '"', '"');
      int j,m= pstr.size();

      if (m>1) {
        if (pstr[0]==key_columns) {
          if (not separator.empty()) {
            pstr= miutil::split(str, separator);
          }
          vector<std::string> vs;
          m=pstr.size();
          for (j=1; j<m; j++) {
            miutil::remove(pstr[j], '"');
            vs= miutil::split(pstr[j], ":");
            if (vs.size()>1) {
              columnName.push_back(vs[0]);
              columnType.push_back(miutil::to_lower(vs[1]));
              if (vs.size()>2) {
                columnTooltip.push_back(vs[2]);
              }else{
                columnTooltip.push_back("");
              }
            }
          }
        } else if (pstr[0]==key_undefined ) {
          vector<string> vs= miutil::split(pstr[1], ",");
          int nu= vs.size();
          // sort with longest undefined strings first
          vector<int> len;
          for (j=0; j<nu; j++)
            len.push_back(vs[j].length());
          for (int k=0; k<nu; k++) {
            int lmax=0, jmax=0;
            for (j=0; j<nu; j++) {
              if (len[j]>lmax) {
                lmax= len[j];
                jmax= j;
              }
            }
            len[jmax]= 0;
            asciiColumnUndefined.push_back(vs[jmax]);
          }
        } else if (pstr[0]==key_skiplines && m>1) {
          asciiSkipDataLines= atoi(pstr[1].c_str());

        } else if (pstr[0]==key_label) {
          labels.push_back(str);

        } else if (pstr[0]==key_splitchar) {
          separator = pstr[1];
        }
      }
    } else {
      i++;
    }
  }

  if (!ok) {
    //####################################################################
    METLIBS_LOG_ERROR("   bad header !!!!!!!!!");
    //####################################################################
    return;
  }

  n= columnType.size();
  //####################################################################
//  METLIBS_LOG_DEBUG("     coloumns= "<<n);
  //####################################################################

  knots=false;
  for (i=0; i<n; i++) {
    //####################################################################
//    METLIBS_LOG_DEBUG("   column "<<i<<" : "<<columnName[i]<<"  "
//        <<columnType[i]);
    //####################################################################
    if      (columnType[i]=="d")
      asciiColumn["date"]= i;
    else if (columnType[i]=="t")
      asciiColumn["time"]= i;
    else if (columnType[i]=="year")
      asciiColumn["year"] = i;
    else if (columnType[i]=="month")
      asciiColumn["month"] = i;
    else if (columnType[i]=="day")
      asciiColumn["day"] = i;
    else if (columnType[i]=="hour")
      asciiColumn["hour"] = i;
    else if (columnType[i]=="min")
      asciiColumn["min"] = i;
    else if (columnType[i]=="sec")
      asciiColumn["sec"] = i;
    else if (miutil::to_lower(columnType[i])=="lon")
      asciiColumn["x"]= i;
    else if (miutil::to_lower(columnType[i])=="lat")
      asciiColumn["y"]= i;
    else if (miutil::to_lower(columnType[i])=="dd")
      asciiColumn["dd"]= i;
    else if (miutil::to_lower(columnType[i])=="ff")    //Wind speed in m/s
      asciiColumn["ff"]= i;
    else if (miutil::to_lower(columnType[i])=="ffk")   //Wind speed in knots
      asciiColumn["ff"]= i;
    else if (miutil::to_lower(columnType[i])=="image")
      asciiColumn["image"]= i;
    else if (miutil::to_lower(columnName[i])=="lon" &&  //Obsolete
        columnType[i]=="r")
      asciiColumn["x"]= i;
    else if (miutil::to_lower(columnName[i])=="lat" &&  //Obsolete
        columnType[i]=="r")
      asciiColumn["y"]= i;
    else if (miutil::to_lower(columnName[i])=="dd" &&   //Obsolete
        columnType[i]=="r")
      asciiColumn["dd"]= i;
    else if (miutil::to_lower(columnName[i])=="ff" &&    //Obsolete
        columnType[i]=="r")
      asciiColumn["ff"]= i;
    else if (miutil::to_lower(columnName[i])=="ffk" &&    //Obsolete
        columnType[i]=="r")
      asciiColumn["ff"]= i;
    else if (miutil::to_lower(columnName[i])=="image" && //Obsolete
        columnType[i]=="s")
      asciiColumn["image"]= i;
    else if (miutil::to_lower(columnName[i])=="name" ||
        columnType[i]=="id")
      asciiColumn["Name"]= i;

    if (miutil::to_lower(columnType[i])=="ffk" ||
        miutil::to_lower(columnName[i])=="ffk")
      knots=true;

  }

//  if (!asciiColumn.count("x") || !asciiColumn.count("y")) {
//    //####################################################################
//    METLIBS_LOG_DEBUG("   bad header, missing lat,lon !!!!!!!!!");
//    //####################################################################
//    return;
//  }

  fileOK= true;
  return;


}

void ObsAscii::decodeData()
{
//  METLIBS_LOG_DEBUG(__FUNCTION__);
  // read data....................................................


  int nColumn= columnType.size();

  int nu= asciiColumnUndefined.size();

  bool useTime (asciiColumn.count("time") ||
      asciiColumn.count("hour"));
  bool isoTime (asciiColumn.count("time"));
  bool allTime (useTime &&
      (asciiColumn.count("date")  ||
          (asciiColumn.count("year") &&
              asciiColumn.count("month") &&
              asciiColumn.count("day"))));
  bool isoDate (allTime && asciiColumn.count("date"));

  bool first=true, addstr=false, cutstr=false;
  std::string taddstr, tstr, timestr;
  miTime obstime;

  miDate filedate= fileTime.date();

  //skip header
  size_t ii=0;
  while ( ii < lines.size() && not miutil::contains(lines[ii], "[DATA]")) {
    ++ii;
  }
  ++ii; //skip [DATA] line too

  int nskip= asciiSkipDataLines + ii;
  int nline= 0;

  for (size_t ii = 0; ii < lines.size(); ++ii ) {
    miutil::trim(lines[ii]);
    nline++;
    if (nline>nskip && (not lines[ii].empty()) && lines[ii][0]!='#') {

      //data structures
      vector<string> pstr;
      ObsData  obsData;

      if (not separator.empty()) {
        pstr= miutil::split(lines[ii], separator, false);
      } else {
        pstr= miutil::split_protected(lines[ii], '"', '"');
      }

      if ( int(pstr.size()) < nColumn ) {
        continue;
      }
      //put data in obsData
      for (int i=0; i<nColumn; i++) {

        //check if undefined value
        if (nu>0) {
          int iu= 0;
          while (iu<nu && pstr[i]!=asciiColumnUndefined[iu]) iu++;
          if (iu<nu) {
            continue;
          }
        }

        obsData.stringdata[columnName[i]] = pstr[i];
        if ( asciiColumn.count( "x") && asciiColumn["x"] == i ) {
          obsData.xpos = atof( pstr[i].c_str() );
        }
        if ( asciiColumn.count( "y") && asciiColumn["y"] == i ) {
          obsData.ypos = atof( pstr[i].c_str() );
        }
        if ( asciiColumn.count( "Name") && asciiColumn["Name"] == i ) {
          obsData.id = pstr[i];
        }
        if ( asciiColumn.count( "ff") && asciiColumn["ff"] == i ) {
          if ( knots ) {
            obsData.fdata["ff"] = ObsPlot::knots2ms(atof(pstr[i].c_str()));
          } else {
            obsData.fdata["ff"] = atof(pstr[i].c_str());
          }
        }
        if ( asciiColumn.count( "dd") && asciiColumn["dd"] == i ) {
          obsData.fdata["dd"] = atof(pstr[i].c_str());
        }
        if ( asciiColumn.count( "image") && asciiColumn["image"] == i ) {
          obsData.stringdata["image"] = pstr[i];
        }
      }

      if (useTime) {
        if(isoTime) {
          tstr= pstr[asciiColumn["time"]];
          if (first) {
            // allowed time formats: HH HH:MM HH:MM:SS HH:MM:SSxxx...
            vector<std::string> tv= miutil::split(tstr, 0, ":");
            if (tv.size()==1) {
              addstr= true;
              taddstr= ":00:00";
            } else if (tv.size()==2) {
              addstr= true;
              taddstr= ":00";
            } else if (tv.size()>=3 && tstr.length()>8) {
              cutstr= true;
            }
            first= false;
          }
          if (addstr)
            tstr+=taddstr;
          else if (cutstr)
            tstr= tstr.substr(0,8);
        } else {
          tstr = pstr[asciiColumn["hour"]] + ":";
          if(asciiColumn.count("min"))
            tstr += pstr[asciiColumn["min"]] + ":";
          else
            tstr += "00:";
          if(asciiColumn.count("sec"))
            tstr += pstr[asciiColumn["sec"]] ;
          else
            tstr += "00";
        }

        if (allTime) {
          if (isoDate)
            timestr= pstr[asciiColumn["date"]] +" "+ tstr;
          else
            timestr= pstr[asciiColumn["year"]] + "-"
            + pstr[asciiColumn["month"]] + "-"
            + pstr[asciiColumn["day"]] + " "+ tstr;
          obstime= miTime(timestr);
        } else {
          miClock clock= miClock(tstr);
          obstime= miTime(filedate,clock);
          int mdiff= miTime::minDiff(obstime,fileTime);
          if      (mdiff<-12*60) obstime.addHour(24);
          else if (mdiff> 12*60) obstime.addHour(-24);
        }

        //#################################################################
//                  METLIBS_LOG_DEBUG("obstime:"<<obstime);
//                 METLIBS_LOG_DEBUG("plotTime:"<<plotTime);
//                 METLIBS_LOG_DEBUG("timeDiff"<<timeDiff);
//                  if (timeDiff < 0 || abs(miTime::minDiff(obstime,plotTime))<timeDiff)
//                    METLIBS_LOG_DEBUG(obstime<<" ok");
//                  else
//                    METLIBS_LOG_DEBUG(obstime<<" not ok");
        //#################################################################
        if (timeDiff <0
            || abs(miTime::minDiff(obstime,plotTime))<timeDiff){
          //            oplot->asciip.push_back(pstr);
          obsData.obsTime = obstime;
          vObsData.push_back(obsData);
          mObsData[obsData.id] = obsData;
        } else {
          //oplot->removeObs();
        }


      } else {
        vObsData.push_back(obsData);
        mObsData[obsData.id] = obsData;
      }
    }
    //####################################################################
    //    METLIBS_LOG_DEBUG("----------- at end -----------------------------");
    //    METLIBS_LOG_DEBUG("     columnName.size()= "<<columnName.size());
    //    METLIBS_LOG_DEBUG("     columnType.size()= "<<columnType.size());
    //    METLIBS_LOG_DEBUG("------------------------------------------------");
    //####################################################################


//    fileOK = (oplot->numPositions()>0);
  }
}
