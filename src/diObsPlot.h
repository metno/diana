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

#include "diPlot.h"
#include "diGLPainter.h"
#include "diObsData.h"

#ifdef ROADOBS
#ifdef NEWARK_INC
#include <newarkAPI/diStation.h>
#else
#include <roadAPI/diStation.h>
#endif
#endif

#include <set>
#include <vector>

/**
 \brief Plot observations

 - synop plot
 - metar plot
 - list plot
 */
class ObsPlot: public Plot {

private:
  std::vector<ObsData> obsp;
  //obs positions
  float *x, *y;

  struct Parameter {
    std::string name;
    bool knotParam;
    bool tempParam;
    bool rrrParam;
    int symbol;
    int precision;
    Parameter():
    knotParam(0), tempParam(false), rrrParam(false), symbol(-1), precision(0)
    {
    }
  };

  std::vector<Parameter> vparam;

  //from plotInfo
  std::string infostr;
  float markerSize;
  float textSize;
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
  std::string image;
  int level;
  bool levelAsField;
  bool annotations;
  std::string m_plottype;
  std::string dialogname;

  const std::string& plottype() const
  {
    return m_plottype;
  }

  std::string currentDatatype; // BUFR only
  std::vector<std::string> datatypes;
  bool priority;
  std::string priorityFile;
  bool tempPrecision; //temp and dewpoint in desidegrees or degrees
  bool unit_ms; //wind in m/s or knots
  bool parameterName; // parameter name printed in front of value (ascii only), from plotAscii
  bool popupText; // selected parameters in popup window

  enum flag {
    QUALITY_GOOD = 4
  };
  bool qualityFlag; // used in plotSynop and plotList to show only good-quality data

  bool wmoFlag; // used in plotSynop and plotList to show only stations with wmonumber

  bool allAirepsLevels;
  int timeDiff;
  bool moretimes; //if true, sort stations according to obsTime
  miutil::miTime Time;
  std::string annotation;
  std::vector<std::string> labels; // labels from ascii-files or PlotModule(edit)
  std::map<std::string, std::vector<std::string> > popupSpec;
  float current; //cuurent, not wind
  bool firstplot;
  bool beendisabled; // obsplot was disabled while area changed

  int startxy; //used in getposition/obs_mslp

  std::set<std::string> knotParameters;
  //Name and last modification time of files used
  std::vector<std::string> fileNames;
  std::vector<long> modificationTime;

  //Criteria
  enum Sign {
    less_than = 0,
    less_than_or_equal_to = 1,
    more_than = 2,
    more_than_or_equal_to = 3,
    equal_to = 4,
    equal_to_exact = 5,
    no_sign = 6
  };

  struct baseCriteria {
    float limit;
    Sign sign;

    bool match(float value) const;
  };

  struct plotCriteria: public baseCriteria {
    bool plot;
  };

  struct colourCriteria: public baseCriteria {
    Colour colour;
  };

  struct markerCriteria: public baseCriteria {
    std::string marker;
  };

  std::map<std::string, std::vector<plotCriteria> > plotcriteria;
  std::map<std::string, std::vector<colourCriteria> > colourcriteria;
  std::map<std::string, std::vector<colourCriteria> > totalcolourcriteria;
  std::map<std::string, std::vector<markerCriteria> > markercriteria;

  std::map<std::string, bool> sortcriteria;
  //which parameters to plot
  std::map<std::string, bool> pFlag;

  // ------------------------------------------------------------------------
  //Positions of plotted observations
  struct UsedBox {
    float x1, x2, y1, y2;
  };
  //  static
  static std::vector<float> xUsed;
  static std::vector<float> yUsed;
  static std::vector<UsedBox> usedBox;

  bool positionFree(float, float, float, float);
  void areaFreeSetup(float scale, float space, int num, float xdist,
      float ydist);
  bool areaFree(int idx);
  // ------------------------------------------------------------------------

  // static priority file
  static std::string currentPriorityFile;
  static std::vector<std::string> priorityList;

  // static synop and metar plot tables
  static short *itabSynop;
  static short *iptabSynop;
  static short *itabMetar;
  static short *iptabMetar;

  short *itab;
  short *iptab;

  float scale;

  DiGLPainter::GLuint circle;
  void prepareCircle(DiGLPainter* gl);
  void drawCircle(DiGLPainter* gl);
  void finishCircle(DiGLPainter* gl);

  //Metar
  struct metarww {
    int lww, lwwg;
  };
  static std::map<std::string, metarww> metarMap;
  static std::map<int, int> lwwg2;
  // only METAR, called from setData
  void initMetarMap();

  //which obs will be plotted
  bool next;
  bool previous;
  bool thisObs;
  int plotnr;
  int maxnr;
  bool fromFile;
  std::vector<int> list_plotnr;    // list of all stations, value = plotnr
  std::vector<int> nextplot;  // list of stations that will be plotted
  std::vector<int> notplot;   // list of stations that will be plottet as x
  std::vector<int> all_this_area; // all stations within area
  std::vector<int> all_stations; // all stations, from last plot or from file
  std::vector<int> all_from_file; // all stations, from file or priority list
  std::vector<int> stations_to_plot; // copy of list of stations that will be plotted
  //id of all stations shown, sorted by plot type
  static std::map<std::string, std::vector<std::string> > visibleStations;

  float areaFreeSpace, areaFreeWindSize;
  float areaFreeXsize, areaFreeYsize;
  float areaFreeXmove[4], areaFreeYmove[4];

  //Hqc
  std::string hqcFlag;  //which parameter is flagged
  bool flaginfo;
  Colour flagColour;
  std::map<std::string, Colour> paramColour;
  std::string selectedStation;
  std::string mark_parameter;

  void getObsLonLat(int obsidx, float& x, float& y);

  bool readTable(const std::string& type, const std::string& filename);
  void readPriorityFile(const std::string& filename);

  void decodeCriteria(std::string critStr);
  void decodeSort(std::string sortStr);

  bool getValueForCriteria(int index, const std::string& param, float& value);
  void adjustRRR(float& value);

  bool checkPlotCriteria(int index);
  void checkTotalColourCriteria(DiGLPainter* gl, int index);
  std::string checkMarkerCriteria(int index);
  void checkColourCriteria(DiGLPainter* gl, const std::string& param, float value);
  void parameterDecode(std::string, bool = true);

  // used many times from plotList and once from plotAscii
  void printUndef(DiGLPainter* gl, float&, float&, bool align_right = false);

  // used only from plotList
  void printList(DiGLPainter* gl, float f, float& xpos, float& ypos, int precision,
      bool align_right = false, std::string opt = "");
  void printListParameter(DiGLPainter* gl, const ObsData& dta, const Parameter& param,
      float& xpos, float& ypos, float yStep, bool align_right, float xshift);
  void printListSymbol(DiGLPainter* gl, const ObsData& dta, const Parameter& param,
      float& xpos, float& ypos, float yStep, bool align_right, const float& xshift);
  void printListRRR(DiGLPainter* gl, const ObsData& dta, const std::string& param, float& xpos,
      float& ypos, bool align_right);
  void printListPos(DiGLPainter* gl, const ObsData& dta,
      float& xpos, float& ypos, float yStep, bool align_right);

  void plotAscii(DiGLPainter* gl, const ObsData& dta,const std::string& param,
      float& xpos, float& ypos, const float& yStep, bool align_right);

  // used from plotSynop, plotMetar, metarWind, ROAD/plotDBMetar, ROAD/plotDBSynop
  void printNumber(DiGLPainter* gl, float, float, float,
      const std::string& align = "left", bool = false, bool = false);

  // from plotList, plotSynop
  void printTime(DiGLPainter* gl, const miutil::miTime&, float, float,
      bool align_right = false, const std::string& = "");

  // from plotList and plotAscii
  void printListString(DiGLPainter* gl, const std::string& txt,
      float& xpos, float& ypos, bool align_right = false);

  float advanceByStringWidth(DiGLPainter* gl, const std::string& txt, float& xpos);
  void advanceByDD(int dd, float& xpos);
  bool checkQuality(const ObsData& dta) const;
  bool checkWMOnumber(const ObsData& dta) const;

  // from plotList, plotSynop, plotMetar, metarWind, ROAD/plotDBMetar, ROAD/plotDBSynop
  void printString(DiGLPainter* gl, const char *, float, float,
      bool align_right = false, bool = false);

  // from plotMetar, ROAD/plotDBMetar (commented)
  void metarSymbol(DiGLPainter* gl, const std::string&, float, float, int&);

  // from plotMetar, metarSymbol, ROAD/plotDBMetar (commented)
  void metarString2int(std::string ww, int intww[]);

  // from plotMetar, ROAD/plotDBMetar
  void metarWind(DiGLPainter* gl, int, int, float &, int &);

  // from plotList, plotSynop, ROAD/plotDBSynop
  void arrow(DiGLPainter* gl, float angle, float xpos, float ypos, float scale = 1.);

  // from plotList, plotSynop, ROAD/plotDBSynop (commented out)
  void zigzagArrow(DiGLPainter* gl, float angle, float xpos, float ypos, float scale = 1.);

  // from plotList, plotSynop, plotMetar, metarSymbol, weather, pastWeather, ROAD/plotDBMetar, ROAD/plotDBSynop
  void symbol(DiGLPainter* gl, int, float, float, float scale = 1, bool align_right = false);

  // from plotSynop, metarWind, ROAD/plotDBSynop
  void cloudCover(DiGLPainter* gl, const float& fN, const float& radius);

  // from ROAD/plotDBSynop
  void cloudCoverAuto(DiGLPainter* gl, const float& fN, const float &radius);

  // from plotList, plotAscii, plotSynop, ROAD/plotDBSynop
  void plotWind(DiGLPainter* gl, int dd, float ff_ms, bool ddvar, float radius, float current =
      -1);

  // used from plotList, plotSynop, ROAD/plotDBSynop, ROAD/plotDBMetar (commented out)
  void weather(DiGLPainter* gl, short int ww, float TTT, int zone, float x, float y,
      float scale = 1, bool align_right = false);

  // used only from plotList, plotSynop, and ROAD/plotDBSynop
  void pastWeather(DiGLPainter* gl, int w, float x, float y, float scale = 1, bool align_right =
      false);

  // used only from plotList and plotSynop
  void wave(DiGLPainter* gl, const float& PwPw, const float& HwHw, float x, float y,
      bool align_right = false);

  // used only from plotList and plotSynop
  int visibility(float vv, bool ship);

  // used only from plotMetar (and commented out in ROAD/plotDBMetar)
  int vis_direction(float dv);

  // used only from plotSynop
  void amountOfClouds(DiGLPainter* gl, short int, short int, float, float);

  // ROAD only used in plotDBMetar, plotDBSynop
  void amountOfClouds_1(DiGLPainter* gl, short int Nh, short int h, float x, float y,
      bool metar = false);
  // ROAD only, used in plotDBMetar, plotDBSynop
  void amountOfClouds_1_4(DiGLPainter* gl, short int Ns1, short int hs1, short int Ns2,
      short int hs2, short int Ns3, short int hs3, short int Ns4, short int hs4,
      float x, float y, bool metar = false);

  void checkAccumulationTime(ObsData &dta); // used in ::plot when testpos == true (ie updating text/symbol layers)
  void checkGustTime(ObsData &dta);
  void checkMaxWindTime(ObsData &dta);

  void plotSynop(DiGLPainter* gl, int index);
  void plotList(DiGLPainter* gl, int index);
  void plotMetar(DiGLPainter* gl, int index);
#ifdef ROADOBS
  void plotRoadobs(DiGLPainter* gl, int index);
  void plotDBSynop(DiGLPainter* gl, int index);
  void plotDBMetar(DiGLPainter* gl, int index);
#endif
  void parameter_sort(const std::string& parameter, bool minValue);
  void priority_sort();
  void time_sort();

  int getObsCount() const;
  int numVisiblePositions(); // slow!

public:
  ObsPlot();
  ~ObsPlot();

  bool operator==(const ObsPlot &rhs) const;

  // clear info about text/symbol layers
  static void clearPos();
  // return the computed index in stationlist, ROADOBS only
  std::vector<int> & getStationsToPlot();
  // clear VisibleStations map from current plottype
  void clearVisibleStations();
  // Returns the allObs boolean
  bool isallObs() {return allObs;};

  void plot(DiGLPainter* gl, PlotOrder zorder);

  bool prepare(const std::string&);
  bool setData();
  void clear();
  void getObsAnnotation(std::string &, Colour &);
  bool getDataAnnotations(std::vector<std::string>& anno);

  void setObsAnnotation(const std::string &anno) // from ObsManager::prepare and ObsManager::sendHqcdata
    { annotation = anno; }

  void setPopupSpec(std::vector<std::string>& txt); // from ObsManager::prepare

  const std::vector<std::string> getObsExtraAnnotations() const; // from PlotModule

  bool updateObs(); // from PlotModule::updateObs
  void logStations(); // from PlotModule::prepareObs, PlotModule::obsTime, PlotModule::updatePlots
  void readStations(); // from PlotModule::obsTime

  void setLabel(const std::string& pin) // from PlotModule and ObsRoad
    { labels.push_back(pin); }

  bool getPositions(std::vector<float>&, std::vector<float>&);
  void obs_mslp(DiGLPainter* gl, PlotOrder porder, float *);

  bool getObsPopupText(int xx,int yy, std::string& obstext );

  // find observation near screen coordinates x, y; only of showpos
  int findObs(int x, int y, const std::string& type="");
  bool showpos_findObs(int x, int y);

  // find name of observation near screen coordinates x, y
  bool getObsName(int xx, int yy, std::string& station);

  // switch to next layer of symbols / text
  void nextObs(bool forward);

  const std::string& getInfoStr() const
    { return infostr; }

  bool mslp() const
    { return devfield; }

  bool moreTimes()
    { return moretimes; }

  const std::vector<std::string>& dataTypes() const // only from ObsManager::prepare
    { return datatypes; }

  void setObsTime(const miutil::miTime& t) // only from ObsManager::prepare
    { Time = t; }

  void setCurrent(float cur) // only from ObsManager::prepare
    { current = cur; }

  void setModificationTime(const std::string& fname); // only from ObsManager::prepare

  bool LevelAsField() const // from PlotModule
    { return levelAsField; }

  int getLevel()
    { return level; }

  void removeObs() // BUFR only
    { obsp.pop_back(); }

  void setDataType(std::string datatype)  // BUFR only
    { currentDatatype = datatype; }

  ObsData& getNextObs(); // BUFR only

  void resetObs(int num) // BUFR only, called from ObsManager
    { obsp.resize(num); }

  int numPositions() const //BUFR only, called from ObsManager before resetobs
    { return getObsCount(); }

  // Dialog info: Name, tooltip and type of parameter buttons. ASCII only
  std::vector<std::string> columnName;
  void addObsData(const std::vector<ObsData>& obs); // ASCII only (ObsAscii ctor)
  void replaceObsData(std::vector<ObsData>& obs); // ROADOBS only, replaces fake data with correct data.)
  const miutil::miTime& getObsTime() const // only from ObsRoad and ObsAscii
    { return Time; }

  void setLabels(const std::vector<std::string>& l) // ObsAscii only
    { labels = l; }

private:
  bool updateDeltaTimes();
  bool updateDeltaTime(ObsData &dta, const miutil::miTime& nowTime); // ASCII only
  ObsPlot(const ObsPlot &rhs);
  ObsPlot& operator=(const ObsPlot &rhs);

public:
  void addObsVector(const std::vector<ObsData>& vdata)  // HQC only (from ObsManager::sendHqcdata)
    { obsp = vdata; }

  void changeParamColour(const std::string& param, bool select); // HQC only

  bool flagInfo() const // HQC only
    { return flaginfo; }

  void setHqcFlag(const std::string& flag) // HQC only
    { hqcFlag = flag; }

  void setSelectedStation(const std::string& station) // HQC only
    { selectedStation = station; }

  /// copy some xpos and ypos from metaData map to obsp
  void mergeMetaData(const std::map<std::string, ObsData>& metaData);

  int getTimeDiff() const
    { return timeDiff; }

  void setTimeDiff(int diff)
    { timeDiff = diff; }

  // if timediff == -1 :use all observations
  // if not: use all observations with abs(obsTime-Time)<timediff
  bool timeOK(const miutil::miTime& t) const;

  // get pressure level etc from field (if needed)
  void updateLevel(const std::string& dataType);

  // Returns the file names containing observation data.
  const std::vector<std::string>& getFileNames() const;

  // observations from road
#ifdef ROADOBS
  bool preparePlot(void);
#endif
};

#endif
