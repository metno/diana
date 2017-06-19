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
#include "diPlotCommand.h"
#include "diGLPainter.h"
#include "diObsData.h"

#include <QPointF>

#include <set>
#include <vector>

struct ObsPositions;
class QTextCodec;

struct ObsPlotCollider {
  struct Box {
    float x1, x2, y1, y2;
    int index;
  };

  std::vector<Box> usedBox;
  std::vector<float> xUsed;
  std::vector<float> yUsed;

  void clear();

  bool positionFree(float, float, float, float);
  void positionPop();

  bool areaFree(const Box* b1, const Box* b2=0);
  void areaPop();

  bool collision(const Box& box) const;
};

/**
 \brief Plot observations

 - synop plot
 - metar plot
 - list plot
 */
class ObsPlot: public Plot {
public:
  enum ObsPlotType {
    OPT_SYNOP, OPT_METAR, OPT_ROADOBS, OPT_ASCII, OPT_LIST
  };

protected:
  std::vector<ObsData> obsp;

  // obs positions in getStaticPlot()->getMapArea().P() coordinates; updated in setData
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
  std::unique_ptr<ObsPositions> devfield;
  bool onlypos;
  bool showOnlyPrioritized;
  std::string image;
  int level;
  bool levelAsField;
  bool annotations;
  const ObsPlotType m_plottype;
  std::string dialogname;

  ObsPlotType plottype() const
    { return m_plottype; }

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
  PlotCommand_cpv labels; // labels from ascii-files or PlotModule(edit)
  std::map<std::string, std::vector<std::string> > popupSpec;
  float current; //cuurent, not wind
  bool firstplot;
  bool beendisabled; // obsplot was disabled while area changed

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

  struct markersizeCriteria: public baseCriteria {
    float size;
  };

  std::map<std::string, std::vector<plotCriteria> > plotcriteria;
  std::map<std::string, std::vector<colourCriteria> > colourcriteria;
  std::map<std::string, std::vector<colourCriteria> > totalcolourcriteria;
  std::map<std::string, std::vector<markerCriteria> > markercriteria;
  std::map<std::string, std::vector<markersizeCriteria> > markersizecriteria;

  std::map<std::string, bool> sortcriteria;
  //which parameters to plot
  std::map<std::string, bool> pFlag;

  ObsPlotCollider* collider_;

  void areaFreeSetup(float scale, float space, int num, float xdist, float ydist);
  bool areaFree(int idx);
  // ------------------------------------------------------------------------

  // static priority file
  static std::string currentPriorityFile;
  static std::vector<std::string> priorityList;

  // static synop and metar plot tables
  static std::vector<short> itabSynop;
  static std::vector<short> iptabSynop;
  static std::vector<short> itabMetar;
  static std::vector<short> iptabMetar;

  const std::vector<short>* itab;
  const std::vector<short>* iptab;

  float scale;

  void drawCircle(DiGLPainter* gl);

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

  QTextCodec* mTextCodec;

  QString decodeText(const std::string& text) const;

  void getObsLonLat(int obsidx, float& x, float& y);

  int vtab(int idx) const;
  QPointF xytab(int idxy) const;
  QPointF xytab(int idx, int idy) const;
  static bool readTable(const ObsPlotType type, const std::string& itab_filename, const std::string& iptab_filename,
      std::vector<short>& ritab, std::vector<short>& riptab);
  void readPriorityFile(const std::string& filename);

  void decodeCriteria(const std::string& critStr);
  void decodeSort(const std::string& sortStr);

  bool getValueForCriteria(int index, const std::string& param, float& value);
  void adjustRRR(float& value);

  bool checkPlotCriteria(int index);
  void checkTotalColourCriteria(DiGLPainter* gl, int index);
  std::string checkMarkerCriteria(int index);
  float checkMarkersizeCriteria(int index);
  void checkColourCriteria(DiGLPainter* gl, const std::string& param, float value);
  void parameterDecode(const std::string&, bool = true);

  // used many times from plotList and once from plotAscii
  void printUndef(DiGLPainter* gl, QPointF&, bool align_right = false);

  // used only from plotList
  void printList(DiGLPainter* gl, float f, QPointF& xypos, int precision,
      bool align_right = false, bool fill_2 = false);
  void printListParameter(DiGLPainter* gl, const ObsData& dta, const Parameter& param,
      QPointF& xypos, float yStep, bool align_right, float xshift);
  void printListSymbol(DiGLPainter* gl, const ObsData& dta, const Parameter& param,
      QPointF& xypos, float yStep, bool align_right, const float& xshift);
  void printListRRR(DiGLPainter* gl, const ObsData& dta, const std::string& param,
      QPointF& xypos, bool align_right);
  void printListPos(DiGLPainter* gl, const ObsData& dta,
      QPointF& xypos, float yStep, bool align_right);

  void plotAscii(DiGLPainter* gl, const ObsData& dta,const std::string& param,
      QPointF& xypos, const float& yStep, bool align_right);

  // used from plotSynop, plotMetar, metarWind, ROAD/plotDBMetar, ROAD/plotDBSynop
  void printNumber(DiGLPainter* gl, float, QPointF xypos,
      const std::string& align = "left", bool = false, bool = false);

  // from plotList, plotSynop
  void printTime(DiGLPainter* gl, const miutil::miTime&, QPointF,
      bool align_right = false, const std::string& = "");

  // from plotList and plotAscii
  void printListString(DiGLPainter* gl, const QString& txt,
      QPointF& xypos, bool align_right = false);

  float advanceByStringWidth(DiGLPainter* gl, const QString& txt, QPointF& xypos);
  void advanceByDD(int dd, QPointF& xypos);
  bool checkQuality(const ObsData& dta) const;
  bool checkWMOnumber(const ObsData& dta) const;

  // from plotList, plotSynop, plotMetar, metarWind, ROAD/plotDBMetar, ROAD/plotDBSynop
  void printString(DiGLPainter* gl, const QString&, QPointF xypos,
      bool align_right = false, bool = false);

  // from plotMetar, ROAD/plotDBMetar (commented)
  void metarSymbol(DiGLPainter* gl, const std::string&, QPointF, int&);

  // from plotMetar, metarSymbol, ROAD/plotDBMetar (commented)
  void metarString2int(std::string ww, int intww[]);

  // from plotMetar, ROAD/plotDBMetar
  void metarWind(DiGLPainter* gl, int, int, float &, int &);

  // from plotList, plotSynop, ROAD/plotDBSynop
  void arrow(DiGLPainter* gl, float angle, QPointF xypos, float scale = 1.);

  // from plotList, plotSynop, ROAD/plotDBSynop (commented out)
  void zigzagArrow(DiGLPainter* gl, float angle, QPointF xypos, float scale = 1.);

  // from plotList, plotSynop, plotMetar, metarSymbol, weather, pastWeather, ROAD/plotDBMetar, ROAD/plotDBSynop
  void symbol(DiGLPainter* gl, int, QPointF, float scale = 1, bool align_right = false);

  // from plotSynop, metarWind, ROAD/plotDBSynop
  void cloudCover(DiGLPainter* gl, const float& fN, const float& radius);

  // from ROAD/plotDBSynop
  void cloudCoverAuto(DiGLPainter* gl, const float& fN, const float &radius);

  // from plotList, plotAscii, plotSynop, ROAD/plotDBSynop
  void plotWind(DiGLPainter* gl, int dd, float ff_ms, bool ddvar, float radius, float current=-1);

  // used from plotList, plotSynop, ROAD/plotDBSynop, ROAD/plotDBMetar (commented out)
  virtual void weather(DiGLPainter* gl, short int ww, float TTT, bool show_time_id, QPointF xy,
      float scale = 1, bool align_right = false);

  // used only from plotList, plotSynop, and ROAD/plotDBSynop
  void pastWeather(DiGLPainter* gl, int w, QPointF xy, float scale = 1, bool align_right=false);

  // used only from plotList and plotSynop
  void wave(DiGLPainter* gl, const float& PwPw, const float& HwHw, QPointF xy, bool align_right = false);

  // used only from plotList and plotSynop
  int visibility(float vv, bool ship);

  // used only from plotMetar (and commented out in ROAD/plotDBMetar)
  int vis_direction(float dv);

  // used only from plotSynop
  void amountOfClouds(DiGLPainter* gl, short int, short int, QPointF xypos);

  // ROAD only used in plotDBMetar, plotDBSynop
  void amountOfClouds_1(DiGLPainter* gl, short int Nh, short int h, QPointF xy,
      bool metar = false);
  // ROAD only, used in plotDBMetar, plotDBSynop
  void amountOfClouds_1_4(DiGLPainter* gl, short int Ns1, short int hs1, short int Ns2,
      short int hs2, short int Ns3, short int hs3, short int Ns4, short int hs4,
      QPointF xy, bool metar = false);

  void checkAccumulationTime(ObsData &dta); // used in ::plot when testpos == true (ie updating text/symbol layers)
  void checkGustTime(ObsData &dta);
  void checkMaxWindTime(ObsData &dta);

  virtual void plotIndex(DiGLPainter* gl, int index);
  void plotSynop(DiGLPainter* gl, int index);
  void plotList(DiGLPainter* gl, int index);
  void plotMetar(DiGLPainter* gl, int index);
  void parameter_sort(const std::string& parameter, bool minValue);
  void priority_sort();
  void time_sort();

  int getObsCount() const;
  int numVisiblePositions() const; // slow!

protected:
  ObsPlot(const miutil::KeyValue_v &pin, ObsPlotType plottype);

public:
  ~ObsPlot();

  static ObsPlot* createObsPlot(const PlotCommand_cp& pin);

  bool operator==(const ObsPlot &rhs) const;

  void setCollider(ObsPlotCollider* collider)
    { collider_ = collider; }

  // return the computed index in stationlist, ROADOBS only
  std::vector<int> & getStationsToPlot();
  // clear VisibleStations map from current plottype
  void clearVisibleStations();
  // Returns the allObs boolean
  bool isallObs() {return allObs;};

  void setTextCodec(QTextCodec* codec);
  void setTextCodec(const char* codecName);

  void plot(DiGLPainter* gl, PlotOrder zorder);

  bool setData();
  void clear();
  void getAnnotation(std::string &, Colour &) const;
  bool getDataAnnotations(std::vector<std::string>& anno);

  void setObsAnnotation(const std::string &anno) // from ObsManager::prepare and ObsManager::sendHqcdata
    { annotation = anno; }

  void setPopupSpec(std::vector<std::string>& txt); // from ObsManager::prepare

  const PlotCommand_cpv getObsExtraAnnotations() const; // from PlotModule

  // @return true iff update is needed (one or more files are changed)
  bool updateObs(); // from PlotModule::updateObs

  void logStations(); // from PlotModule::prepareObs, PlotModule::obsTime, PlotModule::updatePlots
  void readStations(); // from PlotModule::obsTime

  void setLabel(const PlotCommand_cp& pin) // from PlotModule and ObsRoad
    { labels.push_back(pin); }

  void updateObsPositions();
  ObsPositions* getObsPositions()
    { return devfield.get(); }
  void updateFromEditField();

  bool getObsPopupText(int xx,int yy, std::string& obstext );

  // find observation near screen coordinates x, y; only of showpos
  int findObs(int x, int y, const std::string& type="");
  bool showpos_findObs(int x, int y);

  // find name of observation near screen coordinates x, y
  bool getObsName(int xx, int yy, std::string& station);

  // switch to next layer of symbols / text
  void nextObs(bool forward);

  bool mslp() const
    { return devfield.get() != 0; }

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

  void setLabels(const PlotCommand_cpv& l) // ObsAscii only
    { labels = l; }

protected:
  virtual long findModificationTime(const std::string& fname);
  virtual bool isFileUpdated(const std::string& fname, long now, long mod_time);
  int calcNum() const;
  bool isSynopListRoad() const;
  bool isSynopMetarRoad() const;
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
};

#endif
