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
#include <diObsPlot.h>
#include <diFontManager.h>
#include <diImageGallery.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdio.h>

using namespace std;

//  static members
vector<float> ObsPlot::xUsed;
vector<float> ObsPlot::yUsed;
vector<ObsPlot::UsedBox> ObsPlot::usedBox;
map< miString,vector<miString> > ObsPlot::visibleStations;
map<miString,ObsPlot::metarww> ObsPlot::metarMap;
map<int,int> ObsPlot::lwwg2;

miString ObsPlot::currentPriorityFile= "";
vector<miString> ObsPlot::priorityList;
short * ObsPlot::itabSynop=  0;
short * ObsPlot::iptabSynop= 0;
short * ObsPlot::itabMetar=  0;
short * ObsPlot::iptabMetar= 0;


// Default constructor
ObsPlot::ObsPlot()
  :Plot(){
  x           = NULL;
  y           = NULL;
  PI          = acosf(-1.0);
  Scale       = 1;
  undef       = -32767;    //should be defined elsewhere
  allObs      = false;
  levelAsField= false;
  level       = -10;
  leveldiff   = 0;
  plottype    = "synop";
  localTime   = false;
  priority    = false;
  density     = 1;
  numPar      = 0;
  tempPrecision = false;
  allAirepsLevels= false;
  vertical    = true;
  showpos     = false;
  devfield    = false;
  moretimes   = false;
  next        = false;
  previous    = false;
  thisObs     = false;
  current     = -1;
  firstplot   = true;
  beendisabled= false;
  itab        = 0;
  iptab       = 0;
  onlypos     = false;
  flaginfo    = false;
  parameterName = false;
  asciiHeader = false;
  pcriteria   = false; //plot criteria
  ccriteria   = false; //colour criteria
  tccriteria  = false; // total colour criteria
  knotParameters.insert("ff");
  knotParameters.insert("911ff");
  knotParameters.insert("fxfx");
  knotParameters.insert("fmfm");

}



// Destructor
ObsPlot::~ObsPlot(){
  delete[] x;
  delete[] y;
}





void ObsPlot::getObsAnnotation(miString &str, Colour &col)
{

  str = annotation;
  col = origcolour;

}

bool ObsPlot::getDataAnnotations(vector<miString>& anno)
{

  if (!enabled || (obsp.size()==0 && asciip.size()==0) || current<0)
    return false;

  float vectorAnnotationSize=30*fullrect.width()/pwidth*0.7;
  miString vectorAnnotationText=miString(2.5*current,2) + "m/s";
  int nanno = anno.size();
  for(int j=0; j<nanno; j++){
    if (anno[j].contains("arrow")){
      if(anno[j].contains("arrow="))continue;

      miString endString;
      miString startString;
      if(anno[j].contains(",")){
        size_t nn = anno[j].find_first_of(",");
        endString = anno[j].substr(nn);
        startString =anno[j].substr(0,nn);
      } else {
        startString =anno[j];
      }

      miString str  = "arrow=" + miString (vectorAnnotationSize)
	+ ",feather=true,tcolour=" + colour.Name() + endString;
      anno.push_back(str);
      str = "text=\" " + vectorAnnotationText + "\""
	+ ",tcolour=" + colour.Name() + endString ;
      anno.push_back(str);
    }
  }

  return true;
}

ObsData& ObsPlot::getNextObs()
{
  ObsData d;
  obsp.push_back(d);
  return obsp[obsp.size()-1];
}

void ObsPlot::updateLevel(const miString& dataType)
{

  if( level < -1) return; //no levels

  if(dataType == "ocea"){ //ocean
    if( levelAsField )            //from field
      level = getOceanDepth();
    if(level == -1){
      level = 0;            //default!!
      cerr<<"No sea level, using 0m"<<endl;
    }
  }else { //temp, pilot,aireps
    if ( levelAsField )            //from field
      level = getPressureLevel();
    if (level == -1) {
      if(dataType == "aireps")
	level = 500;            //default!!
      else
	level = 1000;            //default!!
      cerr<<"No pressure level, using "<<level<<" hPa"<<endl;
    }
  }

}

int ObsPlot::numPositions()
{
  if(obsp.size()>0)
    return obsp.size();

  return asciip.size();
}

void ObsPlot::clearModificationTime()
{
  fileNames.clear();
  modificationTime.clear();
}

void ObsPlot::setModificationTime(const miString& fname)
{
  struct stat buf;
  fileNames.push_back(fname);
  const char *path = fname.c_str();
  if( !stat(path,&buf) ){
    modificationTime.push_back((long)buf.st_ctime);
  }else {
    modificationTime.push_back(0);
  }

}

bool ObsPlot::updateObs()
{
  //returns true if update is needed
  //(one or more files are changed)
  int n = fileNames.size();
  for( int i=0; i<n; i++ ){

    const char *path = fileNames[i].c_str();
    struct stat buf;
    if( stat(path,&buf) ) {
//       cerr<<"Something is wrong"<<endl;
      return true;
    }
    if( modificationTime[i] != (long)buf.st_ctime) {
//       cerr <<fileNames[i]<<" has changed"<<endl;
      return true;
    }
  }
//   cerr<<"Nothing has changed"<<endl;
  return false; // no update needed

}

bool ObsPlot::prepare(const miString& pin)
{
#ifdef DEBUGPRINT
  cerr << "++ ObsPlot::prepare() ++" << endl;
#endif

  if( pin.size() > 0 ) //if there is an info string, save it.
    infostr = pin;     //if not, use last info string

  setPlotInfo(pin);

  //clear criteria lists
  pcriteria=false;
  ccriteria=false;
  tccriteria=false;
  plotcriteria.clear();
  colourcriteria.clear();
  totalcolourcriteria.clear();

  //Default
  poptions.fontname="BITMAPFONT";
  poptions.fontface="normal";

  vector<miString> tokens = infostr.split('"','"');
  int n = tokens.size();
  vector<miString> parameter;
  miString value,orig_value,key;

  for( int i=0; i<n; i++){
    vector<miString> stokens = tokens[i].split('=');
    if( stokens.size() > 1) {
      key        = stokens[0].downcase();
      orig_value = stokens[1];
      value      = stokens[1].downcase();
      if (key == "plot" ){
	vector<miString> vstr= value.split(":");
	value = vstr[0];
	if( value == "pressure"|| value == "trykk" //"trykk" is obsolete
	    || value == "list" || value == "enkel" //"enkel" is obsolete
	    || value == "tide" || value == "ocean"){
	  value = "list";
	}else if( value == "hqc_synop"){
	  value = "synop";
	  flaginfo = true;
	}else if( value == "hqc_list"){
	  value = "ascii";
	  flaginfo = true;
	} else if (value != "synop" && value != "metar"){
	  value = "ascii";
	}
	plottype = value;
      }
      else if (key == "data" ){
	datatypes = value.split(",");
      }
      else if (key == "parameter" ){
	parameter = value.split(',');
	numPar = parameter.size();
      }
      else if (key == "scale" )
	Scale = atof(value.c_str());
      else if (key == "density" ){
	if(value.downcase() == "allobs" )
	  allObs = true;
	else
	  density = atof(value.c_str());
      }
      else if (key == "priority"){
	priorityFile = orig_value;
	priority = true;
      }
      else if (key == "colour"  ){
	Colour c(value);
	origcolour = c;
      }
      else if (key == "devfield"  ){
	if( value == "true" ){
	  devfield = true;
	}
      }
      else if (key == "devcolour1"  ){
	Colour c(value);
	mslpColour1 = c;
      }
      else if (key == "devcolour2"  ){
	Colour c(value);
	mslpColour2 = c;
      }
      else if (key == "tempprecision"  ){
	if( value == "true" ){
	  tempPrecision=true;
	} else {
	  tempPrecision=false;
	}
      }
      else if (key == "parametername"  ){
	if( value == "true" ){
	  parameterName=true;
	} else {
	  parameterName=false;
	}
      }
      else if (key == "moretimes"  ){
	if( value == "true" )
	  moretimes=true;
	else
	  moretimes=false;
      }
      else if (key == "allairepslevels"  ){
	if( value == "true" )
	  allAirepsLevels= true;
	else
	  allAirepsLevels= false;
      }
      else if (key == "timediff" )
	if(value.downcase() == "alltimes")
	  timeDiff = -1;
	else
	  timeDiff = atoi(value.c_str());
      else if( key == "level" ){
	if(value=="asfield"){
	  levelAsField=true;
	  level = -1;
	} else
	  level = atoi(value.cStr());
      }
      else if( key == "leveldiff" ){
	leveldiff = atoi(value.cStr());
      }
      else if( key == "onlypos" )
	onlypos = true;
      else if( key == "image" )
	image = orig_value;
      else if( key == "showpos" )
	showpos = true;
      else if( key == "orientation" ){
	if(value == "horizontal")
	  vertical = false;
      }else if( key == "criteria" ){
	decodeCriteria(tokens[i]);
      }else if( key == "font" ){
	poptions.fontname=orig_value;
      }else if( key == "face" ){
	poptions.fontface=orig_value;
      }
    }
  }


  if (plottype=="ascii") {
    asciiParameter.clear();
    asciiWind= false;
    for (int i=0; i<numPar; i++) {
      asciiParameter.push_back(parameter[i].downcase());
      if (parameter[i].downcase()=="wind") asciiWind= true;
    }
  } else {
    miString all="all";
    parameterDecode(all,false);
    for (int i=0; i<numPar; i++)
      parameterDecode(parameter[i].downcase());
  }

  clearPos();

  // static tables, read once

  miString path = setup.basicValue("obsplotfilepath");

  if (plottype=="synop" || plottype=="list") {

    if (!itabSynop || !iptabSynop) {
      miString filename= path + "/synpltab.dat";
      if (!readTable(plottype,filename))
        return false;
      itabSynop=  itab;
      iptabSynop= iptab;
    }
    itab=  itabSynop;
    iptab= iptabSynop;

  } else if (plottype=="metar") {

    if (!itabMetar || !iptabMetar) {
      miString filename= path + "/metpltab.dat";
      if (!readTable(plottype,filename))
        return false;
      itabMetar=  itab;
      iptabMetar= iptab;
    }
    itab=  itabMetar;
    iptab= iptabMetar;

  } else {
    itab=  0;
    iptab= 0;
  }

#ifdef DEBUGPRINT
  cerr << "++ End ObsPlot prepare() ++" << endl;
#endif
  return true;
}


bool ObsPlot::setData(void)
{
#ifdef DEBUGPRINT
  cerr << "++ ObsPlot::setData() ++" << endl;
#endif

  int i;

  delete[] x;
  delete[] y;
  x = NULL;
  y = NULL;

  firstplot = true;
  nextplot.clear();
  notplot.clear();
  list_plotnr.clear();

  int numSynop = obsp.size();
  int numObs   = numSynop;


  if(asciip.size()>0){
    asciiData=true;
    numObs   = asciip.size();
  } else {
    asciiData = false;
  }

    if(numObs<1){
//       cerr <<"ObsPlot::setData: no data"<<endl;
      return false;
    }

  // fill point-arrays
  x= new float[numObs];
  y= new float[numObs];

  if (asciiData) {
    for (i=0; i<numObs; i++){
      x[i] = atof(asciip[i][asciiColumn["x"]].c_str());
      y[i] = atof(asciip[i][asciiColumn["y"]].c_str());
    }
    asciipar.clear();
    int nc= asciiColumnName.size();
    int np= asciiParameter.size();
//######################################################################
//    for (int c=0; c<nc; c++)
//      cerr<<"ASCII.PLOT  asciiColumnName: "<<asciiColumnName[c]<<endl;
//    for (int p=0; p<np; p++)
//      cerr<<"ASCII.PLOT  asciiParameter: "<<asciiParameter[p]<<endl;
//######################################################################
    for (int c=0; c<nc; c++) {
      miString cpar= asciiColumnName[c].downcase();
      int p= 0;
      while (p<np && asciiParameter[p]!=cpar) p++;
      if (p<np) asciipar.push_back(c);
//######################################################################
//       cerr<<"ASCII.PLOT  nc,np,c,p: "<<nc<<" "<<np<<" "<<c<<" "<<p<<endl;
//######################################################################
    }

  } else {
    for (i=0; i<numObs; i++){
      x[i] = obsp[i].xpos;
      y[i] = obsp[i].ypos;
    }
  }

  // convert points to correct projection
  gc.geo2xy(area,numObs,x,y);

  bool ddff= true;

  if (asciiData && (!asciiColumn.count("dd") || !asciiColumn.count("ff")))
    ddff= false;

  if (ddff) {

    // find direction of north for each observation
    float *u = new float[numObs];
    float *v = new float[numObs];

    for (i=0; i<numObs; i++){
      u[i]= 0;
      v[i]= 10;
    }

    gc.geov2xy(area,numObs,x,y,u,v);

    if (asciiData) {

      asciidd.resize(numObs);
      asciiff.resize(numObs);

      for (i=0; i<numObs; i++){
	float add= atof( asciip[i][asciiColumn["dd"]].c_str());
	float aff= atof( asciip[i][asciiColumn["ff"]].c_str());
	if(asciiKnots) aff = knots2ms(aff);
	asciiff[i]= int(aff + 0.5);
        if( add > 0.0 && add <= 360.0 ){
	  asciidd[i]= int(add + atan2f(u[i],v[i])*180/PI);
	  if( asciidd[i]<=0 ) asciidd[i] += 360;
	  if( asciidd[i]>360) asciidd[i] -= 360;
	} else {
	  asciidd[i]= -32767;
        }
      }
    } else {

      for (i=0; i<numObs; i++){
//##############################################################
//      if (obsp[i].dd<1 || obsp[i].dd>360 || obsp[i].ff<1)
//	cerr<<"DATA DD,FF "<<obsp[i].dd<<" "<<obsp[i].ff
//            <<" "<<obsp[i].id<<" "<<obsp[i].obsTime<<endl;
//##############################################################
        int angle = (int)(atan2f(u[i],v[i])*180/PI);
        if( obsp[i].fdata.count("dd") &&
	    obsp[i].fdata["dd"] > 0 && obsp[i].fdata["dd"] <= 360 ){
	  obsp[i].fdata["dd_orig"]=obsp[i].fdata["dd"];
	  float dd = obsp[i].fdata["dd"] + angle;
	  if( dd<1  ) dd += 360;
	  if( dd>360) dd -= 360;
	  obsp[i].fdata["dd"] = dd;
        }
        if( obsp[i].fdata.count("dw1dw1")){
	  float dd = obsp[i].fdata["dw1dw1"] + angle;
	  if( dd<1 )  dd += 360;
	  if( dd>360)  dd -= 360;
	  obsp[i].fdata["dw1dw1"] = dd;
        }

        if( obsp[i].fdata.count("ds") ){
	  float dd = obsp[i].fdata["ds"] + angle;
	  if( dd<1 ) dd += 360;
	  if( dd>360 ) dd -= 360;
	  obsp[i].fdata["ds"] = dd;
        }
      }
    }

    delete[] u;
    delete[] v;
  }

  //sort stations according to priority file
  priority_sort();
  //put stations plotted last time on top of list
  readStations();
  //sort stations according to time
  if(moretimes)
    time_sort();


  if( plottype == "metar" && metarMap.size()==0 )
    initMetarMap();

#ifdef DEBUGPRINT
  cerr << "Number of stations: "<<numObs<<endl;
//   cerr << "Font name: "<<poptions.fontname<<endl;
//   cerr << "Font face: "<<poptions.fontface<<endl;
  cerr << "++ End ObsPlot setData() ++" << endl;
#endif

  return true;
}

bool ObsPlot::timeOK(const miTime& t)
{

//if timediff == -1 :use all observations
//if not: use all observations with abs(obsTime-Time)<timediff

  if( timeDiff<0 || abs(miTime::minDiff(t,Time)) < timeDiff+1)
    return true;

  return false;

}

void ObsPlot::logStations()
{
  if(asciiData) return; // difficult to log because no "Id"

  int n=nextplot.size();
//    cerr <<"n:"<<n<<endl;
//    cerr <<"numObs"<<obsp.size()<<endl;
  if(n){
    visibleStations[plottype].clear();
    for(int i=0;i<n;i++)
      visibleStations[plottype].push_back(obsp[nextplot[i]].id);
  }

}

void ObsPlot::readStations()
{

// difficult to log asciidata because no "Id"
  if(asciiData) {
    all_stations = all_from_file;
    return;
  }

  int n= visibleStations[plottype].size();
  //    cerr <<"no. of vis. st.:"<<n<<endl;
    if (n>0) {

      vector<int> tmpList= all_from_file;
      all_stations.clear();

      // Fill the stations from stat into priority_vector,
      // and mark them in tmpList
      int i,j;
      int numObs=obsp.size();
      for (int k=0; k<numObs; k++) {
	i = all_from_file[k];
	j=0;
	while (j<n && visibleStations[plottype][j]!=obsp[i].id) j++;
	if (j<n) {
	  all_stations.push_back(i);
	  tmpList[i] = -1;
	}
      }

      for (i=0; i<tmpList.size(); i++)
        if(tmpList[i] != -1)
          all_stations.push_back(i);
    } else {
      all_stations = all_from_file;
    }


  firstplot = true;
  nextplot.clear();
  notplot.clear();
  list_plotnr.clear();
  fromFile=false;
  //  cerr <<"Leaving read_stations"<<endl;
}

void ObsPlot::clear()
{

  //  logStations();
  firstplot= true;
  nextplot.clear();
  notplot.clear();
  obsp.clear();
  asciip.clear();
  annotation.clear();
  plotname.clear();
  labels.clear();
}

void ObsPlot::priority_sort(void)
{

  //sort the observations according to priority list
  int numObs;
  if(obsp.size()>0)
    numObs = obsp.size();
  else
    numObs = asciip.size();
  //  cerr <<"Priority_sort:"<<numObs<<endl;
  int i;

  all_from_file.resize(numObs);

  // AF: synop: put automatic stations after other types (fixed,ship)
  //     (how to detect other obs. types, temp,aireps,... ???????)
  if (asciiData) {
    for (i=0; i<numObs; i++)
      all_from_file[i]=i;
  } else {
    vector<int> automat;
    int n= 0;
    for (i=0; i<numObs; i++) {
      if (obsp[i].fdata.count("ix") && obsp[i].fdata["ix"]<4)
	all_from_file[n++]= i;
      else
	automat.push_back(i);
    }
    int na= automat.size();
    for (i=0; i<na; i++)
      all_from_file[n++]= automat[i];
  }

  if (!asciiData && priority) {

    if (currentPriorityFile!=priorityFile)
      readPriorityFile(priorityFile);

    if (priorityList.size()>0) {

      vector<int> tmpList= all_from_file;
      all_from_file.clear();

      // Fill the stations from priority list into all_from_file,
      // and mark them in tmpList
      int j, n= priorityList.size();
      for (j=0; j<n; j++) {
	i= 0;
	while (i<numObs && obsp[i].id!=priorityList[j]) i++;
	if (i<numObs) {
	  all_from_file.push_back(i);
	  tmpList[i]= -1;
	}
      }

      for (i=0; i<tmpList.size(); i++)
        if(tmpList[i] != -1)
          all_from_file.push_back(i);
    }
  }
}
void ObsPlot::time_sort(void)
{

  //sort observations according to time
  //both all_from_file (sorted acc. to priority file) and
  // all_stations (stations from last plot on top) are sorted

  int i,index,numObs;

  //data from ascii-files
  if (asciiData) {
    if(asciiTime.size() == 0) return;
    numObs = asciiTime.size();
  } else {
    numObs = obsp.size();
  }

  vector<int> diff(numObs);
  multimap<int,int> sortmap1;
  multimap<int,int> sortmap2;

  if (asciiData) {
    // data from ascii-files
    for (i=0; i<numObs; i++)
      diff[i] = abs(miTime::minDiff(asciiTime[i],Time));
  } else {
    // Data from obs-files
    // find mindiff = abs(obsTime-plotTime) for all observations
    for (int i=0; i<numObs; i++)
      diff[i] = abs(miTime::minDiff(obsp[i].obsTime,Time));
  }


  //Sorting ...
  for (int i=0; i<numObs; i++) {
    index = all_stations[i];
    sortmap1.insert(pair<int,int>(diff[index],index));
  }
  for (int i=0; i<numObs; i++) {
    index = all_from_file[i];
    sortmap2.insert(pair<int,int>(diff[index],index));
  }

  multimap<int,int>::iterator p = sortmap1.begin();
  for (int i=0; i<numObs; i++,p++)
    all_stations[i] = p->second;

  multimap<int,int>::iterator q = sortmap2.begin();
  for (int i=0; i<numObs; i++,q++)
    all_from_file[i] = q->second;

}


void ObsPlot::readPriorityFile(const miString& filename)
{
  priorityList.clear();

  // this will prevent opening a nonexisting file many times
  currentPriorityFile= filename;

  ifstream inFile;
  miString line;

  inFile.open(filename.c_str(),ios::in);
  if (inFile.bad()) {
    cerr << "ObsPlot: Can't open file: " << priorityFile << endl;
    return;
  }

  while (getline(inFile,line)) {
    if (line.length()>0) {
      line.trim();
      if (line.length()>0 && line[0]!='#')
	priorityList.push_back(line);
    }
  }

  inFile.close();
}

//***********************************************************************

bool ObsPlot::getPositions(vector<float> &xpos, vector<float> &ypos)
{
  if(!devfield)
    return false;

  startxy = xpos.size();

  int numObs;
  if(obsp.size()>0)
    numObs = obsp.size();
  else
    numObs = asciip.size();

  for( int i=0; i<numObs; i++){
    xpos.push_back(x[i]);
    ypos.push_back(y[i]);
  }

  return true;
}


int ObsPlot::getPositions(float *xpos, float *ypos, int n)
{

  if(!devfield)
    return false;

  startxy = n;

  int numObs;
  if(obsp.size()>0)
    numObs = obsp.size();
  else
    numObs = asciip.size();

//   cerr<<"n:"<<n<<endl;
//   cerr<<"numObs:"<<numObs<<endl;

  for( int i=0; i<numObs; i++){
    xpos[i+n] = x[i];
    ypos[i+n] = y[i];
  }

  return (n+numObs);
}

//***********************************************************************

void ObsPlot::obs_mslp(float *values)
{

  //PPPP-mslp
  if(!devfield){
    plot();
    return;
  }

  int numObs = obsp.size();

  for(  int i=0; i<numObs; i++){
    if(obsp[i].fdata.count("PPPP") &&  values[i+startxy]<0.9e+35){
      obsp[i].fdata["PPPP_mslp"]=obsp[i].fdata["PPPP"]-values[i+startxy];
    }
  }

  plot();

  for(  int i=0; i<numObs; i++)
    obsp[i].fdata.erase("PPPP_mslp");

}

//***********************************************************************


bool ObsPlot::findObs(int xx, int yy)
{

  if( !showpos )
    return false;

  int n = notplot.size();
  vector<int>::iterator p=notplot.begin();;
  vector<int>::iterator min_p;
  float min_r= 10.0f*fullrect.width()/pwidth;
  min_r= powf(min_r,2);
  float r;
  int   min_i=-1;

  float xpos=xx*fullrect.width()/pwidth + fullrect.x1;
  float ypos=yy*fullrect.height()/pheight + fullrect.y1;

  //find closest station, closer than min_r, from list of stations not plotted
  for(int i=0; i<n; i++,p++){
    r=powf(xpos-x[notplot[i]],2) + powf(ypos-y[notplot[i]],2);
    if(r<min_r){
      min_r=r;
      min_i=i;
      min_p=p;
    }
  }

  if(min_i<0)
    return false;

  //if last station are closer, return false
  if (nextplot.size()) {
    r=powf(xpos-x[nextplot[0]],2) + powf(ypos-y[nextplot[0]],2);
    if(r<min_r)
      return false;
  }

  //insert station found in list of stations to plot, and remove it
  //from list of stations not to plot
  nextplot.insert(nextplot.begin(),notplot[min_i]);
  notplot.erase(min_p);
  thisObs = true;
  return true;

}

//***********************************************************************

bool ObsPlot::getObsName(int xx, int yy, miString& name)
{

  //  if(asciiData) return false;

  static miString lastName;
  float min_r= 10.0f*fullrect.width()/pwidth;
  min_r= powf(min_r,2);
  float r;
  int   min_i=-1;

  float xpos=xx*fullrect.width()/pwidth + fullrect.x1;
  float ypos=yy*fullrect.height()/pheight + fullrect.y1;

  int numObs;
  if(obsp.size()>0)
    numObs = obsp.size();
  else
    numObs = asciip.size();

  if(onlypos){
    for(int i=0; i<numObs; i++){
      r=powf(xpos-x[i],2) + powf(ypos-y[i],2);
      if(r<min_r){
	min_r=r;
	min_i=i;
      }
    }
    if(min_i < 0)
      return false;
  }else{
    int n = nextplot.size();
    for(int i=0; i<n; i++){
      r=powf(xpos-x[nextplot[i]],2) + powf(ypos-y[nextplot[i]],2);
      if(r<min_r){
	min_r=r;
	min_i=i;
      }
    }
    if(min_i < 0)
      return false;
    min_i=nextplot[min_i];
  }

  if(asciiData && asciip[min_i].size()>0) {
    name = asciip[min_i][0];
  } else {
    name = obsp[min_i].id;
  }

  if (name == lastName)
    return false;

  lastName=name;

  selectedStation = name;
  return true;

}

//***********************************************************************

void ObsPlot::nextObs(bool Next)
{

  thisObs  = false;

  if(Next){
    next = true;
    plotnr++;
  } else {
    previous = true;
    plotnr--;
  }
}


bool ObsPlot::plot(){
#ifdef DEBUGPRINT
  cerr << "++ ObsPlot::plot() ++" << endl;
#endif

  if (!enabled) {
    // make sure plot-densities etc are recalc. next time
    if (dirty) beendisabled= true;
    return false;
  }

  if(obsp.size()==0 && asciip.size()==0) {
    //         cerr << "ObsPlot::plot() - Nothing to plot" << endl;
    return false;
  }

  int numObs;
  if(obsp.size()>0)
    numObs = obsp.size();
  else
    numObs = asciip.size();

  Colour selectedColour= origcolour;
  if (origcolour==backgroundColour) origcolour= backContrastColour;
  glColor4ubv(origcolour.RGBA());

  if(Scale<1.75) glLineWidth(1);
  else           glLineWidth(2);

  fp->set(poptions.fontname,poptions.fontface,10*Scale);

  // fontsizeScale != 1 when postscript font size != X font size
  if (hardcopy)
    fontsizeScale = fp->getSizeDiv();
  else
    fontsizeScale = 1.0;

  scale= Scale*fullrect.width()/pwidth*0.7;

  //Plot markers only
  if (onlypos) {
     ImageGallery ig;
     ig.plotImages(numObs,image,x,y,true,Scale);
    return true;
  }

  //Circle
  GLfloat xc,yc;
  GLfloat radius=7.0;
  circle= glGenLists(1);
  glNewList(circle,GL_COMPILE);
  glBegin(GL_LINE_LOOP);
  if (plottype=="list" || plottype=="ascii") {
    float d= radius*0.25;
    glVertex2f(-d,-d);
    glVertex2f(-d, d);
    glVertex2f( d, d);
    glVertex2f( d,-d);
  } else {
    for(int i=0;i<100;i++){
      xc = radius*cos(i*2*PI/100.0);
      yc = radius*sin(i*2*PI/100.0);
      glVertex2f(xc,yc);
    }
  }
  glEnd();
  glEndList();


  int num=numPar;
  if (plottype=="ascii") {
    if (asciiWind)
      num--;
  } else {
    if( pFlag.count("pos") )
      num++;
    if( pFlag.count("wind") )
      num--;
  }

  float xdist,ydist;

  if (plottype == "synop" || plottype == "metar") {
    xdist = 100*scale/density;
    ydist = 90*scale/density;
  } else if (plottype == "list" || plottype == "ascii") {
    if (num>0) {
      if(vertical){
	xdist = 58*scale/density;
	ydist = 18*(num+0.2)*scale/density;
      } else {
	xdist = 50*num*scale/density;
	ydist = 10*scale/density;
      }
    } else {
      xdist = 14*scale/density;
      ydist = 14*scale/density;
    }
  }

  //**********************************************************************
  //Which stations to plot

  bool testpos= true;  // positionFree or areaFree must be tested
  vector<int> ptmp;
  vector<int>::iterator p,pbegin,pend;

  if (dirty || firstplot || beendisabled) { //new area

    //init of areaFreeSetup
    if (plottype=="list" || plottype=="ascii") {
      float w,h;
      fp->getStringSize("0",w,h);
      w*=fontsizeScale;
      float space= w*0.5;
      areaFreeSetup(scale,space,num,xdist,ydist);
    }

    thisObs = false;

    // new area, find stations inside current area
    all_this_area.clear();
    int nn=all_stations.size();
    for (int j=0; j<nn; j++) {
      int i = all_stations[j];
      if (maprect.isinside(x[i],y[i])){
	  all_this_area.push_back(i);
      }
    }

    //    cerr <<"all this area:"<<all_this_area.size()<<endl;
    // plot the observations from last plot if possible,
    // then the rest if possible

    if(!firstplot){
      vector<int> a,b;
      int n= list_plotnr.size();
      if (n==numObs) {
	int psize= all_this_area.size();
	for (int j=0; j<psize; j++) {
	  int i=all_this_area[j];
	  if(list_plotnr[i]==plotnr)
	    a.push_back(i);
	  else
	    b.push_back(i);
	}
	if (a.size()>0) {
	  all_this_area.clear();
	  all_this_area.insert(all_this_area.end(),a.begin(),a.end());
	  all_this_area.insert(all_this_area.end(),b.begin(),b.end());
	}
      }
    }

    //reset
    list_plotnr.clear();
    list_plotnr.insert(list_plotnr.begin(),numObs,-1);
    maxnr=plotnr=0;

    pbegin= all_this_area.begin();
    pend  = all_this_area.end();

  } else if (thisObs) {
    //    cerr <<"thisobs"<<endl;
    // plot the station pointed at and those plotted last time if possible,
    // then the rest if possible.
    ptmp   = nextplot;
    ptmp.insert(ptmp.end(),notplot.begin(),notplot.end());
    pbegin = ptmp.begin();
    pend   = ptmp.end();

  } else if (plotnr > maxnr) {
    //    cerr <<"plotnr:"<<plotnr<<endl;
    // plot as many observations as possible which have not been plotted before
    maxnr++;
    plotnr= maxnr;

    int psize= all_this_area.size();
    for (int j=0; j<psize; j++) {
      int i=all_this_area[j];
      if (list_plotnr[i]==-1)
        ptmp.push_back(i);
    }
    pbegin = ptmp.begin();
    pend   = ptmp.end();

  } else if (previous && plotnr<0) {
    //    cerr <<"plotnr:"<<plotnr<<endl;
    // should return to the initial priority as often as possible...

    if(!fromFile){ //if priority from last plot has been used so far
      all_this_area.clear();
      for (int j=0; j<numObs; j++) {
	int i = all_from_file[j];
	if (maprect.isinside(x[i],y[i]))
	  all_this_area.push_back(i);
      }
      all_stations=all_from_file;
      fromFile=true;
    }
    //clear
    plotnr= maxnr= 0;
    list_plotnr.clear();
    list_plotnr.insert(list_plotnr.begin(),numObs,-1);

    pbegin= all_this_area.begin();
    pend  = all_this_area.end();

  } else if(previous || next){
    //    cerr <<"plotnr:"<<plotnr<<endl;
    //    plot observations from plotnr
    int psize= all_this_area.size();
    notplot.clear();
    nextplot.clear();
    for (int j=0; j<psize; j++) {
      int i=all_this_area[j];
      if (list_plotnr[i] == plotnr) {
	nextplot.push_back(i);
      } else if (list_plotnr[i] > plotnr || list_plotnr[i]==-1) {
	notplot.push_back(i);
      }
    }
    testpos= false;  //no need to test positionFree or areaFree

  } else {
    //nothing has changed
    testpos= false;  //no need to test positionFree or areaFree
  }
//######################################################
//  int ubsize1= usedBox.size();
//######################################################

  if (testpos) {   //test of positionFree or areaFree
    notplot.clear();
    nextplot.clear();
    if (plottype == "list" || plottype == "ascii") {
      for (p=pbegin; p!=pend; p++) {
        int i= *p;
	if (allObs || areaFree(i)) {
	  if( checkPlotCriteria(i) ) {
	    nextplot.push_back(i);
	    list_plotnr[i]= plotnr;
	  } else{
	    list_plotnr[i]= -2;
	    if(usedBox.size()) usedBox.pop_back();
	  }
        } else {
	  notplot.push_back(i);
        }
      }
    } else {
      for (p=pbegin; p!=pend; p++) {
        int i= *p;
	if (allObs || positionFree(x[i],y[i],xdist,ydist)) {
	  if( checkPlotCriteria(i) ) {
	    nextplot.push_back(i);
	    list_plotnr[i]= plotnr;
	  } else{
	    list_plotnr[i]= -2;
	    if(xUsed.size()) xUsed.pop_back();
	    if(yUsed.size()) yUsed.pop_back();
	  }
        } else {
	  notplot.push_back(i);
        }
      }
    }
    if (thisObs) {
      int n= notplot.size();
      for (int i=0; i<n; i++)
	if (list_plotnr[notplot[i]]==plotnr)
	  list_plotnr[notplot[i]]= -1;
    }
  }

//PLOTTING

  if (showpos) {
    Colour col("red");
    if (col==colour) col= Colour("blue");
    if (col==backgroundColour) col= backContrastColour;
    glColor4ubv(col.RGBA());
    int m = notplot.size();
    float d= 4.5*scale;
    glBegin(GL_LINES);
    for (int i=0; i<m; i++) {
      int j=notplot[i];
      glVertex2f(x[j]-d,y[j]-d);
      glVertex2f(x[j]+d,y[j]+d);
      glVertex2f(x[j]-d,y[j]+d);
      glVertex2f(x[j]+d,y[j]-d);
    }
    glEnd();
    glColor4ubv(origcolour.RGBA());
  }

  int n = nextplot.size();

  if (plottype == "synop") {
    for (int i=0; i<n; i++) {
      plotSynop(nextplot[i]);
      if (i % 50 == 0) UpdateOutput();
    }

  } else if (plottype == "metar") {
    for (int i=0; i<n; i++) {
      plotMetar(nextplot[i]);
      if (i % 50 == 0) UpdateOutput();
    }

  } else if (plottype == "list") {
     for (int i=0; i<n; i++) {
       plotList(nextplot[i]);
       if (i % 50 == 0) UpdateOutput();
     }

  } else if (plottype == "ascii") {
    for (int i=0; i<n; i++) {
      plotAscii(nextplot[i]);
      if (i % 50 == 0) UpdateOutput();
    }
  }

  UpdateOutput();


  //reset
  glDeleteLists(circle,1);
  next     = false;
  previous = false;
  thisObs  = false;
  if (nextplot.empty()) plotnr=-1;
  origcolour   = selectedColour; // reset in case a background contrast colour was used
  firstplot    = false;
  beendisabled = false;

#ifdef DEBUGPRINT
  cerr << "++ Returning from ObsPlot::plot() ++" << endl;
#endif
  return true;
}


bool ObsPlot::positionFree(const float& x, const float& y,float dist){
  int n= xUsed.size();
  for( int i=0; i<n; i++ )
    if( (pow(x-xUsed[i],2)+pow(y-yUsed[i],2)) <dist)
      return false;
  xUsed.push_back(x);
  yUsed.push_back(y);
  return true;
}


bool ObsPlot::positionFree(float x, float y,float xdist,float ydist){
  int n= xUsed.size();
  for( int i=0; i<n; i++ )
    if( fabsf(x-xUsed[i])<xdist && fabsf(y-yUsed[i])<ydist){
      return false;
    }
  xUsed.push_back(x);
  yUsed.push_back(y);
  return true;
}


void ObsPlot::areaFreeSetup(float scale, float space, int num,
			    float xdist, float ydist) {

  areaFreeSpace= space;

  bool wind;
  if (plottype=="ascii")
    wind= asciiWind;
  else
    wind= pFlag.count("wind");

  if (wind) areaFreeWindSize= scale*47.;
  else      areaFreeWindSize= 0.0;

  if (num>0) {
    float d= space*0.5;
    areaFreeXsize= xdist + space;
    areaFreeYsize= ydist + space;
    areaFreeXmove[0]= -xdist-d;
    areaFreeXmove[1]= -d;
    areaFreeXmove[2]= -d;
    areaFreeXmove[3]= -xdist-d;
    areaFreeYmove[0]= -d;
    areaFreeYmove[1]= -d;
    areaFreeYmove[2]= -ydist-d;
    areaFreeYmove[3]= -ydist-d;
  } else {
    areaFreeXsize= 0.0;
    areaFreeYsize= 0.0;
  }
}


bool ObsPlot::areaFree(int idx) {

  float xc= x[idx];
  float yc= y[idx];

  UsedBox ub[2];
  int idd,nb=0;

  int pos=1;

  if (areaFreeWindSize>0.0) {
    if (plottype=="ascii")
      idd= asciidd[idx];
    else if( obsp[idx].fdata.count("dd"))
      idd= (int)obsp[idx].fdata["dd"];
    if (idd>0 && idd<361) {
      float dd= float(idd)*PI/180.;
      float xw= areaFreeWindSize * sin(dd);
      float yw= areaFreeWindSize * cos(dd);
      if (xw<0.) {
        ub[0].x1= xc+xw-areaFreeSpace;
        ub[0].x2= xc+areaFreeSpace;
      } else {
        ub[0].x1= xc-areaFreeSpace;
        ub[0].x2= xc+xw+areaFreeSpace;
      }
      if (yw<0.) {
        ub[0].y1= yc+yw-areaFreeSpace;
        ub[0].y2= yc+areaFreeSpace;
      } else {
        ub[0].y1= yc-areaFreeSpace;
        ub[0].y2= yc+yw+areaFreeSpace;
      }
      nb=1;
      if(vertical) {
	pos= (idd-1)/90;
      } else {
	if(idd < 91 ){
	  ub[0].x1+=areaFreeXsize/2;
	  ub[0].x2+=areaFreeXsize*1.5;
	}
      }
    }
  }

  if (areaFreeXsize>0.) {
    xc+=areaFreeXmove[pos];
    yc+=areaFreeYmove[pos];
    ub[nb].x1= xc;
    ub[nb].x2= xc + areaFreeXsize;
    ub[nb].y1= yc;
    ub[nb].y2= yc + areaFreeYsize;
    nb++;
  }

  bool result= true;
  int ib=0;
  int n= usedBox.size();

  while (result && ib<nb) {
    int i= 0;
    while (i<n && (ub[ib].x1>usedBox[i].x2 ||
		   ub[ib].x2<usedBox[i].x1 ||
		   ub[ib].y1>usedBox[i].y2 ||
		   ub[ib].y2<usedBox[i].y1)) i++;
    result= (i==n);
    ib++;
  }

  if (result) {
    for (ib=0; ib<nb; ib++)
      usedBox.push_back(ub[ib]);
  }

  return result;
}


void ObsPlot::clearPos(){
#ifdef DEBUGPRINT
  cerr << "++ ObsPlot::clear() ++" << endl;
  cerr << "clearPos " << xUsed.size()<<endl;
#endif

  //Reset before new plot
  xUsed.clear();
  yUsed.clear();
  usedBox.clear();
}


void ObsPlot::plotList(int index)
{

  GLfloat radius=3.0;
  int printPos=-1;
  float xpos=0;
  float ypos=0;
  float xshift=0;
  float width,height;
  fp->getStringSize("0",width,height);
  height*=fontsizeScale;
  width*=fontsizeScale;
  float yStep=height/scale;  //depend on character height
  miString align;
  int num=numPar;
  bool wind=pFlag.count("wind");

  ObsData &dta = obsp[index];

  map<miString,float>::iterator f_p;
  map<miString,float>::iterator q_p;
  map<miString,float>::iterator ff_p=dta.fdata.find("ff");
  map<miString,float>::iterator dd_p=dta.fdata.find("dd");


  //reset colour
  glColor4ubv(origcolour.RGBA());
  colour = origcolour;

  if(tccriteria)checkTotalColourCriteria(index);

  miString thisImage = image;
  if(mcriteria) thisImage = checkMarkerCriteria(index);

  ImageGallery ig;
  float xShift=ig.widthp(image)/2;
  float yShift=ig.heightp(image)/2;

  if(!pFlag.count("wind")){
    ig.plotImage(thisImage,x[index],y[index],true,Scale);
  }

  glPushMatrix();
  glTranslatef(x[index],y[index],0.0);


  if( pFlag.count("pos") )
    num++;
  if( wind )
    num--;
  if( devfield )
    num++;

  //wind
  if( wind && dd_p != dta.fdata.end() && ff_p != dta.fdata.end() ){
    glPushMatrix();
    glScalef(scale,scale,0.0);
    bool ddvar=false;
    int dd = (int)dd_p->second;
    if(dd==990 || dd==510){
      ddvar=true;
      dd=270;
    }
    printPos = (dd-1)/90;

    if(ff_p->second==0){
      dd=0;
      printPos = 1;
    if(vertical) ypos += yShift;
      xpos += xShift;
    }

    if(ccriteria) checkColourCriteria("dd",(float)dd_p->second);
    if(ccriteria) checkColourCriteria("ff",ff_p->second);
    plotWind(dd,ff_p->second,ddvar,radius,current);


    if(!vertical && dd>20 && dd<92){
      if(dd<70){
	xpos += 48*sin(dd*PI/180)/2;
      } else if(dd<85){
	xpos += 55*sin(dd*PI/180);
      } else {
	xpos += 48*sin(dd*PI/180);
      }
    }

    glPopMatrix();

  } else if( num>0 ){
    if(vertical) ypos += yShift;
    xpos += xShift;
    if(wind) printUndef(xpos,ypos,"left"); //undef wind
  }

  if(!vertical){
    yStep=0;
  } else {

    switch (printPos)
      {
      case 0:
	ypos  += num*yStep + 0.2*yStep;
	align = "right";
	xshift = 2*width;
	break;
      case 1:
	ypos  += num*yStep + 0.2*yStep;
	align = "left";
	xshift = 0;
	break;
      case 2:
	ypos  += -0.2*yStep;
	align = "left";
	xshift = 0;
	break;
      case 3:
	ypos  += -0.2*yStep;
	align = "right";
	xshift = 2*width;
	break;
      default:
	ypos  += num*yStep;
	align = "left";
	xshift = 0;
    }
  }

  if( pFlag.count("pos")){
    ostringstream slat, slon;
    slat << setw(6) << setprecision(2)
	 << setiosflags(ios::fixed) << fabsf(dta.ypos);
    if (dta.ypos>=0.) slat << 'N';
    else               slat << 'S';
    slon << setw(6) << setprecision(2)
	 << setiosflags(ios::fixed) << fabsf(dta.xpos);
    if (dta.xpos>=0.) slon << 'E';
    else               slon << 'W';
    ypos -= yStep;
    miString strlat= slat.str();
    miString strlon= slon.str();
    if(ccriteria) checkColourCriteria("Pos",0);
    printString(strlat.c_str(),xpos,ypos,align);
    ypos -= yStep;
    if(!vertical){
      const char * c=strlat.c_str();
      float w,h;
      fp->getStringSize(c, w, h);
      w*=fontsizeScale;
      xpos+=w/scale+5;
    }
    if(ccriteria) checkColourCriteria("Pos",0);
    printString(strlon.c_str(),xpos,ypos,align);
    if(!vertical){
      const char * c=strlon.c_str();
      float w,h;
      fp->getStringSize(c, w, h);
      w*=fontsizeScale;
      xpos+=w/scale+5;
    }
  }
  if( pFlag.count("dd")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("dd_orig")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("dd",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("ff")){
    ypos -= yStep;
    if(dta.fdata.count("ff")){
      if(ccriteria) checkColourCriteria("ff",ff_p->second);
      printList(ms2knots(ff_p->second),xpos,ypos,0,align);
    }else{
    printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("ttt")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("TTT")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("TTT",f_p->second);
      if(tempPrecision)
	printList(float2int(f_p->second),xpos,ypos,0,align);
      else
	printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("tdtdtd")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("TdTdTd")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("TdTdTd",f_p->second);
      if(tempPrecision)
	printList(float2int(f_p->second),xpos,ypos,0,align);
      else
	printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("pppp")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("PPPP")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("PPPP",f_p->second);
      printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( devfield && (f_p=dta.fdata.find("PPPP_mslp")) != dta.fdata.end()){
    ypos -= yStep;
    printAvvik(f_p->second,xpos,ypos,align);
    if(!vertical) xpos+=2*width/scale;
  }
  if( pFlag.count("ppp")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("ppp")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("ppp",f_p->second);
      printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("a") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("a")) != dta.fdata.end()
       && f_p->second >= 0 && f_p->second < 9 ) {
      if(ccriteria) checkColourCriteria("a",f_p->second);
      symbol(itab[201+(int)f_p->second],
	     xpos*scale-xshift,ypos*scale,0.8*scale,align);
      if(!vertical) xpos+=20;
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("h")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("h")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("h",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("vv")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("VV")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("VV",f_p->second);
      printList(visibility(f_p->second,dta.zone == 99),xpos,ypos,0,align,"fill_2");

    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("n")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("N")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("N",f_p->second);
      printList(float2int(f_p->second),xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("rrr")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("RRR")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("RRR",f_p->second);
      if( f_p->second <0.1 ){ //No precipitation (0.)
	printListString("0.",xpos,ypos,align);
      }else if( f_p->second > 989 ){ //Precipitation, but less than 0.1 mm (0.0)
	printListString("0.0",xpos,ypos,align);
      } else {
	printList(f_p->second,xpos,ypos,1,align);
      }
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("ww")){
    ypos -= yStep;
    //1-3 skal ikke plottes, tror jeg
    if((f_p=dta.fdata.find("ww") ) != dta.fdata.end()&&  f_p->second >3
       && (q_p=dta.fdata.find("TTT")) != dta.fdata.end()) {
      if(ccriteria) checkColourCriteria("ww",f_p->second);
      weather((int16)f_p->second,q_p->second,dta.zone,xpos*scale-xshift,
	      (ypos-0.2*yStep)*scale,scale*0.6,align);
      if(!vertical) xpos+=20;
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if(  pFlag.count("w1") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("W1") ) != dta.fdata.end()
       && f_p->second > 2 && f_p->second < 10 ){
      if(ccriteria) checkColourCriteria("W1",f_p->second);
      pastWeather((int)f_p->second,
	     xpos*scale-xshift,ypos*scale,0.6*scale,align);
      if(!vertical) xpos+=20;
    }else
      printUndef(xpos,ypos,align);
  }
  if( pFlag.count("w2") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("W2")) != dta.fdata.end() && f_p->second > 2
       && f_p->second < 10 ){
      if(ccriteria) checkColourCriteria("W2",f_p->second);
      pastWeather((int)f_p->second,
	     xpos*scale-xshift,ypos*scale,0.6*scale,align);
      if(!vertical) xpos+=20;
    }else
      printUndef(xpos,ypos,align);
  }
  if( pFlag.count("nh")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Nh")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Nh",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("ch") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Ch")) != dta.fdata.end()
       && f_p->second > 0 && f_p->second < 10 ){
      if(ccriteria) checkColourCriteria("Cl",f_p->second);
      symbol(itab[190+(int)f_p->second],
	     xpos*scale-xshift,ypos*scale,0.6*scale,align);
      if(!vertical) xpos+=20;
    }else
      printUndef(xpos,ypos,align);
  }
  if( pFlag.count("cm") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Cm")) != dta.fdata.end()
       && f_p->second > 0 && f_p->second < 10 ){
      if(ccriteria) checkColourCriteria("Cm",f_p->second);
      symbol(itab[180+(int)f_p->second],
	     xpos*scale-xshift,ypos*scale,0.6*scale,align);
      if(!vertical) xpos+=20;
    }else
      printUndef(xpos,ypos,align);
  }
  if( pFlag.count("cl") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Cl")) != dta.fdata.end()
       && f_p->second > 0 && f_p->second < 10 ){
      if(ccriteria) checkColourCriteria("Ch",f_p->second);
      symbol(itab[170+(int)f_p->second],
	     xpos*scale-xshift,ypos*scale,0.6*scale,align);
      if(!vertical) xpos+=20;
    }else
      printUndef(xpos,ypos,align);
  }
  if( pFlag.count("vs")){
    ypos -= yStep;
    if( dta.fdata.find("ds") != dta.fdata.end()
	&& (f_p=dta.fdata.find("vs")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("vs",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("ds") ){
    ypos -= yStep;
    if( dta.fdata.find("vs") != dta.fdata.end()
      && (f_p=dta.fdata.find("ds")) != dta.fdata.end() ){
      if(ccriteria) checkColourCriteria("ds",f_p->second);
      arrow(f_p->second, xpos*scale-xshift,ypos*scale,scale*0.6);
      if(!vertical) xpos+=20;
    }else
      printUndef(xpos,ypos,align);
  }
  if( pFlag.count("twtwtw")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("TwTwTw")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("TwTwTw",f_p->second);
      if(tempPrecision)
	printList(float2int(f_p->second),xpos,ypos,0,align);
      else
	printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("pwahwa") ){
    ypos -= yStep;
    map<miString,float>::iterator p;
    if((f_p=dta.fdata.find("PwaPwa"))!= dta.fdata.end()
       && (p=dta.fdata.find("HwaHwa"))!= dta.fdata.end()){
      if(ccriteria) checkColourCriteria("PwaHwa",f_p->second);
      wave(f_p->second,p->second,xpos,ypos,align);
      if(!vertical) xpos+=5*width/scale;
    }else{
       printUndef(xpos,ypos,align);
    }
  }
  if(  pFlag.count("dw1dw1") ){
    ypos -= yStep;
    if((f_p=dta.fdata.find("dw1dw1")) != dta.fdata.end() ){
      if(ccriteria) checkColourCriteria("dw1dw1",f_p->second);
	zigzagArrow(f_p->second,
	       xpos*scale-xshift,ypos*scale,scale*0.6);
      if(!vertical) xpos+=20;
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("pw1hw1") ){
    ypos -= yStep;
    map<miString,float>::iterator p;
    if((f_p=dta.fdata.find("Pw1Pw1")) != dta.fdata.end()
       && (p=dta.fdata.find("Hw1Hw1"))!= dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Pw1Hw1",f_p->second);
      wave(f_p->second,p->second,xpos,ypos,align);
      if(!vertical) xpos+=5*width/scale;
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("txtn")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("TxTn")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("TxTn",f_p->second);
      if(tempPrecision)
	printList(float2int(f_p->second),xpos,ypos,0,align);
      else
       printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("sss")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("sss")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("sss",f_p->second);
      printList(f_p->second,xpos,ypos,1,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("911ff")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("911ff")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("911ff",f_p->second);
      printList(ms2knots(f_p->second),xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("s")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("s")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("s",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("fxfx")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("fxfx")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("fxfx",f_p->second);
      printList(ms2knots(f_p->second),xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("t_red")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("T_red")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("T_red",f_p->second);
      if(tempPrecision)
	printList(float2int(f_p->second),xpos,ypos,0,align);
      else
	printList(f_p->second,xpos,ypos,01,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("id")){
    ypos -= yStep;
    if(ccriteria) checkColourCriteria("Id",0);
    printListString(dta.id.cStr(),xpos,ypos,align);
  }
  if( pFlag.count("zone")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Zone")) != dta.fdata.end()){
    if(ccriteria) checkColourCriteria("Zone",f_p->second);
    printList(f_p->second,xpos,ypos,0,align);
    }else{
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("date")){
    ypos -= yStep;
    if(ccriteria) checkColourCriteria("Dato",0);
    printTime(dta.obsTime,xpos,ypos,align,"dato");
    if(!vertical) xpos+=5*width/scale;
  }
  if( pFlag.count("time")){
    ypos -= yStep;
      if(ccriteria) checkColourCriteria("Time",0);
    printTime(dta.obsTime,xpos,ypos,align,"h.m");
    if(!vertical) xpos+=5*width/scale;
  }
  if( pFlag.count("hhh")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("HHH")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("HHH",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    } else {
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("height")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("stationHeight")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Height",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    } else {
      printUndef(xpos,ypos,align);
    }
  }
  if( pFlag.count("pwapwa")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("PwaPwa")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("PwaPwa",f_p->second);
      printList(f_p->second,xpos,ypos,1,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("hwahwa")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("HwaHwa")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("HwaHwa",f_p->second);
      printList(f_p->second,xpos,ypos,1,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("dyp(1)")){
    ypos -= yStep;
    if( (f_p=dta.fdata.find("Dyp(1)")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Dyp(1)",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    }else{
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("tttt") ){
    ypos -= yStep;
    if( (f_p=dta.fdata.find("TTTT")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("TTTT",f_p->second);
      printList(f_p->second,xpos,ypos,2,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("ssss")){
    ypos -= yStep;
    if( (f_p=dta.fdata.find("SSSS")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("SSSS",f_p->second);
      printList(f_p->second,xpos,ypos,2,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("dyp(2)")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Dyp(2)")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Dyp(2)",f_p->second);
      printList(f_p->second,xpos,ypos,0,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("retn") ){
    ypos -= yStep;
    if( (f_p=dta.fdata.find("Retn")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Retn",f_p->second);
      printList(f_p->second,xpos,ypos,2,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("strøm")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("Strøm")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("Strøm",f_p->second);
      printList(f_p->second,xpos,ypos,2,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }
  if( pFlag.count("te")){
    ypos -= yStep;
    if((f_p=dta.fdata.find("TE")) != dta.fdata.end()){
      if(ccriteria) checkColourCriteria("TE",f_p->second);
      printList(f_p->second,xpos,ypos,2,align);
    } else {
      printList(undef,xpos,ypos,2,align);
    }
  }


  glPopMatrix();

}


void ObsPlot::printUndef(float& xpos, float& ypos, miString align)
{

  glColor4ubv(colour.RGBA());

  float x = xpos*scale;
  float y = ypos*scale;

  const char * c="X";

  float w,h;
  fp->getStringSize(c, w, h);
  w*=fontsizeScale;

  if(!vertical)
    xpos+=w/scale+5;

  if( align == "right" ) {
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);


}

void ObsPlot::printList(float f, float& xpos, float& ypos,
		          int precision, miString align, miString opt)
{
  float x = xpos*scale;
  float y = ypos*scale;

  ostringstream cs;
  cs.setf(ios::fixed);
  cs.precision(precision);
  vector<miString> vstr = opt.split(",");
  int n = vstr.size();
  for(int i=0; i<n; i++){
    if( vstr[i] == "showplus" )
      cs.setf(ios::showpos);
    else if(vstr[i] == "showpoint"){
       cs.setf(ios::showpoint);
    }
    else if( vstr[i] == "fill_2" ) {
      cs.width(2);
      cs.fill('0');
    }
  }

  if( f != undef )
    cs << f ;
  else
    cs << "X";

  miString str=cs.str();
  const char * c=str.c_str();

    float w,h;
    fp->getStringSize(c, w, h);
    w*=fontsizeScale;

    if(!vertical)
      xpos+=w/scale+5;

  if( align == "right" ) {
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);

}

void ObsPlot::printListString(const char *c,float& xpos,float& ypos,
				miString align){

  float x = xpos*scale;
  float y = ypos*scale;

  float w,h;
  fp->getStringSize(c, w, h);
  w*=fontsizeScale;

  if(!vertical)
    xpos += w/scale + 5;

  if( align == "right" ) {
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);

}

void ObsPlot::plotAscii(int index)
{

  GLfloat radius=3.0;
  int printPos=-1;
  float xpos=0;
  float ypos=0;
  float w,h;
  fp->getStringSize("0",w,h);
  h*=fontsizeScale;
  float yStep=h/scale;  //depend on character height
  miString align;
  int num=numPar;
  bool wind=pFlag.count("wind");

  //reset colour
    glColor4ubv(origcolour.RGBA());
    colour = origcolour;

  if(tccriteria)checkTotalColourCriteria(index);

  miString thisImage = image;
  if(mcriteria) thisImage = checkMarkerCriteria(index);

  ImageGallery ig;
  float xShift=ig.widthp(image)/2;
  float yShift=ig.heightp(image)/2;

  if(!asciiWind){
    if(asciiColumn.count("image")){
      miString thisImage = asciip[index][asciiColumn["image"]];
      float xShift=ig.widthp(thisImage)/2;
      float yShift=ig.heightp(thisImage)/2;
      ig.plotImage(thisImage,x[index],y[index],true,Scale);
    } else {
      ig.plotImage(thisImage,x[index],y[index],true,Scale);
    }
  }

  glPushMatrix();
  glTranslatef(x[index],y[index],0.0);

  if (asciiWind)
    num--;


  //wind
//##############################################################
//  if (asciiWind)
//    cerr<<"ASCIIWIND dd,ff: "<<asciidd[index]<<" "
//			     <<asciiff[index]<<endl;
//##############################################################
    if(asciip[index].size()>0 && asciip[index][0]==selectedStation ){
      Colour c("red");
      glColor4ubv(c.RGBA());
    }
  if( asciiWind && asciidd[index] != undef ){
    int dd =asciidd[index];
    glPushMatrix();
    glScalef(scale,scale,0.0);
      plotWind(dd,asciiff[index],false,radius,current);
    printPos = (dd-1)/90;
    if(asciiff[index]==0){
      printPos = 1;
      xpos += xShift;
    }


    if(!vertical && dd>20 && dd<92){
      if(dd<70){
	xpos += 48*sin(dd*PI/180)/2;
      } else if(dd<85){
	xpos += 55*sin(dd*PI/180);
      } else {
	xpos += 48*sin(dd*PI/180);
      }
    }

    glPopMatrix();
  } else if( num>0 ){
    if(vertical) ypos += yShift;
    xpos += xShift;
    if(wind) printUndef(xpos,ypos,"left"); //undef wind
  }

  if(!vertical){
    yStep=0;
  } else {
    switch (printPos)
      {
      case 0:
	ypos  += num*yStep + 0.2*yStep;
	align = "right";
	break;
      case 1:
	ypos  += num*yStep + 0.2*yStep;
	align = "left";
	break;
      case 2:
	ypos  += -0.2*yStep;
	align = "left";
	break;
      case 3:
	ypos  += -0.2*yStep;
	align = "right";
	break;
      default:
	ypos  += num*yStep;
	align = "left";
      }
  }

  int n= asciipar.size();
  for (int i=0; i<n; i++) {
    int j= asciipar[i];
    ypos -= yStep;
    miString str = asciip[index][j];
    float value = atof(str.cStr());
    if(parameterName)
      str = asciiColumnName[j] + " " + str;
    if(ccriteria) checkColourCriteria(asciiColumnName[j],value);
    printListString(str.cStr(),xpos,ypos,align);
  }

  glPopMatrix();

}


void ObsPlot::plotSynop(int index)
{

  ObsData &dta = obsp[index];

  GLfloat radius=7.0;
  GLfloat x1,x2,x3,y1,y2,y3;
  int lpos;
  const map<miString,float>::iterator fend=dta.fdata.end();
  map<miString,float>::iterator f_p;
  map<miString,float>::iterator h_p;
  map<miString,float>::iterator ttt_p=dta.fdata.find("TTT");

//Some positions depend on wheather the following parameters are plotted or not
  bool ClFlag = (pFlag.count("cl") && dta.fdata.count("Cl") ||
		 (pFlag.count("st.type") && dta.dataType.exists()));
  bool TxTnFlag = (pFlag.count("txtn") && dta.fdata.find("TxTn")!=fend);
  bool timeFlag = (pFlag.count("time") && dta.zone==99);
  bool precip   = (dta.fdata.count("ix") && dta.fdata["ix"] == -1);


  //reset colour
  glColor4ubv(origcolour.RGBA());
  colour = origcolour;

  if(tccriteria)checkTotalColourCriteria(index);

  glPushMatrix();
  glTranslatef(x[index],y[index],0.0);

  glPushMatrix();
  glScalef(scale,scale,0.0);


  glCallList(circle);

  // manned / automated station - ix
  if( (dta.fdata.count("ix") && dta.fdata["ix"] > 3)
      || ( dta.fdata.count("auto") && dta.fdata["auto"] == 0)){
    y1 = y2 = -1.1*radius;
    x1 = y1*sqrtf(3.0);
    x2 = -1*x1;
    x3 = 0;
    y3 = radius*2.2;
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
    glVertex2f(x1,y1);
    glVertex2f(x2,y2);
    glVertex2f(x3,y3);
    glEnd();
  }

  //wind - dd,ff
  if( pFlag.count("wind") && dta.fdata.count("dd") && dta.fdata.count("ff")
      && dta.fdata["dd"]!=undef){
    bool ddvar=false;
    int dd = (int)dta.fdata["dd"];
    if(dd==990 || dd==510){
      ddvar=true;
      dd=270;
    }
    if(ms2knots(dta.fdata["ff"])<1.)
      dd=0;
    lpos = itab[(dd/10+3)/2]+10;
    if(ccriteria) checkColourCriteria("dd",dd);
    if(ccriteria) checkColourCriteria("ff",dta.fdata["ff"]);
    plotWind(dd,dta.fdata["ff"],ddvar,radius);
  }
  else
    lpos = itab[1] +10;

  //Total cloud cover - N
  if((f_p=dta.fdata.find("N")) != fend ) {
    if(ccriteria) checkColourCriteria("N",f_p->second);
    cloudCover(f_p->second,radius);
  } else if( !precip ) {
    glColor4ubv(colour.RGBA());
    cloudCover(undef,radius);
  }

  //Weather - WW
  float VVxpos = iptab[lpos+14] + 22;
  if( pFlag.count("ww") &&
      (f_p=dta.fdata.find("ww")) != fend &&
      f_p->second >3) {//1-3 skal ikke plottes
    if(ccriteria) checkColourCriteria("ww",dta.fdata["ww"]);
    weather((int16)f_p->second,ttt_p->second,dta.zone,
	     iptab[lpos+12],iptab[lpos+13]);
    VVxpos = iptab[lpos+12] -18;
  }

  //characteristics of pressure tendency - a
  map<miString,float>::iterator ppp_p=dta.fdata.find("ppp");;
  if( pFlag.count("a") && (f_p=dta.fdata.find("a")) != fend
      && f_p->second >= 0 && f_p->second < 9 ) {
    if(ccriteria) checkColourCriteria("a",f_p->second);
    if(ppp_p!=fend && ppp_p->second > 9 )
      symbol(itab[201+(int)f_p->second], iptab[lpos+42]+10, iptab[lpos+43],0.8);
    else
      symbol(itab[201+(int)f_p->second], iptab[lpos+42], iptab[lpos+43],0.8);
  }

  // High cloud type - Ch
  if( pFlag.count("ch") && (f_p=dta.fdata.find("Ch")) != fend ){
    if(ccriteria) checkColourCriteria("Ch",f_p->second);
    symbol(itab[190+(int)f_p->second], iptab[lpos+4], iptab[lpos+5],0.8);
  }

  // Middle cloud type - Cm
  if( pFlag.count("cm") && (f_p=dta.fdata.find("Cm")) != fend ){
    if(ccriteria) checkColourCriteria("Cm",f_p->second);
    symbol(itab[180+(int)f_p->second], iptab[lpos+2], iptab[lpos+3],0.8);
  }

  // Low cloud type - Cl
  if( pFlag.count("cl") && (f_p=dta.fdata.find("Cl")) != fend   ){
    if(ccriteria) checkColourCriteria("Cl",f_p->second);
    symbol(itab[170+(int)f_p->second], iptab[lpos+22], iptab[lpos+23],0.8);
  }

  // Past weather - W1
  if(  pFlag.count("w1") && (f_p=dta.fdata.find("W1")) != fend){
    if(ccriteria) checkColourCriteria("W1",f_p->second);
    pastWeather(int(f_p->second), iptab[lpos+34], iptab[lpos+35],0.8);
  }

  // Past weather - W2
  if( pFlag.count("w2") && (f_p=dta.fdata.find("W2")) != fend ){
    if(ccriteria) checkColourCriteria("W2",f_p->second);
    pastWeather((int)f_p->second, iptab[lpos+36], iptab[lpos+37],0.8);
  }

  // Direction of ship movement - ds
  if( pFlag.count("ds") && dta.fdata.find("vs") != fend
      && (f_p=dta.fdata.find("ds")) != fend ){
    if(ccriteria) checkColourCriteria("ds",f_p->second);
    arrow(f_p->second, iptab[lpos+32], iptab[lpos+33]);
  }

  // Direction of swell waves - dw1dw1
  if(  pFlag.count("dw1dw1")
       && (f_p=dta.fdata.find("dw1dw1")) != fend ){
    if(ccriteria) checkColourCriteria("dw1dw1",f_p->second);
    zigzagArrow(f_p->second, iptab[lpos+30], iptab[lpos+31]);
  }

  glPopMatrix();

  // Pressure - PPPP
  if( devfield){
    if( (f_p=dta.fdata.find("PPPP_mslp")) != fend ){
      printAvvik(f_p->second,iptab[lpos+44]+2,iptab[lpos+45]+2);
    }
  }
  else if( pFlag.count("pppp") && (f_p=dta.fdata.find("PPPP")) != fend ){
    if(ccriteria) checkColourCriteria("PPPP",f_p->second);
    printNumber(f_p->second,iptab[lpos+44]+2,iptab[lpos+45]+2,"PPPP");
  }

  // Pressure tendency over 3 hours - ppp
  if( pFlag.count("ppp") && ppp_p != fend ){
    if(ccriteria) checkColourCriteria("ppp",ppp_p->second);
    printNumber(ppp_p->second,iptab[lpos+40]+2,iptab[lpos+41]+2,"ppp");
  }
  // Clouds
  if(pFlag.count("nh") ||pFlag.count("h")){
    f_p=dta.fdata.find("Nh");
    h_p=dta.fdata.find("h");
    if(f_p!=fend || h_p!=fend){
      float Nh,h;
      if(f_p==fend) Nh=undef;
      else Nh=f_p->second;
      if(h_p==fend) h=undef;
      else h=h_p->second;
      if(ccriteria && Nh!=undef) checkColourCriteria("Nh",Nh);
      if(ccriteria && h!=undef) checkColourCriteria("h",h);
      if( ClFlag ){
	amountOfClouds((int16)Nh, (int16)h,iptab[lpos+24]+2,iptab[lpos+25]+2);
      } else{
	amountOfClouds((int16)Nh, (int16)h,iptab[lpos+24]+2,iptab[lpos+25]+2+10);
      }
    }
  }

  //Precipitation - RRR
  if(pFlag.count("rrr") && (f_p=dta.fdata.find("RRR")) != fend &&
     !(dta.zone == 99 && dta.fdata.count("ds")&&dta.fdata.count("vs"))){
    if(ccriteria) checkColourCriteria("RRR",f_p->second);
    if( f_p->second < 0.1)   //No precipitation (0.)
      printString("0.",iptab[lpos+32]+2,iptab[lpos+33]+2);
    else if( f_p->second > 989)  //Precipitation, but less than 0.1 mm (0.0)
      printString("0.0",iptab[lpos+32]+2,iptab[lpos+33]+2);
    else
      printNumber(f_p->second,iptab[lpos+32]+2,iptab[lpos+33]+2,"RRR");
  }
  // Horizontal visibility - VV
  if( pFlag.count("vv") && (f_p=dta.fdata.find("VV")) != fend ){
    if(ccriteria) checkColourCriteria("VV",f_p->second);
    printNumber(visibility(f_p->second,dta.zone == 99),VVxpos,iptab[lpos+15],"fill_2");
  }
  // Temperature - TTT
  if( pFlag.count("ttt") && ttt_p != fend ){
    if(ccriteria) checkColourCriteria("TTT",ttt_p->second);
    printNumber(ttt_p->second,iptab[lpos+10]+2,iptab[lpos+11]+2,"temp");
  }

  // Dewpoint temperature - TdTdTd
  if( pFlag.count("tdtdtd") && (f_p=dta.fdata.find("TdTdTd")) != fend ){
    if(ccriteria) checkColourCriteria("TdTdTd",f_p->second);
    printNumber(f_p->second,iptab[lpos+16]+2,iptab[lpos+17]+2,"temp");
  }

  // Max/min temperature - TxTxTx/TnTnTn
  if( TxTnFlag ){
    if((f_p=dta.fdata.find("TxTn")) != fend ){
      if(ccriteria) checkColourCriteria("TxTn",f_p->second);
      printNumber(f_p->second,iptab[lpos+8]+2,iptab[lpos+9]+2,"temp");
    }
  }

  // Snow depth - sss
  if( pFlag.count("sss") &&
      (f_p=dta.fdata.find("sss")) != fend && dta.zone!=99 ){
    if(ccriteria) checkColourCriteria("sss",f_p->second);
    printNumber(f_p->second,iptab[lpos+46]+2,iptab[lpos+47]+2);
  }

  // Maximum wind speed (gusts) - 911ff
  if( pFlag.count("911ff") &&
      (f_p=dta.fdata.find("911ff")) != fend ){
    if(ccriteria) checkColourCriteria("911ff",f_p->second);
    printNumber(ms2knots(f_p->second),
		iptab[lpos+38]+2,iptab[lpos+39]+2,"fill_2",true);
  }

  // State of the sea - s
  if( pFlag.count("s") && (f_p=dta.fdata.find("s")) != fend ){
    if(ccriteria) checkColourCriteria("s",f_p->second);
    if(TxTnFlag)
      printNumber(f_p->second,iptab[lpos+6]+2,iptab[lpos+7]+2);
    else
      printNumber(f_p->second,iptab[lpos+6]+2,iptab[lpos+7]-14);
  }

  // Maximum wind speed
  if( pFlag.count("fxfx") && (f_p=dta.fdata.find("fxfx")) != fend
      && !(dta.zone >1 && dta.zone < 99)){
    if(ccriteria) checkColourCriteria("fxfx",f_p->second);
    if(TxTnFlag)
      printNumber(ms2knots(f_p->second),
		  iptab[lpos+6]+12,iptab[lpos+7]+2,"fill_2",true);
    else
      printNumber(ms2knots(f_p->second),
		  iptab[lpos+6]+12,iptab[lpos+7]-14,"fill_2",true);
  }

  //Maritime

  // Ship's average speed - vs
  if( pFlag.count("vs") && dta.fdata.find("ds") != fend
      && (f_p=dta.fdata.find("vs")) != fend ){
    if(ccriteria) checkColourCriteria("vs",f_p->second);
    printNumber(f_p->second,iptab[lpos+32]+18,iptab[lpos+33]+2);
  }

  //Time
  if(timeFlag && !dta.obsTime.undef()){
    if(ccriteria) checkColourCriteria("Time",0);
    printTime(dta.obsTime, float(iptab[lpos+46]+2),
	      float(iptab[lpos+47]+2),"left","h.m");
  }

  // Ship or buoy identifier
  if( pFlag.count("id") && dta.zone == 99){
    if(ccriteria) checkColourCriteria("Id",0);
    miString kjTegn = dta.id;
    if( timeFlag )
      printString(kjTegn.cStr(),iptab[lpos+46]+2,iptab[lpos+47]+15);
    else
      printString(kjTegn.cStr(),iptab[lpos+46]+2,iptab[lpos+47]+2);
  }

  //Wmo block + station number - land stations
  if( (pFlag.count("st.no(5)") || pFlag.count("st.no(3)")) && dta.zone != 99){
    if(ccriteria) checkColourCriteria("St.no(5)",0);
      miString kjTegn = dta.id;
      if(!pFlag.count("st.no(5)") && kjTegn.size()>4){
	kjTegn = kjTegn.substr(2,3);
	if(ccriteria) checkColourCriteria("St.no(3)",0);
      }
      if( (pFlag.count("sss") && dta.fdata.count("sss"))) //if snow
	printString(kjTegn.cStr(),iptab[lpos+46]+2,iptab[lpos+47]+15);
      else
	printString(kjTegn.cStr(),iptab[lpos+46]+2,iptab[lpos+47]+2);
  }

  //Sea temperature
  if( pFlag.count("twtwtw") &&
      (f_p=dta.fdata.find("TwTwTw")) != fend ){
    if(ccriteria) checkColourCriteria("TwTwTw",f_p->second);
    printNumber(f_p->second,
		iptab[lpos+18]+2,iptab[lpos+19]+2,"temp",true);
  }

  //Wave
  if( pFlag.count("pwahwa") &&
      (f_p=dta.fdata.find("PwaPwa")) != fend &&
      (h_p=dta.fdata.find("HwaHwa")) != fend  ){
    if(ccriteria) checkColourCriteria("PwaHwa",0);
    wave(f_p->second,h_p->second,
	 iptab[lpos+20]+2,iptab[lpos+21]+2);
  }
  if( pFlag.count("pw1hw1")
    && ((f_p=dta.fdata.find("Pw1Pw1")) != fend &&
	(h_p=dta.fdata.find("Hw1Hw1")) != fend ) ){
    if(ccriteria) checkColourCriteria("Pw1Hw1",0);
    wave(f_p->second,h_p->second,iptab[lpos+28]+2,iptab[lpos+29]+2);
  }

  if(!flaginfo){
    glPopMatrix();
    return;
  }

  //----------------- HQC only ----------------------------------------
  //Flag colour

  if( pFlag.count("id") ){
    glColor4ubv(colour.RGBA());
    int ypos = iptab[lpos+47]+2;
    if( timeFlag ) ypos += 13;
    if( (pFlag.count("sss") && dta.fdata.count("sss"))) ypos += 13;
    printString(dta.id.cStr(),iptab[lpos+46]+2,ypos);
  }

    //Flag + red/yellow/green
  if(pFlag.count("flag")
     && hqcFlag.exists() && dta.flagColour.count(hqcFlag)) {
    glPushMatrix();
    glScalef(scale,scale,0.0);
    glColor4ubv(dta.flagColour[hqcFlag].RGBA());
    glCallList(circle);
    cloudCover(8,radius);
    //    glColor4ubv(colour.RGBA());
    glColor4ubv(colour.RGBA());
    glPopMatrix();
  }


  //Type of station (replace Cl)
  bool typeFlag = (pFlag.count("st.type") && dta.dataType.exists());
  if( typeFlag )
    printString(dta.dataType.c_str(), iptab[lpos+22], iptab[lpos+23]);

  // Man. precip, marked by dot
  if( precip ){
    glPushMatrix();
      glScalef(scale*0.3,scale*0.3,0.0);
      glCallList(circle);
    cloudCover(8,radius);
    glPopMatrix();
  }

  //id
  if(pFlag.count("flag") && hqcFlag.exists()&& dta.flag.count(hqcFlag)){
    glColor4ubv(paramColour["flag"].RGBA());
    int ypos=iptab[lpos+47];
    if(pFlag.count("id") || typeFlag) ypos +=15;
    if(timeFlag) ypos +=15;
    if(pFlag.count("sss") && dta.fdata.count("sss")) ypos +=15; //if snow
    printString(dta.flag[hqcFlag].cStr(),iptab[lpos+46]+2,ypos);
  }

  //red circle
  if( pFlag.count("id") && dta.id==selectedStation ){
    glPushMatrix();
      glScalef(scale*1.3,scale*1.3,0.0);
      GLfloat lwidth;
      glGetFloatv(GL_LINE_WIDTH,&lwidth);
      glLineWidth(2*lwidth);
      Colour c("red");
      glColor4ubv(c.RGBA());
      glCallList(circle);
      glPopMatrix();
      glLineWidth(lwidth);
  }
  //----------------- end HQC only ----------------------------------------

  glPopMatrix();

}



void ObsPlot::plotMetar(int index)
{

  ObsData &dta = obsp[index];

  GLfloat radius=7.0;
  int lpos = itab[1]+10;
  const map<miString,float>::iterator fend=dta.fdata.end();
  map<miString,float>::iterator f2_p;
  map<miString,float>::iterator f_p;


  //reset colour
  glColor4ubv(origcolour.RGBA());
  colour = origcolour;

  if(tccriteria)checkTotalColourCriteria(index);

  glPushMatrix();
  glTranslatef(x[index],y[index],0.0);

  //Circle
  glPushMatrix();
  glScalef(scale,scale,0.0);
  glCallList(circle);
  glPopMatrix();

  //wind
  if(pFlag.count("wind") && dta.fdata.count("dd") && dta.fdata.count("ff")){
    if(ccriteria) checkColourCriteria("dd",dta.fdata["dd"]);
    if(ccriteria) checkColourCriteria("ff",dta.fdata["ff"]);
    metarWind((int)dta.fdata["dd"],ms2knots(dta.fdata["ff"]),radius,lpos);
  }

  //limit of variable wind direction
  int dndx=16;
  if(pFlag.count("dndx") && (f_p=dta.fdata.find("dndndn")) != fend
     && (f2_p=dta.fdata.find("dxdxdx")) != fend ){
      ostringstream cs;
      cs << f_p->second/10<<"V"<<f2_p->second/10;
      printString(cs.str().c_str(),iptab[lpos+2]+2,iptab[lpos+3]+2);
      dndx = 2;
    }
  //Wind gust
  float xid,yid;
  if(pFlag.count("fmfm") &&  (f_p=dta.fdata.find("fmfm")) != fend ){
    if(ccriteria) checkColourCriteria("fmfm",f_p->second);
    printNumber(ms2knots(f_p->second),
		iptab[lpos+4]+2,iptab[lpos+5]+2-dndx,"left",true);
      //understrekes
      xid = iptab[lpos+4]+20+15;
      yid = iptab[lpos+5]-dndx+8;
    } else {
      xid = iptab[lpos+4]+2+15;
      yid = iptab[lpos+5]+2-dndx+8;
    }

  //Temperature
  if(pFlag.count("ttt") && (f_p=dta.fdata.find("TTT")) != fend ){
    if(ccriteria) checkColourCriteria("TTT",f_p->second);
    //    if( dta.TT>-99.5 && dta.TT<99.5 ) //right aligned
      printNumber(f_p->second,iptab[lpos+12]+23,iptab[lpos+13]+16,"right");
  }

  //Dewpoint temperature
  if( pFlag.count("tdtdtd") && (f_p=dta.fdata.find("TdTdTd")) != fend ){
    if(ccriteria) checkColourCriteria("TdTdTd",f_p->second);
  //    if( dta.TdTd>-99.5 && dta.TdTd<99.5 )  //right aligned and underlined
      printNumber(f_p->second,iptab[lpos+14]+23,iptab[lpos+15]-16,"right",true);
}
  glPushMatrix();
  glScalef(scale,scale,0.0);
  glScalef(0.8,0.8,0.0);

  //Significant weather
  int wwshift=0; //idxm
  if(pFlag.count("ww")){
    if(ccriteria) checkColourCriteria("ww",0);
    if(dta.ww.size()>0 && dta.ww[0].exists()){
      metarSymbol(dta.ww[0],iptab[lpos+8], iptab[lpos+9], wwshift);
    }
    if(dta.ww.size()>1 && dta.ww[1].exists()){
      metarSymbol(dta.ww[1],iptab[lpos+10], iptab[lpos+11], wwshift);
    }
  }

  //Recent weather
  if(pFlag.count("reww")){
    if(ccriteria) checkColourCriteria("REww",0);
    if(dta.REww.size()>0 && dta.REww[0].exists()){
      int intREww[5];
      metarString2int(dta.REww[0],intREww);
      if(intREww[0]>=0 && intREww[0]<100 ){
	symbol(itab[40+intREww[0]],iptab[lpos+30],iptab[lpos+31]+2);
      }
    }
    if(dta.REww.size()>1 && dta.REww[1].exists()){
      int intREww[5];
      metarString2int(dta.REww[1],intREww);
      if(intREww[0]>=0 && intREww[0]<100 ){
	symbol(itab[40+intREww[0]],iptab[lpos+30]+15,iptab[lpos+31]+2);
      }
    }
  }

  glPopMatrix();

  //Visibility (worst)
  if(pFlag.count("vvvv/dv")){
    if( (f_p=dta.fdata.find("VVVV")) != fend){
      if(ccriteria) checkColourCriteria("VVVV/Dv",0);
      if((f2_p=dta.fdata.find("Dv")) != fend){
	printNumber(float(int(f_p->second)/100),iptab[lpos+12]+2+wwshift,iptab[lpos+13]+2);
	printNumber(vis_direction(f2_p->second),
			iptab[lpos+12]+22+wwshift,iptab[lpos+13]+2);
      } else {
	printNumber(float(int(f_p->second)/100),
		    iptab[lpos+12]+22+wwshift,iptab[lpos+13]+2);
      }
    }
  }

  //Visibility (best)
  if(pFlag.count("vxvxvxvx/dvx")){
    if( (f_p=dta.fdata.find("VxVxVxVx")) != fend){
      if(ccriteria) checkColourCriteria("VVVV/Dv",0);
      if((f2_p=dta.fdata.find("Dvx")) != fend){
	printNumber(float(int(f_p->second)/100),iptab[lpos+12]+2+wwshift,iptab[lpos+15]);
	printNumber(f2_p->second,iptab[lpos+12]+22+wwshift,iptab[lpos+13]);
      } else {
	printNumber(float(int(f_p->second)/100),iptab[lpos+14]+22+wwshift,iptab[lpos+15]);
      }
    }
  }

  //CAVOK
  if(pFlag.count("clouds")){
    if(ccriteria) checkColourCriteria("Clouds",0);

    if( dta.CAVOK ){
      printString("CAVOK",iptab[lpos+18]+2,iptab[lpos+19]+2);
    } else {  //Clouds
      int ncl = dta.cloud.size();
      for( int i=0;i<ncl;i++)
	printString(dta.cloud[i].cStr(),iptab[lpos+18+i*4]+2,iptab[lpos+19+i*4]+2);
    }
  }

  //QNH ??
  if(pFlag.count("phphphph")){
    if( (f_p=dta.fdata.find("PHPHPHPH")) != fend ){
    if(ccriteria) checkColourCriteria("PHPHPHPH",f_p->second);
    int pp = (int)f_p->second;
      pp -= (pp/100)*100;
      printNumber(pp,iptab[lpos+32]+2,iptab[lpos+33]+2,"fill_2");
    }
  }

    //Id
  if(pFlag.count("id")){
    if(ccriteria) checkColourCriteria("Id",0);
    printString(dta.metarId.cStr(),xid,yid);
  }

  glPopMatrix();
}


void ObsPlot::metarSymbol(miString ww, float xpos, float ypos, int &idxm)
{

  int intww[5];

  metarString2int(ww, intww);

  if(intww[0]<0 || intww[0]>99 ) return;

  int lww1, lww2, lww3, lww4;
  int sign;
  int idx=0;
  float dx=xpos, dy=ypos;

  lww1=intww[0];
  lww2=intww[1];
  lww3=intww[2];
  lww4=intww[3];
  sign=intww[4];

  if( lww1 == 24 ){
    dx += 10;
    symbol(itab[40+lww1], dx, dy);
    lww3=undef;
    if( lww2>500 &&lww2<9000 ){
      lww3 = lww2 - (lww2/100)*100;
      lww2 = lww2/100;
      dx -= 5;
    }
    if( lww2==45 || (lww2>49 && lww2<71) || lww2==87 || lww2==89 ) {
      dy -= 6;
      symbol(itab[40+lww2],dx,dy);
      if( lww3 > 49 ){
	dx += 10;
	symbol(itab[40+lww3],dx,dy);
      }
      dy += 6;
    }
    lww1 = undef;
  } else {
    if( lww2 > 0 ) idx -= 20;
    if( lww3 > 0 ) idx -= 20;
    if( lww4 > 0 ) idx -= 20;
    if( idx < idxm) idxm = idx;
  }
  dx = xpos + idx;
  if( sign > 0) {
    symbol(itab[40+sign],dx+2,dy);
    dy -= 2;
  }
  dx += 10;

  int lwwx[4];
  lwwx[0]=lww1;
  lwwx[1]=lww2;
  lwwx[2]=lww3;
  lwwx[3]=lww4;
  for(int i=0; i<4 && lwwx[i]>-1;i++){
    lww1 = lwwx[i];
    dy = ypos;
    if( lww1>5000 && lww1<9000){
      dy += 5;
      symbol(itab[40+lww1/100],dx,dy);
      dy   -= 10;
      lww1 -= (lww1/100)*100;
    }
    symbol(itab[40+lww1],dx,dy);
    dx += 20;
  }

}


void ObsPlot::metarString2int(miString ww, int intww[]){

  int size = ww.size();

  if ( size == 0 ) {
    for( int i=0; i<4; i++)
      intww[i] = undef;
    intww[4] = 0;
    return;
  }

  int sign=0;
  if( ww[0]=='-' || ww[0]=='+' ){
    sign = ( ww[0]=='-') ? 1:2;
    ww.erase(ww.begin());
  } else if( size > 1 && ww.substr(0,2)=="VC" ){
    sign = 3;
    ww.erase(ww.begin(),ww.begin()+2);
  }


  vector<metarww> ia;
  for( int i=0; i<size; i+=2 ){
    miString sub=ww.substr(i,2);
    if(metarMap.find(sub) != metarMap.end())
      ia.push_back(metarMap[sub]);
  }

  int iasize = ia.size();
  for( int i=1; i<iasize; i++){
    if( ia[i].lwwg < ia[i-1].lwwg ){
      metarww tmp = ia[i];
      ia[i]       = ia[i-1];
      ia[i-1]     = tmp;
      i=0;
    }
  }

  if( iasize > 1 ) {
    bool k=false;
    if((ia[0].lww==11 || ia[0].lww==41) && (ia[1].lww==45)) //MIFG, BCFG
      k=true;
    if((ia[0].lww==38 || ia[0].lww==36) && (ia[1].lww==70)) //BLSN, DRSN
      k=true;
    //BLDU, BLSA, DRDU, DRSA
    if((ia[0].lww==38 || ia[0].lww==36) && (ia[1].lww==6 || ia[1].lww==7)) {
      ia[0].lww=31;
      k=true;
    }
    if(k)
      ia.erase(ia.begin()+1);
  }

  iasize = ia.size();
  int j1=0;
  if( iasize > 1 )
    for( int i=0; i<iasize; i++)
      if( ia[i].lwwg == 2 ){
	j1++;
	if( i!=0 && ia[0].lww==80 && ia[i].lww==80 ){
	  ia[i].lww  = 70;
	  ia[i].lwwg = 3;
	  j1--;
	}
      }

  //testing group 2, only one should be left
  if( j1>1 ){
    if( ia[0].lwwg == 2 ){
      int j2=lwwg2[ia[0].lww];
      for( int i=1; i<iasize; i++)
	if( ia[i].lwwg == 2 && lwwg2[ia[i].lww] < j2){
	  ia[0].lww = ia[i].lww;
	  ia.erase(ia.begin()+i);
	}
    }
  }

  if(ia.size()>1)
    if(ia[0].lwwg==3 && ia[0].lww!=79 && ia[1].lwwg==3 && ia[1].lww!=79){
      ia[0].lww += 100*ia[1].lww;
      ia.erase(ia.begin()+1);
    }

  if(ia.size()>2)
    if(ia[1].lwwg==3 && ia[1].lww!=79 && ia[2].lwwg==3 && ia[2].lww!=79){
      ia[1].lww += 100*ia[2].lww;
      ia.erase(ia.begin()+2);
    }

  if(ia.size()>3)
    if(ia[2].lwwg==3 && ia[2].lww!=79 && ia[3].lwwg==3 && ia[3].lww!=79){
      ia[2].lww += 100*ia[3].lww;
      ia.erase(ia.begin()+3);
    }

  iasize = ia.size();
  int i;
  for( i=0; i<iasize; i++){
    intww[i] = ia[i].lww;
  }
  for( ; i<4; i++)
    intww[i] = undef;
  intww[4] = sign;

  return;
}
void ObsPlot::metarWind(int dd, int ff, float & radius, int &lpos)
{

  GLfloat x1,x2,x3,y1,y2,y3,x4,y4;

  lpos = itab[1]+10;
  //dd=999;
  bool box=false;
  bool kryss=true;

  //dd=999 - variable wind direction, ff will be written in the middel of the
  //         circle. (This never happends, because they write dd=990)
  if(dd==999 && ff>=0 && ff<100 ){
    printNumber(ff,-8,-8,"right");
    y1 = -12;
    box   = true;
    kryss = false;
  } else if( dd<0 || dd>360){
    lpos = itab[20] + 10;
    if( ff>=0 && ff<100 ) //dd undefined - ff written above the circle
      printNumber(ff,0,14,"center");
    else                  //dd and ff undefined - XX written above the circle
      printString("XX",-8,14);
    y1 = 10;
    box= true;
  }

  glPushMatrix();
  glScalef(scale,scale,0.0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // Kryss over sirkelen hvis ikke variabel vindretn (dd=999)
  if(kryss)
    cloudCover(undef,radius);   //cloud cover not observed

  if(box){
    x1=x4=-12;
    x2=x3=x1+22;
    y2=y1;
    y3=y4=y1+22;
    glBegin(GL_POLYGON);
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
      glVertex2f(x3,y3);
      glVertex2f(x4,y4);
    glEnd();
    glPopMatrix();
    return;
  }

  // vindretn. observert, men ikke vindhast. - skjer ikke for metar (kun synop)

  // vindstille
  if( dd == 0) {
    glPushMatrix();
    glScalef(1.5,1.5,0.0);
    glCallList(circle);
    cloudCover(9,radius);   //litt større kryss
    glPopMatrix();
    glPopMatrix();
    return;
//     glBegin(GL_POLYGON);
//     for(i=0;i<100;i++){
//       x1 = radius*1.5*cos(i*2*PI/100.0);
//       y1   = radius*1.5*sin(i*2*PI/100.0);
//       glVertex2f(x1,y1);
//     }
//     glEnd();
  }
//   else {

//#### if( ff > 0 && ff < 100 ){
  if( ff > 0 && ff < 200 ){
    lpos = itab[((dd/10)+3)/2] + 10;
    glPushMatrix();
    glRotatef(360-dd,0.0,0.0,1.0);

    ff = (ff+2)/5*5;
    x1 = 0;
    y1 = radius;
    x2 = 0;
    y2 = 47.0;
    glBegin(GL_LINES);
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
    glEnd();

    // vindhastighet
      x1 = 0;
      y1 = 47.0;
      x2 = 13;
      y2 = y1 + 7;
      if ( ff < 10 ){
	y1 -= 6;
	y2 -= 6;
      }
      if (ff>=50) {
        for( ; ff>=50; ff-=50 ){
          x3 = 0;
	  y3 = y1;
          y2 -= 15;
          y1 -= 15;
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          glBegin(GL_POLYGON);
          glVertex2f(x1,y1);
          glVertex2f(x2,y2);
          glVertex2f(x3,y3);
          glVertex2f(x1,y1);
          glEnd();
        }
        y1 -= 6;
        y2 -= 6;
      }
      for( ; ff>=10; ff-=10 ){
	glBegin(GL_LINES);
	glVertex2f(x1,y1);
	glVertex2f(x2,y2);
	glEnd();
	y1 -= 6;
	y2 -= 6;
      }
      if(ff >= 5 ){
	x2 = (x1+x2)/2;
      y2 = (y1+y2)/2;
      glBegin(GL_LINES);
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
      glEnd();
      }
      glPopMatrix();
    }

  glPopMatrix();

}

void ObsPlot::initMetarMap(){
  metarMap["BC"].lww  = 41;
  metarMap["BC"].lwwg = 2;
  metarMap["BL"].lww  = 38;
  metarMap["BL"].lwwg = 2;
  metarMap["BR"].lww  = 10;
  metarMap["BR"].lwwg = 4;
  metarMap["DR"].lww  = 36;
  metarMap["DR"].lwwg = 2;
  metarMap["DS"].lww  = 34;
  metarMap["DS"].lwwg = 5;
  metarMap["DU"].lww  = 6;
  metarMap["DU"].lwwg = 4;
  metarMap["DZ"].lww  = 50;
  metarMap["DZ"].lwwg = 3;
  metarMap["FC"].lww  = 19;
  metarMap["FC"].lwwg = 5;
  metarMap["FG"].lww  = 45;
  metarMap["FG"].lwwg = 4;
  metarMap["FU"].lww  = 4;
  metarMap["FU"].lwwg = 4;
  metarMap["FZ"].lww  = 24;
  metarMap["GZ"].lwwg = 2;
  metarMap["GR"].lww  = 89;
  metarMap["GR"].lwwg = 3;
  metarMap["GS"].lww  = 87;
  metarMap["GS"].lwwg = 3;
  metarMap["HZ"].lww  = 05;
  metarMap["HZ"].lwwg = 4;
  metarMap["IC"].lww  = 76;
  metarMap["IC"].lwwg = 3;
  metarMap["MI"].lww  = 11;
  metarMap["MI"].lwwg = 2;
  metarMap["PE"].lww  = 79;
  metarMap["PE"].lwwg = 3;
  metarMap["PO"].lww  = 8;
  metarMap["PO"].lwwg = 5;
  metarMap["RA"].lww  = 60;
  metarMap["RA"].lwwg = 3;
  metarMap["RE"].lww  = 60;
  metarMap["RE"].lwwg = 3;
  metarMap["SA"].lww  = 7;
  metarMap["SA"].lwwg = 4;
  metarMap["SG"].lww  = 77;
  metarMap["SG"].lwwg = 3;
  metarMap["SH"].lww  = 80;
  metarMap["SH"].lwwg = 2;
  metarMap["SN"].lww  = 70;
  metarMap["SN"].lwwg = 3;
  metarMap["SQ"].lww  = 18;
  metarMap["SQ"].lwwg = 5;
  metarMap["SS"].lww  = 34;
  metarMap["SS"].lwwg = 5;
  metarMap["TS"].lww  = 17;
  metarMap["TS"].lwwg = 2;
  metarMap["VA"].lww  = 4;
  metarMap["VA"].lwwg = 4;

  lwwg2[17] = 0;
  lwwg2[24] = 1;
  lwwg2[80] = 2;
  lwwg2[41] = 3;
  lwwg2[11] = 4;
  lwwg2[38] = 5;
  lwwg2[36] = 6;
  lwwg2[31] = 7;
}

void ObsPlot::printNumber(float f,float x,float y,miString align,bool line,
			  bool mark)
{

  x*=scale;
  y*=scale;

  ostringstream cs;

  if( align =="temp" ) {
    cs.setf(ios::fixed);
    if(tempPrecision){
      f = float2int(f);
      cs.precision(0);
    }else{
      cs.precision(1);
    }
    cs << f ;
    float w,h;
    miString str=cs.str();
    const char * c=str.c_str();
    fp->getStringSize(c, w, h);
      w*=fontsizeScale;
    x -= w-30*scale;
  }

  else   if( align == "center" ) {
    float w,h;
    cs << f ;
    miString str=cs.str();
    const char * c=str.c_str();
    fp->getStringSize(c, w, h);
      w*=fontsizeScale;
    x -= w/2;
  }

  else if( align == "PPPP" ) {
    cs.width(3);
    cs.fill('0');
    f  = (int(f*10+0.5))%1000;
    cs << f ;
  }

  else if( align == "fill_2" ) {
    int i = float2int(f);
    cs.width(2);
    cs.fill('0');
    cs << i ;
  }
  else if( align == "ppp" ) {
    if( f > 0 )  cs << '+';
    else         cs << '-';
    if( fabsf(f) < 1  ) cs << '0';
    cs << fabsf(f)*10 ;
  }
  else if( align == "RRR" ) {
    cs.setf(ios::fixed);
    cs.setf(ios::showpoint);
    if( f<1){
      cs.precision(1);
    }else{
      cs.precision(0);
    }
    cs << f ;
  }
  else
    cs << f ;

  miString str=cs.str();
  const char * c=str.c_str();

  if(mark){
    float cw,ch;
    Colour col("white");
    if(!(colour == col) )
      glColor4ubv(col.RGBA()); //white
    else
      glColor3ub(0,0,0); //black
    fp->getStringSize(c,cw,ch);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
      glVertex2f(x,y-0.2*ch);
      glVertex2f(x+cw,y-0.2*ch);
      glVertex2f(x+cw,y+0.8*ch);
      glVertex2f(x,y+0.8*ch);
    glEnd();
    glColor3ub(0,0,0); //black
    glBegin(GL_LINE_LOOP);
      glVertex2f(x,y-0.2*ch);
      glVertex2f(x+cw,y-0.2*ch);
      glVertex2f(x+cw,y+0.8*ch);
      glVertex2f(x,y+0.8*ch);
    glEnd();
    glColor4ubv(colour.RGBA());
  }

  fp->drawStr(c,x,y, 0.0);

  if( line){
    float w,h;
    fp->getStringSize(c, w, h);
      w*=fontsizeScale;
      h*=fontsizeScale;
    glBegin(GL_LINES);
      glVertex2f(x,(y-h/6));          //get size of string
      glVertex2f((x+w),(y-h/6));
    glEnd();
  }

}


void ObsPlot::printAvvik(float f,float x, float y, miString align)
{

  x *= scale;
  y *= scale;

  ostringstream cs;
  cs.setf(ios::fixed);
  cs.precision(1);
  cs.setf(ios::showpos);

  if( f != undef ){
    cs << f ;
    if(f>0){
      //      Colour c("cyan");
      glColor4ubv(mslpColour2.RGBA());
    }
    else if(f<0){
      //      Colour c("red");
      glColor4ubv(mslpColour1.RGBA());
    }
  }
  else {
    cs << "X";
  }

  miString str=cs.str();

  const char * c=str.c_str();

  if( align == "right" ) {
    float w,h;
    fp->getStringSize(c, w, h);
    w*=fontsizeScale;
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);

  glColor4ubv(colour.RGBA());

}


void ObsPlot::printString(const char *c,float x,float y,miString align,bool line){

  x*=scale;
  y*=scale;

  if( align == "right" ) {
    float w,h;
    fp->getStringSize(c, w, h);
      w*=fontsizeScale;
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);

  if( line){
    float w,h;
    fp->getStringSize(c, w, h);
      w*=fontsizeScale;
      h*=fontsizeScale;
    glBegin(GL_LINES);
    glVertex2f(x,(y-h/6));          //get size of string
    glVertex2f((x+w),(y-h/6));
    glEnd();
  }

  //  glColor4ubv(colour.RGBA());

}


void ObsPlot::printTime(miTime time,float x,float y,
			miString align, miString format){

  if(time.undef()) return;

  x *= scale;
  y *= scale;

  const char * c;
  if(format == "h.m"){
    c = time.format("%H.%M").cStr();
  } else if(format == "dato"){
    c = time.format("%m-%d").cStr();
  } else {
    c = time.isoTime().cStr();
  }

  if( align == "right" ) {
    float w,h;
    fp->getStringSize(c, w, h);
    w*=fontsizeScale;
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);

}


int ObsPlot::visibility(float VV, bool ship)
{
  //Code table 4377
  //12.2.1.3.2   In reporting visibility at sea,
  //             the decile 90 - 99 shall be used for VV

  int vv = int(VV);
  //SHIP
  if(ship){
    if( vv<50    ) return 90;
    if( vv<200   ) return 91;
    if( vv<500   ) return 92;
    if( vv<1000  ) return 93;
    if( vv<2000  ) return 94;
    if( vv<4000  ) return 95;
    if( vv<10000 ) return 96;
    if( vv<20000 ) return 97;
    if( vv<50000 ) return 98;
    return 99;
  }

  //LAND
  if(vv<5100) return (vv/=100);
  if(vv<31000) return (vv/1000 +50);
  if(vv<71000) return ((vv-30000)/5000 +80);
  return 89;

}

int ObsPlot::vis_direction(float dv)
{
  if(dv< 22) return 8;
  return int(dv)/45;
}

void ObsPlot::amountOfClouds(int16 Nh, int16 h, float x, float y)
{

  miString str;
  const char * c;

  ostringstream ost;
  if( Nh>-1 && Nh<10 )
    ost << Nh ;
  else
    ost << "x";

  str=ost.str();
  c=str.c_str();
  fp->drawStr(c,x*scale,y*scale, 0.0);

  x += 8;
  y -= 2;
  fp->drawStr("/",x*scale,y*scale, 0.0);

  ostringstream ostr;
  x += 6;  // += 8;
  y -= 2;
  if( h>-1 && h<10)
    ostr << h;
  else
    ostr << "x";

  str=ostr.str();
  c=str.c_str();
  fp->drawStr(c,x*scale,y*scale, 0.0);

}


void ObsPlot::arrow(float& angle, float xpos, float ypos,float scale)
{
  glPushMatrix();
  glTranslatef(xpos,ypos,0.0);
  glScalef(scale,scale,0.0);
  glTranslatef(8,8,0.0);
  glRotatef(360-angle,0.0,0.0,1.0);

  glBegin(GL_LINE_STRIP);
    glVertex2f(0,-6);
    glVertex2f(0,6);
  glEnd();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBegin(GL_POLYGON);
    glVertex2f(-2,2);
    glVertex2f(0,6);
    glVertex2f(2,2);
  glEnd();

  glPopMatrix();
}
void ObsPlot::zigzagArrow(float& angle, float xpos, float ypos,float scale)
{
  glPushMatrix();
  glTranslatef(xpos,ypos,0.0);
  glScalef(scale,scale,0.0);
  glTranslatef(9,9,0.0);
  glRotatef(359-angle,0.0,0.0,1.0);

  glBegin(GL_LINE_STRIP);
    glVertex2f( 0,0);
    glVertex2f( 2,1);
    glVertex2f(-2,3);
    glVertex2f( 2,5);
    glVertex2f(-2,7);
    glVertex2f( 2,9);
    glVertex2f( 0,10);
  glEnd();

  glBegin(GL_LINES);
    glVertex2f( 0, 0);
    glVertex2f( 0,-10);
    glVertex2f( 0,-10);
    glVertex2f( 4,-6);
    glVertex2f( 0,-10);
    glVertex2f(-4,-6);
  glEnd();

  glPopMatrix();
}

void ObsPlot::symbol(int n, float xpos, float ypos,float scale, miString align)
{

  int npos, nstep, k1, k2, k=0;
  int ipx,ipy;

  glPushMatrix();
  glTranslatef(xpos,ypos,0.0);
  glScalef(scale,scale,0.0);

  npos = iptab[n+3];
  nstep= iptab[n+9];
  k1   = n+10;
  k2   = k1 + nstep*npos;

  GLfloat x[100];
  GLfloat y[100];

  x[0] = iptab[n+4];
  y[0] = iptab[n+5];

  for( int i=k1; i<k2; i+=nstep){
    ipx = iptab[i];
    ipy = iptab[i+1];
    if( ipx > -100 && ipx < 100){
      k++;
      x[k] = x[k-1] + ipx;
      y[k] = y[k-1] + ipy;
    }
    else {
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<=k; j++)
	glVertex2f(x[j],y[j]);
      glEnd();
      x[0] = x[k] + (ipx - (ipx/100)*100);
      y[0] = y[k] + ipy;
      k = 0;
    }
  }


  if( k > 0 ){
    glBegin(GL_LINE_STRIP);
    for(int j=0; j<=k; j++)
      glVertex2f(x[j],y[j]);
    glEnd();
  }


  glPopMatrix();

}


void ObsPlot::cloudCover(const float& fN, const float &radius)
{

  int N = float2int(fN);

  int i;
  float x,y;

  // Total cloud cover N

  if(N < 0 || N > 9) { //cloud cover not observed
    x = radius*1.1/sqrt((float)2);
    glBegin(GL_LINES);
    glVertex2f(x,x);
    glVertex2f(-1*x,-1*x);
    glVertex2f(x,-1*x);
    glVertex2f(-1*x,x);
    glEnd();
  }
  else if( N == 9) {
    x = radius/sqrt((float)2);
    glBegin(GL_LINES);
    glVertex2f(x,x);
    glVertex2f(-1*x,-1*x);
    glVertex2f(x,-1*x);
    glVertex2f(-1*x,x);
    glEnd();
  }
  else if( N == 7){
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    for( i=5;i<46;i++){
      x = radius*cos(i*2*PI/100.0);
      y = radius*sin(i*2*PI/100.0);
      glVertex2f(y,x);
    }
    i=5;
    x = radius*cos(i*2*PI/100.0);
    y = radius*sin(i*2*PI/100.0);
    glVertex2f(y,x);
    glEnd();
    glBegin(GL_POLYGON);
    for(i=55;i<96;i++){
      x = radius*cos(i*2*PI/100.0);
      y = radius*sin(i*2*PI/100.0);
      glVertex2f(y,x);
    }
    i=55;
    x = radius*cos(i*2*PI/100.0);
    y = radius*sin(i*2*PI/100.0);
    glVertex2f(y,x);
    glEnd();
  }
  else {
    if( N == 1 || N == 3){
      glBegin(GL_LINES);
      glVertex2f(0,radius);
      glVertex2f(0,-radius);
      glEnd();
    }
    else if( N == 5){
      glBegin(GL_LINES);
      glVertex2f(8,0);
      glVertex2f(-8,0);
      glEnd();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    glVertex2f(0,0);
    for(i=0;i<101*(N/2)/4.0;i++){
      x = radius*cos(i*2*PI/100.0);
      y = radius*sin(i*2*PI/100.0);
      glVertex2f(y,x);
    }
    glVertex2f(0,0);
    glEnd();
  }

}


void ObsPlot::plotWind(int dd, float ff_ms, bool ddvar, float &radius, float current)
{

  //full feather = current
  if(current>0) ff_ms = ff_ms*10.0/current;

  float ff; //wind in knots (current in m/s)

  if(current<0)
    ff = ms2knots(ff_ms);
  else
    ff = int(ff_ms);

  GLfloat x1,x2,x3,y1,y2,y3,x4,y4;
  int i;

  // just a guess of the max possible in plotting below
  if (ff>200) ff= 200;

  glPushMatrix();

  // calm
  if( ff < 1.) {
    glBegin(GL_LINE_LOOP);
    for(i=0;i<100;i++){
      x1 = radius*1.5*cos(i*2*PI/100.0);
      y1 = radius*1.5*sin(i*2*PI/100.0);
      glVertex2f(x1,y1);
    }
    glEnd();
  } else {
    glRotatef(360-dd,0.0,0.0,1.0);

    glBegin(GL_LINES);

    ff = (ff+2)/5*5;
    x1 = 0;
    y1 = radius;
    if (plottype=="list" || plottype=="ascii" || current>0) y1=0.;
    x2 = 0;
    y2 = 47.0;
    glVertex2f(x1,y1);
    glVertex2f(x2,y2);

    //variable wind direction
    if( ddvar ){
      x1=x3=-3;
      x2=x4=3;
      y1=y4=24.5;
      y2=y3=30.5;
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
      glVertex2f(x3,y3);
      glVertex2f(x4,y4);
    }

    // wind speed not observed
    if( ff < 0 ){
      x1=x3=-3;
      x2=x4=3;
      y1=y4=44;
      y2=y3=50;
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
      glVertex2f(x3,y3);
      glVertex2f(x4,y4);
    }

    // wind
    x1 = 0;
    y1 = 47.0;
    x2 = 13;
    y2 = y1 + 7;
    if ( ff < 10 ){
      y1 -= 6;
      y2 -= 6;
    }
    if (ff>=50) {
      glEnd();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      for( ; ff>=50; ff-=50 ){
        x3 = 0;
	y3 = y1;
        y2 -= 15;
        y1 -= 15;
        glBegin(GL_POLYGON);
        glVertex2f(x1,y1);
        glVertex2f(x2,y2);
        glVertex2f(x3,y3);
        glVertex2f(x1,y1);
        glEnd();
      }
      y1 -= 6;
      y2 -= 6;
      glBegin(GL_LINES);
    }
    for( ; ff>=10; ff-=10 ){
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
      y1 -= 6;
      y2 -= 6;
    }
    if(ff >= 5 ){
      x2 = (x1+x2)/2;
      y2 = (y1+y2)/2;
      glVertex2f(x1,y1);
      glVertex2f(x2,y2);
    }
    if(current>0){
      glVertex2f(-4,6);
      glVertex2f(0,0);
      glVertex2f(0,0);
      glVertex2f(4,6);
    }
    glEnd();
  }
  glPopMatrix();

}


void ObsPlot::weather(int16 ww, float &TTT, int &zone,
		      float xpos, float ypos, float scale, miString align)
{

  const int auto2man[100]=  {0,1,2,3,4,5,0,0,0,0,
		       10,76,13,0,0,0,0,0,18,0,
		       28,21,20,21,22,24,29,38,38,37,
		       41,41,43,45,47,49,0,0,0,0,
		       63,63,65,63,65,74,75,66,67,0,
		       53,51,53,55,56,57,57,58,59,0,
		       63,61,63,65,66,67,67,68,69,0,
		       73,71,73,75,79,79,79,0,0,0,
		       81,80,81,82,85,86,86,0,0,0,
		       17,17,95,96,17,97,99,0,0,0};

  if(ww>99) ww= auto2man[ww-100];

  int index;

  index = iptab[1247+ww];
  if( ww == 7 && zone == 99 )    index = iptab[1247+127];
  if( TTT<0 && (ww>92 && ww<98)) index = iptab[1247+ww+10];
  if((TTT>=0 && TTT<3) && (ww==95 || ww==97)) index = iptab[1247+ww+20];

  if( index > 3 ){
    float idx = iptab[1211+index];
    xpos += (idx + (22-idx)*0.2)*scale;
    ypos -= 4*scale;
  }
  //do not plot ww<4

  int n=itab[40+ww];
  if( ww == 7 && zone == 99 )    n = itab[140];
  if( ww>92 && TTT>-1000 && TTT<1000 ) {
    if( TTT<0 && (ww>92 && ww<96)) n = itab[48+ww];
    if( TTT<0 && (ww == 97)) n = itab[144];
    if((TTT>=0 && TTT<3) && (ww==95)) n = itab[145];
    if((TTT>=0 && TTT<3) && (ww==97)) n = itab[146];
  }

  symbol(n,xpos,ypos,0.8*scale,align);
}

void ObsPlot::pastWeather(int w, float xpos, float ypos,
			  float scale, miString align)
{

  const int auto2man[10] = {0,4,3,4,6,5,6,7,8,9};

  if(w>9) w=auto2man[w-10];

  if(w<2) return;

  symbol(itab[158+w], xpos, ypos,scale);

}

void ObsPlot::wave(const float& PwPw, const float& HwHw,
		   float x, float y, miString align)
{
  ostringstream cs;

  int pwpw = float2int(PwPw);
  int hwhw = float2int(HwHw * 2); // meters -> half meters
  x *= scale;
  y *= scale;
  if( pwpw>=0 && pwpw<100 ){
    cs.width(2);
    cs.fill('0');
    cs << pwpw ;
  }
  else
    cs << "xx";
  cs << "/";
  if( hwhw>=0 && hwhw<100){
    cs.width(2);
    cs.fill('0');
    cs << hwhw ;
  }
  else
    cs << "xx" ;

  miString str=cs.str();
  const char * c=str.c_str();

  if( align == "right" ) {
    float w,h;
    fp->getStringSize(c, w, h);
    w*=fontsizeScale;
    x -= w;
  }

  fp->drawStr(c,x,y, 0.0);

}


bool ObsPlot::readTable(const miString& type,
			const miString& filename){

  //   Initialize itab and iptab from file.
#ifdef DEBUGPRINT
  cerr << "++ ObsPlot::readTable ++" << endl;
#endif

  const int ITAB=380;
  int   size,psize;

  itab=  0;
  iptab= 0;

  if( type == "synop" || type == "list")
    psize= 11320;
  else if( type == "metar" )
    psize= 3072;
  else
    return false;  // table for unknown plot type;

  FILE *fp = fopen(filename.c_str(),"r");
  if (!fp){
    cerr << "ObsPlot::readTable: Couldn't open "<<filename<<endl;
    return false;
  }

  // itab sored in 2 fortran records (each 256 shorts)
  size = 512 + psize;

  short *table = new short[size];

  if (fread(table, 2, size, fp) != size) {
    cerr << "ObsPlot::readTable: Error reading "<<filename<<endl;
    fclose(fp);
    delete[] table;
    return false;
  }

  fclose(fp);

  // indexing as in fortran code (start from element 1)

  itab=  new short[ITAB+1];
  iptab= new short[psize+1];

  for(int i=0; i<ITAB; i++)
    itab[i+1] = table[i];

  for(int i=512; i<size; i++)
    iptab[i-511] = table[i];

  if( type == "synop" )
    for( int i = itab[232]+3; i <= itab[267]+3; i += 16 )
      iptab[i]=1;
  else if( type == "metar" )
    for( int i = itab[162]+3; i <= itab[197]+3; i += 16 )
      iptab[i]=1;


  delete[] table;

  return true;
}

void ObsPlot::decodeCriteria(miString critStr)
{

  critStr = critStr.substr(critStr.find_first_of("=")+1,critStr.size()-1);
  vector<miString> vstr = critStr.split(";");
  int nvstr=vstr.size();
  for( int i=0; i<nvstr; i++){
    vector<miString> vcrit = vstr[i].split(",");
    if( vcrit.size()<2 ) continue;
    miString sep;
    Sign sign;
    miString parameter;
    float limit=0.0;
    if( vcrit[0].contains(">") ){
      sep = ">";
      sign = more_than;
    } else if( vcrit[0].contains("<") ){
      sep = "<";
      sign = less_than;
    } else if( vcrit[0].contains("=") ){
      sep = "=";
      sign = equal_to;
    } else{
      sign = no_sign;
    }
    if(sep.exists()){
      vector<miString> sstr = vcrit[0].split(sep);
      if(sstr.size()!=2) continue;
      parameter = sstr[0];
      limit = atof(sstr[1].cStr());
      if (knotParameters.count(parameter) ) {
        limit = knots2ms(limit);
      }
    } else {
      parameter = vcrit[0];
    }

    if(vcrit[1].downcase() == "plot"){
      pcriteria=true;
      plotCriteria pc;
      pc.limit=limit;
      pc.sign=sign;
      pc.plot=true;
      plotcriteria[parameter].push_back(pc);
    }else if(vcrit.size()>2 && vcrit[2].downcase() == "marker"){
      mcriteria=true;
      markerCriteria mc;
      mc.limit=limit;
      mc.sign=sign;
      mc.marker=vcrit[1];
      markercriteria[parameter].push_back(mc);
    }else {
      Colour c(vcrit[1]);
      colourCriteria cc;
      cc.limit=limit;
      cc.sign=sign;
      cc.colour = c;
      if (vcrit.size()==3 && vcrit[2].downcase() == "total" ){
	tccriteria=true;
	totalcolourcriteria[parameter].push_back(cc);
      }else {
	ccriteria=true;
	colourcriteria[parameter].push_back(cc);
      }
    }
  }

}


void ObsPlot::checkColourCriteria(const miString& param, float value)
{

  //reset colour
  glColor4ubv(colour.RGBA());

  bool thiscolour=false;
  Colour col;
  map<miString,vector<colourCriteria> >::iterator p=colourcriteria.find(param);

  if(p==colourcriteria.end()) return;

  //RRR=990 - Precipitation, but less than 0.1 mm (0.0)
  //RRR=0   - No precipitation (0.)
  if(param=="RRR"){
    if(value<0.1) value =-1.0;
    if(value>989) value = 0.0;
  }

  int n=colourcriteria[param].size();
  for(int i=0; i<n; i++ ){
    float delta = fabsf(value)*0.01;
    if( (p->second[i].sign == less_than && value<p->second[i].limit)
	|| (p->second[i].sign == more_than && value>p->second[i].limit)
	|| (p->second[i].sign == equal_to  &&
	    (value>p->second[i].limit-delta &&
	     value<p->second[i].limit+delta))
	|| (p->second[i].sign == no_sign)){
      col=p->second[i].colour;
      thiscolour=true;
    }
  }
  if(thiscolour){
    glColor4ubv(col.RGBA());
  }

}


bool ObsPlot::checkPlotCriteria(int index)
{

  if(!pcriteria) return true;
  bool doPlot=false;

  map<miString,vector<plotCriteria> >::iterator p= plotcriteria.begin();

  for(;p!=plotcriteria.end(); p++){
    int ncrit=p->second.size();
    float value;
    if(plottype =="ascii"){
      int n=asciip[index].size();
      int j=0;
      while(j<n && asciiColumnName[j]!=p->first) j++;
      if(j==n) continue;
      value = atof(asciip[index][j].cStr());
    } else {
      if(obsp[index].fdata.count(p->first))
	value=obsp[index].fdata[p->first];
      else if (p->first.downcase() != obsp[index].dataType )
	continue;
    }

  //RRR=990 - Precipitation, but less than 0.1 mm (0.0)
  //RRR=0   - No precipitation (0.)
  if(p->first=="RRR"){
    if(value<0.1) value =-1.0;
    if(value>989) value = 0.0;
  }

    bool bplot=true;
    bool cplot=false;
    for(int i=0; i<ncrit; i++){
      float delta = fabsf(value)*0.01;
      if( (p->second[i].sign == less_than && value<p->second[i].limit)
	  || (p->second[i].sign == more_than && value>p->second[i].limit)
	  || (p->second[i].sign == equal_to &&
	    (value>p->second[i].limit-delta &&
	     value<p->second[i].limit+delta))
	  || (p->second[i].sign == no_sign) )
	cplot=true;
      bplot = bplot && cplot;
      cplot=false;
    }
    doPlot=doPlot || bplot;
  }

  return doPlot;
}

void  ObsPlot::checkTotalColourCriteria(int index)
{

  map<miString,vector<colourCriteria> >::iterator p
    = totalcolourcriteria.begin();

  for(;p!=totalcolourcriteria.end(); p++){
    int ncrit=p->second.size();
    float value;
    if(plottype =="ascii"){
      int n=asciip[index].size();
      int j=0;
      while(j<n && asciiColumnName[j]!=p->first) j++;
      if(j==n) continue;
      value = atof(asciip[index][j].cStr());
    } else {
      if(obsp[index].fdata.count(p->first))
	value=obsp[index].fdata[p->first];
      else if (p->first.downcase() != obsp[index].dataType )
	continue;
    }

  //RRR=990 - Precipitation, but less than 0.1 mm (0.0)
  //RRR=0   - No precipitation (0.)
  if(p->first=="RRR"){
    if(value<0.1) value =-1.0;
    if(value>989) value = 0.0;
  }

    for(int i=0; i<ncrit; i++){
      if( (p->second[i].sign == less_than && value<p->second[i].limit)
	  || (p->second[i].sign == more_than && value>p->second[i].limit)
	  || (p->second[i].sign == equal_to && value==p->second[i].limit)
	  ||(p->second[i].sign == no_sign) )
	colour = p->second[i].colour;
    }
  }

  glColor4ubv(colour.RGBA());

}

miString  ObsPlot::checkMarkerCriteria(int index)
{

  miString marker=image;

  map<miString,vector<markerCriteria> >::iterator p
    = markercriteria.begin();

  for(;p!=markercriteria.end(); p++){
    int ncrit=p->second.size();
    float value;
    if(plottype =="ascii"){
      int n=asciip[index].size();
      int j=0;
      while(j<n && asciiColumnName[j]!=p->first) j++;
      if(j==n) continue;
      value = atof(asciip[index][j].cStr());
    } else {
      if(obsp[index].fdata.count(p->first))
	value=obsp[index].fdata[p->first];
      else if (p->first.downcase() != obsp[index].dataType )
	continue;
    }

    for(int i=0; i<ncrit; i++){
      if( (p->second[i].sign == less_than && value<p->second[i].limit)
	  || (p->second[i].sign == more_than && value>p->second[i].limit)
	  || (p->second[i].sign == equal_to && value==p->second[i].limit)
	  ||(p->second[i].sign == no_sign) )
	marker = p->second[i].marker;
    }
  }

  return marker;
}

void ObsPlot::changeParamColour(const miString& param, bool select)
{

  ccriteria = select;
  if(select){
    colourCriteria cc;
    cc.sign = no_sign;
    cc.colour = Colour("red");;
    colourcriteria[param].push_back(cc);
  } else {
    colourcriteria.erase(param);
  }

}

void ObsPlot::parameterDecode(miString parameter, bool add)
{

  paramColour[parameter]=colour;
  if(parameter=="txtxtx")
    paramColour["tntntn"]=colour;
  if(parameter=="tntntn")
    paramColour["txtxtx"]=colour;
  if(parameter=="pwapwa")
    paramColour["hwahwa"]=colour;
  if(parameter=="hwahwa")
    paramColour["pwapwa"]=colour;

  if(parameter=="txtxtx" || parameter=="tntntn")
    parameter="txtn";
  else if(parameter=="pwapwa" || parameter=="hwahwa")
    parameter="hwahwa";
  else if(parameter=="hw1Pw1" || parameter=="hw1hw1")
    parameter="pw1hw1";
  else if (parameter=="dd_ff" || parameter=="vind")
    parameter="vind";
  else if (parameter=="kjtegn" )
    parameter="id";
  else if (parameter=="tid" )
    parameter="time";
  else if (parameter=="dato" )
    parameter="date";

  pFlag[parameter]=add;
}
