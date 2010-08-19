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
#ifndef diObsPlot_h
#define diObsPlot_h

#include <diPlot.h>
#include <diObsData.h>
#include <GL/gl.h>
#include <set>
#ifdef ROADOBS
#include <roadAPI/diStation.h>
#endif

/**

  \brief Plot observations

  - synop plot
  - metar plot
  - list plot

*/
class ObsPlot : public Plot {

private:
  vector<ObsData> obsp;
  //obs positions
  float *x, *y;

  //from plotInfo
  miutil::miString infostr;
  float Scale;
  bool allObs;     //plot all observations
  int numPar;      //number of parameters to plot
  float density;     // distannce between obs
  Colour origcolour;
  Colour colour;
  Colour mslpColour1;
  Colour mslpColour2;
  bool vertical;
  bool showpos;
  bool devfield;
  bool onlypos;
  bool showOnlyPrioritized;
  miutil::miString image;
  int level;
  int leveldiff;
  bool levelAsField;
  miutil::miString plottype;
  vector<miutil::miString> datatypes;
  bool priority;
  miutil::miString priorityFile;
  bool tempPrecision; //temp and dewpoint in desidegrees or degrees
  bool parameterName; //parameter name printed in front of value (ascii only)
  bool allAirepsLevels;
  int timeDiff;
  bool moretimes; //if true, sort stations according to obsTime
  miutil::miTime Time;
  bool localTime;  //Use Time, not ctime
  int undef;
  miutil::miString annotation;
  vector<miutil::miString> labels;    // labels from ascii-files or PlotModule(edit)
  float fontsizeScale; //needed when postscript font != X font
  float current; //cuurent, not wind
  bool firstplot;
  bool beendisabled; // obsplot was disabled while area changed

  int startxy; //used in getposition/obs_mslp

  set<miutil::miString> knotParameters;
  //Name and last modification time of files used
  vector<miutil::miString> fileNames;
  vector<long> modificationTime;

  //Criteria
  enum Sign{
    less_than = 0,
    more_than = 1,
    equal_to  = 2,
    no_sign   = 3
  };

  struct plotCriteria{
    bool plot;
    float limit;
    Sign sign;
  };

  struct colourCriteria{
    float limit;
    Sign sign;
    Colour colour;
  };

  struct markerCriteria{
    float limit;
    Sign sign;
    miutil::miString marker;
  };

  map<miutil::miString,vector<plotCriteria> > plotcriteria;
  map<miutil::miString,vector<colourCriteria> > colourcriteria;
  map<miutil::miString,vector<colourCriteria> > totalcolourcriteria;
  map<miutil::miString,vector<markerCriteria> > markercriteria;

  bool pcriteria;
  bool ccriteria;
  bool tccriteria;
  bool mcriteria;

  //which parameters to plot
  map<miutil::miString,bool> pFlag;

//Positions of plotted observations
  struct UsedBox {
    float x1,x2,y1,y2;
  };
  //  static
  static vector<float> xUsed;
  static vector<float> yUsed;
  static vector<UsedBox> usedBox;

  // static priority file
  static miutil::miString currentPriorityFile;
  static vector<miutil::miString> priorityList;

  // static synop and metar plot tables
  static short *itabSynop;
  static short *iptabSynop;
  static short *itabMetar;
  static short *iptabMetar;

  short *itab;
  short *iptab;

  float PI;
  float scale;
  GLuint circle;


  //Metar
  struct metarww{
    int lww, lwwg;
  };
  static map<miutil::miString,metarww> metarMap;
  static map<int,int> lwwg2;

  //which obs will be plotted
  bool next;
  bool previous;
  bool  thisObs;
  int plotnr;
  int maxnr;
  bool fromFile;
  vector<int> list_plotnr;    // list of all stations, value = plotnr
  vector<int> nextplot;  // list of stations that will be plotted
  vector<int> notplot;   // list of stations that will be plottet as x
  vector<int> all_this_area; // all stations within area
  vector<int> all_stations; // all stations, from last plot or from file
  vector<int> all_from_file; // all stations, from file or priority list
  //id of all stations shown, sorted by plot type
  static map< miutil::miString, vector<miutil::miString> > visibleStations;

  float areaFreeSpace,areaFreeWindSize;
  float areaFreeXsize,areaFreeYsize;
  float areaFreeXmove[4],areaFreeYmove[4];

  //Hqc
  miutil::miString hqcFlag;  //which parameter is flagged
  bool flaginfo;
  Colour flagColour;
  map<miutil::miString,Colour> paramColour;
  miutil::miString selectedStation;
  miutil::miString mark_parameter;


  bool readTable(const miutil::miString& type, const miutil::miString& filename);
  void readPriorityFile(const miutil::miString& filename);

  void decodeCriteria(miutil::miString critStr);
  bool checkPlotCriteria(int index);
  void checkTotalColourCriteria(int index);
  miutil::miString checkMarkerCriteria(int index);
  void checkColourCriteria(const miutil::miString& param, float value);
  void parameterDecode(miutil::miString , bool =true);

  bool positionFree(const float&, const float&, float);
  bool positionFree(float,float, float,float);
  void areaFreeSetup(float scale, float space, int num,
		     float xdist, float ydist);
  bool areaFree(int idx);

  void printUndef(float& , float&, miutil::miString ="left");
  void printList(float f, float& xpos, float& ypos,
		   int precision, miutil::miString align="left", miutil::miString opt="");
  void printNumber(float, float, float, miutil::miString ="left",
		   bool =false, bool =false);
  void printAvvik(float, float, float, miutil::miString ="left");
  void printTime(miutil::miTime, float, float, miutil::miString ="left", miutil::miString ="");
  void printListString(const char *, float&, float& ,miutil::miString ="left");
  void printString(const char *, float , float ,miutil::miString ="left",bool =false);
  void metarSymbol(miutil::miString, float, float, int&);
  void metarString2int(miutil::miString ww, int intww[]);
  void initMetarMap();
  void metarWind(int,int,float &, int &);
  void arrow(float& angle, float xpos, float ypos, float scale=1.);
  void zigzagArrow(float& angle, float xpos, float ypos, float scale=1.);
  void symbol(int, float, float,float scale=1, miutil::miString align="left");
  void cloudCover(const float& fN, const float& radius);
  void plotWind(int dd,float ff_ms, bool ddvar,float &radius,float current=-1);
  //  void plotArrow(int,int, bool,float &);
  void weather(int16 ww, float & TTT, int& zone,
	       float x, float y, float scale=1, miutil::miString align="left");
  void pastWeather(int w, float x, float y,
		   float scale=1, miutil::miString align="left");
  void wave(const float& PwPw, const float& HwHw,
	    float x, float y, miutil::miString align="left");
  int visibility(float vv, bool ship);
  int vis_direction(float dv);
  void amountOfClouds(int16, int16, float,float);
  void checkAccumulationTime(ObsData &dta);
  void plotSynop(int index);
  void plotList(int index);
  void plotAscii(int index);
  void plotMetar(int index);
#ifdef ROADOBS
  void plotRoadobs(int index);
#endif
  void priority_sort(void);
  void time_sort(void);
  int ms2knots(float ff) {return (float2int(ff*3600.0/1852.0));}
  float knots2ms(float ff) {return (ff*1852.0/3600.0);}



public:
  // Constructors
  ObsPlot();
  ObsPlot(const ObsPlot &rhs);
  // Destructor
  ~ObsPlot();

  // Assignment operator
  ObsPlot& operator=(const ObsPlot &rhs);
  // Equality operator
  bool operator==(const ObsPlot &rhs) const;

  bool plot();
  bool plot(const int){return false;}
  bool updateObs();
  void clearModificationTime();
  void setModificationTime(const miutil::miString& fname);
  bool prepare(const miutil::miString&);
  bool setData(void);
  void logStations();
  void readStations();
  void clear();
  static void clearPos();
  void getObsAnnotation(miutil::miString &, Colour &);
  bool getDataAnnotations(vector<miutil::miString>& anno);
  void setObsAnnotation(miutil::miString &anno){annotation =anno;}
  vector<miutil::miString> getObsExtraAnnotations(){return labels;}
  void setLabel(const miutil::miString& pin){labels.push_back(pin);}
  bool getPositions(vector<float>&,vector<float>&);
  int  getPositions(float*,float*,int);
  int  numPositions();
  void obs_mslp(float *);
  bool findObs(int,int);
  bool getObsName(int xx,int yy, miutil::miString& station);
  void nextObs(bool);
  vector<miutil::miString> getStations();
  void putStations(vector<miutil::miString>);
  miutil::miString getInfoStr(){return infostr;}
  bool mslp(){return devfield;}
  int float2int(float f){return (int)(f > 0.0 ? f + 0.5 : f - 0.5);}
  void changeParamColour(const miutil::miString& param, bool select);

  bool moreTimes(){return moretimes;}
  vector<miutil::miString>& dataTypes(){return datatypes;}
  miutil::miTime getObsTime(){return Time;}
  void setObsTime(const miutil::miTime& t){Time=t;}
  int getTimeDiff(){return timeDiff;}
  void setTimeDiff(int diff){timeDiff=diff;}
  void setCurrent(float cur){current=cur;}
  miutil::miString plotType(){return plottype;}
  bool LevelAsField(){return levelAsField;}
  bool AllAirepsLevels(){return allAirepsLevels;}
  int getLevelDiff(){return leveldiff;}
  int getLevel(){return level;}
  void resetObs(int num){obsp.resize(num);}
  int sizeObs(){return obsp.size();}
  void removeObs(){obsp.pop_back();}
  ObsData& getNextObs();
  void addObs(vector<ObsData> vdata){obsp = vdata;}
  bool timeOK(const miutil::miTime& t);
  //get get pressure level etc from field (if needed)
  void updateLevel(const miutil::miString& dataType);

  //ascii files
  bool asciiHeader;
  bool asciiData;
  bool asciiOK;
  bool asciiKnots;
  int  asciiSkipDataLines;
  miutil::miString asciiDataName;
  miutil::miTime   asciiMainTime;
  miutil::miTime   asciiStartTime;
  miutil::miTime   asciiEndTime;
  vector<miutil::miString> asciiColumnName;
  vector<miutil::miString> asciiColumnTooltip;
  vector<miutil::miString> asciiColumnType;
  vector<miutil::miString> asciiColumnHide;
  vector<miutil::miString> asciiColumnUndefined;

  vector<miutil::miTime> asciiTime;

  vector< vector<miutil::miString> > asciip;
  vector<int> asciiLengthMax;

  map<miutil::miString,int> asciiColumn; //column index(time, x,y,dd,ff etc)

  vector<miutil::miString> asciiParameter;
  vector<int>      asciipar;
  bool             asciiWind;

  vector<int> asciidd;
  vector<float> asciiff;

  // observations from road
#ifdef ROADOBS
  bool roadobsHeader;
  bool roadobsData;
  bool roadobsOK;
  bool roadobsKnots;
  int  roadobsSkipDataLines;
  miutil::miString roadobsDataName;
  miutil::miTime   roadobsMainTime;
  miutil::miTime   roadobsStartTime;
  miutil::miTime   roadobsEndTime;
  vector<miutil::miString> roadobsColumnName;
  vector<miutil::miString> roadobsColumnTooltip;
  vector<miutil::miString> roadobsColumnType;
  vector<miutil::miString> roadobsColumnHide;
  vector<miutil::miString> roadobsColumnUndefined;

  vector<miutil::miTime> roadobsTime;

  //vector< vector<miutil::miString> > roadobsp;
  // needed to get data from road ON DEMAND
  miutil::miString filename;
  miutil::miString databasefile;
  miutil::miString stationfile;
  miutil::miString headerfile;
  miutil::miTime filetime;
  map <int, vector<miutil::miString> > roadobsp;
  vector<road::diStation>  * stationlist;
  vector<road::diStation> stations_to_plot;
  bool preparePlot(void);
  vector<int> roadobsLengthMax;

  map<miutil::miString,int> roadobsColumn; //column index(time, x,y,dd,ff etc)

  vector<miutil::miString> roadobsParameter;
  vector<int>      roadobspar;
  bool             roadobsWind;

  vector<int> roadobsdd;
  vector<float> roadobsff;
  static int ucount;
#endif

//Hqc
  bool flagInfo(){return flaginfo;}
  void setHqcFlag(const miutil::miString& flag){hqcFlag=flag;}
  void setSelectedStation(const miutil::miString& station){selectedStation=station;}
};

#endif
