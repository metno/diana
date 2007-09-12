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
#include <fstream>
#include <iostream>
#include <diObsAscii.h>
#include <diObsPlot.h>
#include <vector>


ObsAscii::ObsAscii(const miString &filename, const miString &headerfile,
		   const miTime &filetime, ObsPlot *oplot, bool readData)
{
  readFile(filename,headerfile,filetime,oplot,readData );
}


void ObsAscii::readFile(const miString &filename, const miString &headerfile,
			const miTime &filetime, ObsPlot *oplot, bool readData)
{
//####################################################################
//  cerr<<"ObsAscii::readFile  filename= "<<filename
//      <<"   filetime= "<<filetime<<endl;
//####################################################################
  int n,i;
  vector<miString> vstr,pstr;
  miString str;

  // open filestream
  ifstream file;
  if (headerfile.empty() || oplot->asciiHeader) {
    file.open(filename.c_str());
    if (file.bad()) {
      cerr << "ObsAscii: " << filename << " not found" << endl;
      return;
    }
  } else if (!oplot->asciiHeader) {
    file.open(headerfile.c_str());
    if (file.bad()) {
      cerr << "ObsAscii: " << headerfile << " not found" << endl;
      return;
    }
  }


  if (!oplot->asciiHeader || headerfile.empty()) {
    // read header
    size_t p;

    while (getline(file,str)) {
      str.trim();
      if (str.exists()) {
        p= str.find('#');
        if (p==string::npos) {
          if (str=="[DATA]") break;  // end of header, start data
	  vstr.push_back(str);
        } else if (p>0) {
          pstr= str.split("#");
          if (pstr[0]=="[DATA]") break;  // end of header, start data
	  vstr.push_back(pstr[0]);
        }
      }
    }
  }
//####################################################################
//  cerr<<"----------- at start -----------------------------"<<endl;
//  cerr<<"   oplot->asciip.size()= "<<oplot->asciip.size()<<endl;
//  cerr<<"     oplot->dateAscii= "<<oplot->dateAscii<<endl;
//  cerr<<"     oplot->timeAscii= "<<oplot->timeAscii<<endl;
//  cerr<<"     oplot->xAscii=    "<<oplot->xAscii<<endl;
//  cerr<<"     oplot->yAscii=    "<<oplot->yAscii<<endl;
//  cerr<<"     oplot->ddAscii=   "<<oplot->ddAscii<<endl;
//  cerr<<"     oplot->ffAscii=   "<<oplot->ffAscii<<endl;
//  cerr<<"     oplot->asciiColumnName.size()= "<<oplot->asciiColumnName.size()<<endl;
//  cerr<<"     oplot->asciiColumnType.size()= "<<oplot->asciiColumnType.size()<<endl;
//  cerr<<"     oplot->asciiHeader= "<<oplot->asciiHeader<<endl;
//  cerr<<"--------------------------------------------------"<<endl;
//####################################################################


  if (!oplot->asciiHeader) {

    oplot->asciiOK=   false;
    oplot->asciiColumn.clear();
    oplot->asciiSkipDataLines= 0;
 
    oplot->asciip.clear();
    oplot->asciiColumnName.clear();
    oplot->asciiColumnType.clear();
    oplot->asciiColumnHide.clear();
    oplot->asciiColumnUndefined.clear();
    oplot->asciiLengthMax.clear();
    bool useLabel = false; 

    // parse header

//     const miString key_name=      "NAME";
//     const miString key_mainTime=  "MAINTIME";
//     const miString key_startTime= "STARTTIME";
//     const miString key_endTime=   "ENDTIME";
    const miString key_columns=   "COLUMNS";
//     const miString key_hide=      "HIDE";
    const miString key_undefined= "UNDEFINED";
    const miString key_skiplines= "SKIP_DATA_LINES";
    const miString key_label=     "LABEL";
  //const miString key_plot=      "PLOT";
  //const miString key_format=    "FORMAT";

    bool ok= true;
    n= vstr.size();
//####################################################################
//cerr<<"HEADER:"<<endl;
//for (int j=0; j<n; j++)
//  cerr<<vstr[j]<<endl;
//cerr<<"-----------------"<<endl;
//####################################################################
    size_t p1,p2;
    i= 0;

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
        pstr= str.split(" ");
        int j,m= pstr.size();

        if (m>1) {
// 	  if (pstr[0]==key_name) {
//             oplot->asciiDataName= pstr[1];
// 	  } else if (pstr[0]==key_mainTime && m>2) {
//             miString tstr= pstr[1] + " " + pstr[2];
// 	    oplot->asciiMainTime= miTime(tstr);
// 	  } else if (pstr[0]==key_startTime && m>2) {
//             miString tstr= pstr[1] + " " + pstr[2];
// 	    oplot->asciiStartTime= miTime(tstr);
// 	  } else if (pstr[0]==key_endTime) {
//             miString tstr= pstr[1] + " " + pstr[2];
// 	    oplot->asciiEndTime= miTime(tstr);
// 	  } else if (pstr[0]==key_columns) {
	    if (pstr[0]==key_columns) {
	    vector<miString> vs;
            for (j=1; j<m; j++) {
	      vs= pstr[j].split(':');
	      if (vs.size()>1) {
	        oplot->asciiColumnName.push_back(vs[0]);
	        oplot->asciiColumnType.push_back(vs[1].downcase());
	      }
            }
// 	  } else if (pstr[0]==key_hide) {
//             for (j=1; j<m; j++)
// 	      oplot->asciiColumnHide.push_back(pstr[j]);
	  } else if (pstr[0]==key_undefined && m>1) {
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
	      oplot->asciiColumnUndefined.push_back(vs[jmax]);
            }
	  } else if (pstr[0]==key_skiplines && m>1) {
	    oplot->asciiSkipDataLines= atoi(pstr[1].cStr());

	  } else if (pstr[0]==key_label) {
	    oplot->setLabel(str);
	    useLabel=true;
	//} else if (pstr[0]==key_plot) {

	//} else if (pstr[0]==key_format) {

          }
        }
      } else {
        i++;
      }
    }

    if (!ok) {
//####################################################################
//    cerr<<"   bad header !!!!!!!!!"<<endl;
//####################################################################
      file.close();
      return;
    }

    n= oplot->asciiColumnType.size();
//####################################################################
//  cerr<<"     coloumns= "<<n<<endl;
//####################################################################

    oplot->asciiKnots=false;
    for (i=0; i<n; i++) {
//####################################################################
//cerr<<"   column "<<i<<" : "<<oplot->asciiColumnName[i]<<"  "
//		            <<oplot->asciiColumnType[i]<<endl;
//####################################################################
      if      (oplot->asciiColumnType[i]=="d")
        oplot->asciiColumn["date"]= i;
      else if (oplot->asciiColumnType[i]=="t")
        oplot->asciiColumn["time"]= i;
      else if (oplot->asciiColumnType[i]=="year")
        oplot->asciiColumn["year"] = i;
      else if (oplot->asciiColumnType[i]=="month")
        oplot->asciiColumn["month"] = i;
      else if (oplot->asciiColumnType[i]=="day")
         oplot->asciiColumn["day"] = i;
      else if (oplot->asciiColumnType[i]=="hour")
         oplot->asciiColumn["hour"] = i;
      else if (oplot->asciiColumnType[i]=="min")
        oplot->asciiColumn["min"] = i;
      else if (oplot->asciiColumnType[i]=="sec")
        oplot->asciiColumn["sec"] = i;
      else if (oplot->asciiColumnType[i].downcase()=="lon")
        oplot->asciiColumn["x"]= i;
      else if (oplot->asciiColumnType[i].downcase()=="lat")
        oplot->asciiColumn["y"]= i;
      else if (oplot->asciiColumnType[i].downcase()=="dd")
        oplot->asciiColumn["dd"]= i;
      else if (oplot->asciiColumnType[i].downcase()=="ff")    //Wind speed in m/s
        oplot->asciiColumn["ff"]= i;
      else if (oplot->asciiColumnType[i].downcase()=="ffk")   //Wind speed in knots
        oplot->asciiColumn["ff"]= i;
      else if (oplot->asciiColumnType[i].downcase()=="image")
        oplot->asciiColumn["image"]= i;
      else if (oplot->asciiColumnName[i].downcase()=="lon" &&  //Obsolete
	       oplot->asciiColumnType[i]=="r")                
        oplot->asciiColumn["x"]= i;                           
      else if (oplot->asciiColumnName[i].downcase()=="lat" &&  //Obsolete
	       oplot->asciiColumnType[i]=="r")                
        oplot->asciiColumn["y"]= i;                           
      else if (oplot->asciiColumnName[i].downcase()=="dd" &&   //Obsolete
	       oplot->asciiColumnType[i]=="r")                 
        oplot->asciiColumn["dd"]= i;
      else if (oplot->asciiColumnName[i].downcase()=="ff" &&    //Obsolete
	       oplot->asciiColumnType[i]=="r")
        oplot->asciiColumn["ff"]= i;
      else if (oplot->asciiColumnName[i].downcase()=="ffk" &&    //Obsolete
	       oplot->asciiColumnType[i]=="r")
        oplot->asciiColumn["ff"]= i;
      else if (oplot->asciiColumnName[i].downcase()=="image" && //Obsolete
	       oplot->asciiColumnType[i]=="s")
        oplot->asciiColumn["image"]= i;

      if (oplot->asciiColumnType[i].downcase()=="ffk" ||
	  oplot->asciiColumnName[i].downcase()=="ffk") 
	oplot->asciiKnots=true;  

    }

    for (i=0; i<n; i++)
      oplot->asciiLengthMax.push_back(0);

    if (!oplot->asciiColumn.count("x") || !oplot->asciiColumn.count("y")) {
//####################################################################
//    cerr<<"   bad header, missing lat,lon !!!!!!!!!"<<endl;
//####################################################################
      file.close();
      return;
    }

    if (!readData) {
      file.close();
      oplot->asciiOK= true;
      return;
    }

    if(!useLabel) //if there are labels, the header must be read each time
      oplot->asciiHeader = true;

    if (headerfile.exists()) {
      file.close();
      file.open(filename.c_str());
      if (file.bad()) {
        cerr << "ObsAscii: " << filename << " not found" << endl;
        return;
      }
    }

  }

  // read data....................................................

  miTime tplot= oplot->getObsTime();
  int    tdiff= oplot->getTimeDiff() + 1;

  n= oplot->asciiColumnType.size();

  int nu= oplot->asciiColumnUndefined.size();

  bool useTime (oplot->asciiColumn.count("time") || 
		oplot->asciiColumn.count("hour"));
  bool isoTime (oplot->asciiColumn.count("time"));
  bool allTime (useTime && 
		(oplot->asciiColumn.count("date")  || 
		 (oplot->asciiColumn.count("year") && 
		  oplot->asciiColumn.count("month") && 
		  oplot->asciiColumn.count("day"))));
  bool isoDate (allTime && oplot->asciiColumn.count("date"));

  bool first=true, addstr=false, cutstr=false;
  miString taddstr, tstr, timestr;
  miTime obstime;

  miDate filedate= filetime.date();

  int nskip= oplot->asciiSkipDataLines;
  int nline= 0;

  while (getline(file,str)) {
    str.trim();
    nline++;
    if (nline>nskip && str.exists() && str[0]!='#') {
      pstr= str.split('"','"');
      if (pstr.size()>=n) {
	if (nu>0) {
	  for (i=0; i<n; i++) {
	    int iu= 0;
            while (iu<nu && pstr[i]!=oplot->asciiColumnUndefined[iu]) iu++;
	    if (iu<nu) pstr[i]="X";
	  }
	}

	if (useTime) {
	  if(isoTime) {
	    tstr= pstr[oplot->asciiColumn["time"]];
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
	    tstr = pstr[oplot->asciiColumn["hour"]] + ":";
	    if(oplot->asciiColumn.count("min")) 
	      tstr += pstr[oplot->asciiColumn["min"]] + ":";
	    else 
	      tstr += "00:";
	    if(oplot->asciiColumn.count("sec")) 
	      tstr += pstr[oplot->asciiColumn["sec"]] ;
	    else 
	      tstr += "00";
	  }

	  if (allTime) {
	    if (isoDate) 
	      timestr= pstr[oplot->asciiColumn["date"]] +" "+ tstr;
	    else 
	      timestr= pstr[oplot->asciiColumn["year"]] + "-" 
		+ pstr[oplot->asciiColumn["month"]] + "-" 
		+ pstr[oplot->asciiColumn["day"]] + " "+ tstr;
	    obstime= miTime(timestr);
	  } else {
	    miClock clock= miClock(tstr);
	    obstime= miTime(filedate,clock);
	    int mdiff= miTime::minDiff(obstime,filetime);
	    if      (mdiff<-12*60) obstime.addHour(24);
	    else if (mdiff> 12*60) obstime.addHour(-24);
	  }
	
//#################################################################
//  if (abs(miTime::minDiff(obstime,tplot))<tdiff)
//    cerr<<obstime<<" ok"<<endl;
//  else
//    cerr<<obstime<<" not ok"<<endl;
//#################################################################
	  if (oplot->getTimeDiff() <0 
	      || abs(miTime::minDiff(obstime,tplot))<tdiff){
	    oplot->asciip.push_back(pstr);
	    oplot->asciiTime.push_back(obstime);
	  }
	  
	} else {
	  oplot->asciip.push_back(pstr);
	}
	for (i=0; i<n; i++) {
	  if (oplot->asciiLengthMax[i]<pstr[i].length())
	      oplot->asciiLengthMax[i]=pstr[i].length();
	}
      }
    }
  }
//####################################################################
//  cerr<<"----------- at end -----------------------------"<<endl;
//  cerr<<"   oplot->asciip.size()= "<<oplot->asciip.size()<<endl;
//  cerr<<"     oplot->dateAscii= "<<oplot->dateAscii<<endl;
//  cerr<<"     oplot->timeAscii= "<<oplot->timeAscii<<endl;
//  cerr<<"     oplot->xAscii=    "<<oplot->xAscii<<endl;
//  cerr<<"     oplot->yAscii=    "<<oplot->yAscii<<endl;
//  cerr<<"     oplot->ddAscii=   "<<oplot->ddAscii<<endl;
//  cerr<<"     oplot->ffAscii=   "<<oplot->ffAscii<<endl;
//  cerr<<"     oplot->asciiColumnName.size()= "<<oplot->asciiColumnName.size()<<endl;
//  cerr<<"     oplot->asciiColumnType.size()= "<<oplot->asciiColumnType.size()<<endl;
//  cerr<<"     oplot->asciiHeader= "<<oplot->asciiHeader<<endl;
//  cerr<<"------------------------------------------------"<<endl;
//####################################################################

  file.close();

  oplot->asciiOK= (oplot->asciip.size()>0);
}
