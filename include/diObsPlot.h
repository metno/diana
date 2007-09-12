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
  miString infostr;
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
  miString image;
  int level;
  int leveldiff;
  bool levelAsField;
  miString plottype;
  vector<miString> datatypes;
  bool priority;
  miString priorityFile;
  bool tempPrecision; //temp and dewpoint in desidegrees or degrees
  bool parameterName; //parameter name printed in front of value (ascii only)
  bool allAirepsLevels;
  int timeDiff;
  bool moretimes; //if true, sort stations according to obsTime
  miTime Time;
  bool localTime;  //Use Time, not ctime
  int undef;
  miString annotation;
  vector<miString> labels;    // labels from ascii-files or PlotModule(edit)
  float fontsizeScale; //needed when postscript font != X font
  float current; //cuurent, not wind
  bool firstplot;
  bool beendisabled; // obsplot was disabled while area changed

  int startxy; //used in getposition/obs_mslp

  //Name and last modification time of files used
  vector<miString> fileNames;
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
    miString marker;
  };

  map<miString,vector<plotCriteria> > plotcriteria;
  map<miString,vector<colourCriteria> > colourcriteria;
  map<miString,vector<colourCriteria> > totalcolourcriteria;
  map<miString,vector<markerCriteria> > markercriteria;

  bool pcriteria;
  bool ccriteria;
  bool tccriteria;
  bool mcriteria;

  //which parameters to plot
  map<miString,bool> pFlag;

//Positions of plotted observations
  struct UsedBox {
    float x1,x2,y1,y2;
  };
  //  static
  static vector<float> xUsed;
  static vector<float> yUsed;
  static vector<UsedBox> usedBox;

  // static priority file
  static miString currentPriorityFile;
  static vector<miString> priorityList;

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
  static map<miString,metarww> metarMap;
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
  static map< miString, vector<miString> > visibleStations;

  float areaFreeSpace,areaFreeWindSize;
  float areaFreeXsize,areaFreeYsize;
  float areaFreeXmove[4],areaFreeYmove[4];

  //Hqc
  miString hqcFlag;  //which parameter is flagged
  bool flaginfo;
  Colour flagColour;
  map<miString,Colour> paramColour;
  miString selectedStation;
  miString mark_parameter;


  bool readTable(const miString& type, const miString& filename);
  void readPriorityFile(const miString& filename);

  void decodeCriteria(miString critStr);
  bool checkPlotCriteria(int index);
  void checkTotalColourCriteria(int index);
  miString checkMarkerCriteria(int index);
  void checkColourCriteria(const miString& param, float value);
  void parameterDecode(miString , bool =true);

  bool positionFree(const float&, const float&, float);
  bool positionFree(float,float, float,float);
  void areaFreeSetup(float scale, float space, int num,
		     float xdist, float ydist);
  bool areaFree(int idx);

  void printUndef(float& , float&, miString ="left");
  void printList(float f, float& xpos, float& ypos,
		   int precision, miString align="left", miString opt="");
  void printNumber(float, float, float, miString ="left",
		   bool =false, bool =false);
  void printAvvik(float, float, float, miString ="left");
  void printTime(miTime, float, float, miString ="left", miString ="");
  void printListString(const char *, float&, float& ,miString ="left");
  void printString(const char *, float , float ,miString ="left",bool =false);
  void metarSymbol(miString, float, float, int&);
  void metarString2int(miString ww, int intww[]);
  void initMetarMap();
  void metarWind(int,int,float &, int &);
  void arrow(float& angle, float xpos, float ypos, float scale=1.);
  void zigzagArrow(float& angle, float xpos, float ypos, float scale=1.);
  void symbol(int, float, float,float scale=1, miString align="left");
  void cloudCover(const float& fN, const float& radius);
  void plotWind(int dd,float ff_ms, bool ddvar,float &radius,float current=-1);
  //  void plotArrow(int,int, bool,float &);
  void weather(int16 ww, float & TTT, int& zone,
	       float x, float y, float scale=1, miString align="left");
  void pastWeather(int w, float x, float y,
		   float scale=1, miString align="left");
  void wave(const float& PwPw, const float& HwHw,
	    float x, float y, miString align="left");
  int visibility(float vv, bool ship);
  int vis_direction(float dv);
  void amountOfClouds(int16, int16, float,float);
  void plotSynop(int index);
  void plotList(int index);
  void plotAscii(int index);
  void plotMetar(int index);
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
  void setModificationTime(const miString& fname);
  bool prepare(const miString&);
  bool setData(void);
  void logStations();
  void readStations();
  void clear();
  static void clearPos();
  void getObsAnnotation(miString &, Colour &);
  bool getDataAnnotations(vector<miString>& anno);
  void setObsAnnotation(miString &anno){annotation =anno;}
  vector<miString> getObsExtraAnnotations(){return labels;}
  void setLabel(const miString& pin){labels.push_back(pin);}
  bool getPositions(vector<float>&,vector<float>&);
  int  getPositions(float*,float*,int);
  int  numPositions();
  void obs_mslp(float *);
  bool findObs(int,int);
  bool getObsName(int xx,int yy, miString& station);
  void nextObs(bool);
  vector<miString> getStations();
  void putStations(vector<miString>);
  miString getInfoStr(){return infostr;}
  bool mslp(){return devfield;}
  int float2int(float f){return (int)(f > 0.0 ? f + 0.5 : f - 0.5);}
  void changeParamColour(const miString& param, bool select);

  bool moreTimes(){return moretimes;}
  vector<miString>& dataTypes(){return datatypes;}
  miTime getObsTime(){return Time;}
  void setObsTime(const miTime& t){Time=t;}
  int getTimeDiff(){return timeDiff;}
  void setTimeDiff(int diff){timeDiff=diff;}
  void setCurrent(float cur){current=cur;}
  miString plotType(){return plottype;}
  bool LevelAsField(){return levelAsField;}
  bool AllAirepsLevels(){return allAirepsLevels;}
  int getLevelDiff(){return leveldiff;}
  int getLevel(){return level;}
  void resetObs(int num){obsp.resize(num);}
  void removeObs(){obsp.pop_back();}
  ObsData& getNextObs();
  void addObs(vector<ObsData> vdata){obsp = vdata;}
  bool timeOK(const miTime& t);
  //get get pressure level etc from field (if needed)
  void updateLevel(const miString& dataType);

  //ascii files
  bool asciiHeader;
  bool asciiData;
  bool asciiOK;
  bool asciiKnots;
  int  asciiSkipDataLines;
  miString asciiDataName;
  miTime   asciiMainTime;
  miTime   asciiStartTime;
  miTime   asciiEndTime;
  vector<miString> asciiColumnName;
  vector<miString> asciiColumnType;
  vector<miString> asciiColumnHide;
  vector<miString> asciiColumnUndefined;

  vector<miTime> asciiTime;

  vector< vector<miString> > asciip;
  vector<int> asciiLengthMax;

  map<miString,int> asciiColumn; //column index(time, x,y,dd,ff etc)

  vector<miString> asciiParameter;
  vector<int>      asciipar;
  bool             asciiWind;

  vector<int> asciidd;
  vector<float> asciiff;


//Hqc
  bool flagInfo(){return flaginfo;}
  void setHqcFlag(const miString& flag){hqcFlag=flag;}
  void setSelectedStation(const miString& station){selectedStation=station;}
};

#endif
