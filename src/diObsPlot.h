/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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
#include <vector>
#ifdef ROADOBS
#ifdef NEWARK_INC
#include <newarkAPI/diStation.h>
#else
#include <roadAPI/diStation.h>
#endif
#endif

/**

  \brief Plot observations

  - synop plot
  - metar plot
  - list plot

*/
class ObsPlot : public Plot {

private:
  std::vector<ObsData> obsp;
  std::map< miutil::miString, int > idmap; // maps obsData with id to index in obsp
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
  bool vertical_orientation;
  bool left_alignment;
  bool showpos;
  bool devfield;
  bool onlypos;
  bool showOnlyPrioritized;
  miutil::miString image;
  int level;
  int leveldiff;
  bool levelAsField;
  miutil::miString plottype;
  miutil::miString currentDatatype;
  std::vector<miutil::miString> datatypes;
  bool priority;
  miutil::miString priorityFile;
  bool tempPrecision; //temp and dewpoint in desidegrees or degrees
  bool parameterName; //parameter name printed in front of value (ascii only)
  bool qualityFlag;
  bool wmoFlag;
  bool allAirepsLevels;
  int timeDiff;
  bool moretimes; //if true, sort stations according to obsTime
  miutil::miTime Time;
  bool localTime;  //Use Time, not ctime
  int undef;
  miutil::miString annotation;
  std::vector<miutil::miString> labels;    // labels from ascii-files or PlotModule(edit)
  float fontsizeScale; //needed when postscript font != X font
  float current; //cuurent, not wind
  bool firstplot;
  bool beendisabled; // obsplot was disabled while area changed

  int startxy; //used in getposition/obs_mslp

  std::set<miutil::miString> knotParameters;
  //Name and last modification time of files used
  std::vector<miutil::miString> fileNames;
  std::vector<long> modificationTime;

  enum flag {QUALITY_GOOD = 4};

  //Criteria
  enum Sign{
    less_than = 0,
    less_than_or_equal_to = 1,
    more_than = 2,
    more_than_or_equal_to = 3,
    equal_to  = 4,
    no_sign   = 5
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

  std::map<miutil::miString,std::vector<plotCriteria> > plotcriteria;
  std::map<miutil::miString,std::vector<colourCriteria> > colourcriteria;
  std::map<miutil::miString,std::vector<colourCriteria> > totalcolourcriteria;
  std::map<miutil::miString,std::vector<markerCriteria> > markercriteria;

  bool pcriteria;
  bool ccriteria;
  bool tccriteria;
  bool mcriteria;

  //which parameters to plot
  std::map<miutil::miString,bool> pFlag;

//Positions of plotted observations
  struct UsedBox {
    float x1,x2,y1,y2;
  };
  //  static
  static std::vector<float> xUsed;
  static std::vector<float> yUsed;
  static std::vector<UsedBox> usedBox;

  // static priority file
  static miutil::miString currentPriorityFile;
  static std::vector<miutil::miString> priorityList;

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
  static std::map<miutil::miString,metarww> metarMap;
  static std::map<int,int> lwwg2;

  //which obs will be plotted
  bool next;
  bool previous;
  bool  thisObs;
  int plotnr;
  int maxnr;
  bool fromFile;
  std::vector<int> list_plotnr;    // list of all stations, value = plotnr
  std::vector<int> nextplot;  // list of stations that will be plotted
  std::vector<int> notplot;   // list of stations that will be plottet as x
  std::vector<int> all_this_area; // all stations within area
  std::vector<int> all_stations; // all stations, from last plot or from file
  std::vector<int> all_from_file; // all stations, from file or priority list
  //id of all stations shown, sorted by plot type
  static std::map< miutil::miString, std::vector<miutil::miString> > visibleStations;

  float areaFreeSpace,areaFreeWindSize;
  float areaFreeXsize,areaFreeYsize;
  float areaFreeXmove[4],areaFreeYmove[4];

  //Hqc
  miutil::miString hqcFlag;  //which parameter is flagged
  bool flaginfo;
  Colour flagColour;
  std::map<miutil::miString,Colour> paramColour;
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
  void cloudCoverAuto(const float& fN, const float &radius);
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
  void amountOfClouds_1(int16 Nh, int16 h, float x, float y, bool metar=false);
  void amountOfClouds_1_4(int16 Ns1, int16 hs1, int16 Ns2, int16 hs2, int16 Ns3, int16 hs3, int16 Ns4, int16 hs4, float x, float y, bool metar=false);
  void checkAccumulationTime(ObsData &dta);
  void checkGustTime(ObsData &dta);
  void checkMaxWindTime(ObsData &dta);
  void plotSynop(int index);
  void plotList(int index);
  void plotAscii(int index);
  void plotMetar(int index);
#ifdef ROADOBS
  void plotRoadobs(int index);
  void plotDBSynop(int index);
  void plotDBMetar(int index);
#endif
  void priority_sort(void);
  void time_sort(void);



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
  bool getDataAnnotations(std::vector<miutil::miString>& anno);
  void setObsAnnotation(miutil::miString &anno){annotation =anno;}
  std::vector<miutil::miString> getObsExtraAnnotations(){return labels;}
  void setLabel(const miutil::miString& pin){labels.push_back(pin);}
  void setLabels(const std::vector<miutil::miString>& l){labels = l;}
  bool getPositions(std::vector<float>&, std::vector<float>&);
  int  getPositions(float*,float*,int);
  int  numPositions();
  void obs_mslp(float *);
  bool findObs(int,int);
  bool getObsName(int xx,int yy, miutil::miString& station);
  void nextObs(bool);
  std::vector<miutil::miString> getStations();
  void putStations(std::vector<miutil::miString>);
  miutil::miString getInfoStr(){return infostr;}
  bool mslp(){return devfield;}
  static int float2int(float f){return (int)(f > 0.0 ? f + 0.5 : f - 0.5);}
  void changeParamColour(const miutil::miString& param, bool select);

  bool moreTimes(){return moretimes;}
  void setDataType(miutil::miString datatype){currentDatatype = datatype;}
  std::vector<miutil::miString>& dataTypes(){return datatypes;}
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
  void mergeMetaData(std::map<miutil::miString, ObsData>& metaData);
  void addObsData(const std::vector<ObsData>& obs);
  void addObsVector(const std::vector<ObsData>& vdata){obsp = vdata;}
  bool timeOK(const miutil::miTime& t);
  //get get pressure level etc from field (if needed)
  void updateLevel(const miutil::miString& dataType);
  static int ms2knots(float ff) {return (float2int(ff*3600.0/1852.0));}
  static float knots2ms(float ff) {return (ff*1852.0/3600.0);}

  std::vector<miutil::miString> getFileNames() const; // Returns the file names containing observation data.

  //Dialog info: Name, tooltip and type of parameter buttons. Used in ascii files
  std::vector<miutil::miString> columnName;

  // observations from road
  bool roadobsData;
#ifdef ROADOBS
  bool roadobsHeader;
  bool roadobsOK;
  bool roadobsKnots;
  int  roadobsSkipDataLines;
  miutil::miString roadobsDataName;
  miutil::miTime   roadobsMainTime;
  miutil::miTime   roadobsStartTime;
  miutil::miTime   roadobsEndTime;
  std::vector<miutil::miString> roadobsColumnName;
  std::vector<miutil::miString> roadobsColumnTooltip;
  std::vector<miutil::miString> roadobsColumnType;
  std::vector<miutil::miString> roadobsColumnHide;
  std::vector<miutil::miString> roadobsColumnUndefined;

  std::vector<miutil::miTime> roadobsTime;

  //vector< std::vector<miutil::miString> > roadobsp;
  // needed to get data from road ON DEMAND
  miutil::miString filename;
  miutil::miString databasefile;
  miutil::miString stationfile;
  miutil::miString headerfile;
  miutil::miTime filetime;
  map <int, std::vector<miutil::miString> > roadobsp;
  std::vector<road::diStation>  * stationlist;
  std::vector<road::diStation> stations_to_plot;
  bool preparePlot(void);
  std::vector<int> roadobsLengthMax;

  std::map<miutil::miString,int> roadobsColumn; //column index(time, x,y,dd,ff etc)

  std::vector<miutil::miString> roadobsParameter;
  std::vector<int>      roadobspar;
  bool             roadobsWind;

  std::vector<int> roadobsdd;
  std::vector<float> roadobsff;
  static int ucount;
#endif

//Hqc
  bool flagInfo(){return flaginfo;}
  void setHqcFlag(const miutil::miString& flag){hqcFlag=flag;}
  void setSelectedStation(const miutil::miString& station){selectedStation=station;}
};

#endif
