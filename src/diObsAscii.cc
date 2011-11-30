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

#include <fstream>
#include <iostream>
#include <diObsAscii.h>
#include <diObsPlot.h>
#include <vector>
#include <curl/curl.h>

using namespace::miutil;

ObsAscii::ObsAscii(const miString &filename, const miString &headerfile,
    const vector<miutil::miString> headerinfo, const miTime &filetime,
    ObsPlot *oplot, bool readData)
{
  fileOK = false;
  knots = false;
  readFile(filename,headerfile, headerinfo, filetime,oplot,readData );
}

void ObsAscii::getFromFile(const miutil::miString &filename, vector<miutil::miString>& lines)
{

  // open filestream
  ifstream file(filename.cStr());
  if (!file) {
    cerr << "ObsAscii: " << filename << " not found" << endl;
    return;
  }

  miString str;
  while (getline(file, str)) {
    lines.push_back(str);
  }

  file.close();

}

size_t write_dataa(void *buffer, size_t size, size_t nmemb, void *userp)
{
  string tmp =(char *)buffer;
  (*(string*) userp)+=tmp.substr(0,nmemb);
  return (size_t)(size *nmemb);
}



void ObsAscii::getFromHttp(const miutil::miString &url, vector<miutil::miString>& lines)
{

  CURL *curl = NULL;
  CURLcode res;
  string data;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_dataa);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
  }

  miString mdata=data;
  vector<miString> result;
  result = mdata.split("\n");
  lines.insert(lines.end(),result.begin(),result.end());

}

void ObsAscii::readFile(const miString &filename, const miString &headerfile,
    const vector<miutil::miString> headerinfo, const miTime &filetime,
    ObsPlot *oplot, bool readData)
{

  //####################################################################
//  cerr<<"ObsAscii::readFile  filename= "<<filename
//      <<"   filetime= "<<filetime<<endl;
//  cerr <<"Headerfiles:"<<headerfile<<endl;
//  for(size_t i = 0; i < headerinfo.size(); i++) {
//      cerr << "headerinfo: " << headerinfo[i]<<endl;
//    }
  //####################################################################

  vector<miString> lines;

  // open filestream
  if ( !headerfile.empty() ) {
    if (headerfile.contains("http")) {
      getFromHttp(headerfile,lines);
    } else {
      getFromFile(headerfile, lines);
    }
  } else if (headerinfo.size() > 0 ) {
    lines = headerinfo;
  }

  if (headerfile.empty() || readData) {
    if (filename.contains("http")) {
      getFromHttp(filename, lines);
    } else {
      getFromFile(filename, lines);
    }
  }

  decodeHeader(oplot, lines);

  if ( readData ) {
    decodeData(oplot, lines, filetime);
  }
}

void ObsAscii::decodeHeader(ObsPlot *oplot, vector<miutil::miString> lines)
{
//cerr <<__FUNCTION__<<endl;
  vector<miString> vstr,pstr;
  miString str;
  size_t p;

  for (size_t i = 0; i < lines.size(); ++i ) {
    lines[i].trim();
    if (lines[i].exists()) {
      p= lines[i].find('#');
      if (p==string::npos) {
        if (lines[i]=="[DATA]") break;  // end of header, start data
        vstr.push_back(lines[i]);
      } else if (p>0) {
        pstr= lines[i].split("#");
        if (pstr[0]=="[DATA]") break;  // end of header, start data
        vstr.push_back(pstr[0]);
      }
    }
  }



  fileOK=   false;
  asciiColumn.clear();
  asciiSkipDataLines= 0;

  //oplot->asciip.clear();
  oplot->columnName.clear();
  oplot->columnTooltip.clear();
  oplot->columnType.clear();
  asciiColumnUndefined.clear();

  // parse header

  const miString key_columns=   "COLUMNS";
  const miString key_undefined= "UNDEFINED";
  const miString key_skiplines= "SKIP_DATA_LINES";
  const miString key_label=     "LABEL";
  const miString key_splitchar=     "SEPARATOR";

  bool ok= true;
  int n= vstr.size();
  //####################################################################
//  cerr<<"HEADER:"<<endl;
//  for (int  j=0; j<n; j++)
//    cerr<<vstr[j]<<endl;
//  cerr<<"-----------------"<<endl;
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
      pstr= str.split('"','"');
      int j,m= pstr.size();

      if (m>1) {
        if (pstr[0]==key_columns) {
          if ( separator.exists()  ) {
            pstr= str.split(separator);
          }
          vector<miString> vs;
          m=pstr.size();
          for (j=1; j<m; j++) {
            pstr[j].remove('"');
            vs= pstr[j].split(':');
            if (vs.size()>1) {
              oplot->columnName.push_back(vs[0]);
              oplot->columnType.push_back(vs[1].downcase());
              if (vs.size()>2) {
                oplot->columnTooltip.push_back(vs[2]);
              }else{
                oplot->columnTooltip.push_back("");
              }
            }
          }
        } else if (pstr[0]==key_undefined ) {
          vector<miString> vs= pstr[1].split(',');
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
          asciiSkipDataLines= atoi(pstr[1].cStr());

        } else if (pstr[0]==key_label) {
          oplot->setLabel(str);

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
    cerr<<"   bad header !!!!!!!!!"<<endl;
    //####################################################################
    return;
  }

  n= oplot->columnType.size();
  //####################################################################
//  cerr<<"     coloumns= "<<n<<endl;
  //####################################################################

  knots=false;
  for (i=0; i<n; i++) {
    //####################################################################
//    cerr<<"   column "<<i<<" : "<<oplot->columnName[i]<<"  "
//        <<oplot->columnType[i]<<endl;
    //####################################################################
    if      (oplot->columnType[i]=="d")
      asciiColumn["date"]= i;
    else if (oplot->columnType[i]=="t")
      asciiColumn["time"]= i;
    else if (oplot->columnType[i]=="year")
      asciiColumn["year"] = i;
    else if (oplot->columnType[i]=="month")
      asciiColumn["month"] = i;
    else if (oplot->columnType[i]=="day")
      asciiColumn["day"] = i;
    else if (oplot->columnType[i]=="hour")
      asciiColumn["hour"] = i;
    else if (oplot->columnType[i]=="min")
      asciiColumn["min"] = i;
    else if (oplot->columnType[i]=="sec")
      asciiColumn["sec"] = i;
    else if (oplot->columnType[i].downcase()=="lon")
      asciiColumn["x"]= i;
    else if (oplot->columnType[i].downcase()=="lat")
      asciiColumn["y"]= i;
    else if (oplot->columnType[i].downcase()=="dd")
      asciiColumn["dd"]= i;
    else if (oplot->columnType[i].downcase()=="ff")    //Wind speed in m/s
      asciiColumn["ff"]= i;
    else if (oplot->columnType[i].downcase()=="ffk")   //Wind speed in knots
      asciiColumn["ff"]= i;
    else if (oplot->columnType[i].downcase()=="image")
      asciiColumn["image"]= i;
    else if (oplot->columnName[i].downcase()=="lon" &&  //Obsolete
        oplot->columnType[i]=="r")
      asciiColumn["x"]= i;
    else if (oplot->columnName[i].downcase()=="lat" &&  //Obsolete
        oplot->columnType[i]=="r")
      asciiColumn["y"]= i;
    else if (oplot->columnName[i].downcase()=="dd" &&   //Obsolete
        oplot->columnType[i]=="r")
      asciiColumn["dd"]= i;
    else if (oplot->columnName[i].downcase()=="ff" &&    //Obsolete
        oplot->columnType[i]=="r")
      asciiColumn["ff"]= i;
    else if (oplot->columnName[i].downcase()=="ffk" &&    //Obsolete
        oplot->columnType[i]=="r")
      asciiColumn["ff"]= i;
    else if (oplot->columnName[i].downcase()=="image" && //Obsolete
        oplot->columnType[i]=="s")
      asciiColumn["image"]= i;
    else if (oplot->columnName[i].downcase()=="name" ||
        oplot->columnType[i]=="id")
      asciiColumn["Name"]= i;

    if (oplot->columnType[i].downcase()=="ffk" ||
        oplot->columnName[i].downcase()=="ffk")
      knots=true;

  }

  if (!asciiColumn.count("x") || !asciiColumn.count("y")) {
    //####################################################################
    cerr<<"   bad header, missing lat,lon !!!!!!!!!"<<endl;
    //####################################################################
    return;
  }

  fileOK= true;
  return;


}

void ObsAscii::decodeData(ObsPlot *oplot, vector<miutil::miString> lines, const miutil::miTime &filetime)
{
//  cerr <<__FUNCTION__<<endl;
  // read data....................................................

  miTime tplot= oplot->getObsTime();
  int    tdiff= oplot->getTimeDiff() + 1;

  int nColumn= oplot->columnType.size();

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
  miString taddstr, tstr, timestr;
  miTime obstime;

  miDate filedate= filetime.date();

  //skip header
  size_t ii=0;
  while ( ii < lines.size() && !lines[ii].contains("[DATA]")) {
    ++ii;
  }
  ++ii; //skip [DATA] line too

  int nskip= asciiSkipDataLines + ii;
  int nline= 0;

  for (size_t ii = 0; ii < lines.size(); ++ii ) {

    lines[ii].trim();
    nline++;
    if (nline>nskip && lines[ii].exists() && lines[ii][0]!='#') {

      //data structures
      vector<miString> pstr;
      ObsData & obsData = oplot->getNextObs();

      if ( separator.exists() ) {
        pstr= lines[ii].split(separator, false);
      } else {
        pstr= lines[ii].split('"','"');
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

        obsData.stringdata[oplot->columnName[i]] = pstr[i];
        if ( asciiColumn["x"] == i ) {
          obsData.xpos = atof( pstr[i].c_str() );
        }
        if ( asciiColumn["y"] == i ) {
          obsData.ypos = atof( pstr[i].c_str() );
        }
        if ( asciiColumn["Name"] == i ) {
          obsData.id = pstr[i];
        }
        if ( asciiColumn["ff"] == i ) {
          if ( knots ) {
            obsData.fdata["ff"] = oplot->knots2ms(atof(pstr[i].c_str()));
          } else {
            obsData.fdata["ff"] = atof(pstr[i].c_str());
          }
        }
        if ( asciiColumn["dd"] == i ) {
          obsData.fdata["dd"] = atof(pstr[i].c_str());
        }
      }

      if (useTime) {
        if(isoTime) {
          tstr= pstr[asciiColumn["time"]];
          if (first) {
            // allowed time formats: HH HH:MM HH:MM:SS HH:MM:SSxxx...
            vector<miString> tv= tstr.split(':');
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
          int mdiff= miTime::minDiff(obstime,filetime);
          if      (mdiff<-12*60) obstime.addHour(24);
          else if (mdiff> 12*60) obstime.addHour(-24);
        }

        //#################################################################
        //          cerr <<"obstime:"<<obstime<<endl;
        //          cerr <<"tplot:"<<tplot<<endl;
        //          if (abs(miTime::minDiff(obstime,tplot))<tdiff)
        //            cerr<<obstime<<" ok"<<endl;
        //          else
        //            cerr<<obstime<<" not ok"<<endl;
        //#################################################################
        if (oplot->getTimeDiff() <0
            || abs(miTime::minDiff(obstime,tplot))<tdiff){
          //            oplot->asciip.push_back(pstr);
          obsData.obsTime = obstime;
        } else {
          oplot->removeObs();
        }


      }
    }
    //####################################################################
    //    cerr<<"----------- at end -----------------------------"<<endl;
    //    cerr<<"     oplot->columnName.size()= "<<oplot->columnName.size()<<endl;
    //    cerr<<"     oplot->columnType.size()= "<<oplot->columnType.size()<<endl;
    //    cerr<<"------------------------------------------------"<<endl;
    //####################################################################


    fileOK = (oplot->numPositions()>0);
  }
}
