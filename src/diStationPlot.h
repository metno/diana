/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef StationPlot_h
#define StationPlot_h

#include "diStationInfo.h"
#include "diColour.h"
#include "diImageGallery.h"
#include "diPlot.h"
#include "diPlotOptions.h"

#include <puDatatypes/miCoordinates.h>
#include <diField/diArea.h>

#include <vector>
#include <deque>

/**
   \brief text and alignment for one stationPlot point
*/
struct stationText{
  std::string text;
  Alignment hAlign;
};

  /**
     \brief properties for each station held in a station plot object

     This defines coordinates, image, text to be shown as a label, and other properties.
  */
struct Station {
  enum Status
  {
    noStatus = 0,
    failed = 1,
    underRepair = 2,
    working = 3,
    unknown = 4
  };

  enum Type
  {
    automatic,
    visual
  };

  std::string name;
  miCoordinates pos;
  std::string image;
  std::string image2;
  bool isVisible;
  bool isSelected;
  int dd,ff,north;
  int alpha;
  float scale;
  Colour colour;
  int height; /**< station height */
  int barHeight; /**< barometer height */
  std::string id; /**< WMO or climate number */
  std::vector <stationText> vsText;
  std::string url;
  Status status;
  Type type;
  miutil::miTime time;

  Station() {
    scale = 1; alpha=255;
  }

  float lon() const
    { return pos.dLon(); }
  float lat() const
    { return pos.dLat(); }
};

class StationArea {
public:
  StationArea(float minLat, float maxLat, float minLon, float maxLon);

  std::vector<Station*> findStations(const miCoordinates& pos, float radius) const;
  std::vector<Station*> findStations(float lat, float lon, float radius) const
    { return findStations(miCoordinates(lon, lat), radius); }

  Station* findStation(const miCoordinates& pos, float radius) const;
  Station* findStation(float lat, float lon, float radius) const
    { return findStation(miCoordinates(lon, lat), radius); }

  void addStation(Station* station);

  bool contains(const miCoordinates& pos) const
    { return contains(pos.dLat(), pos.dLon()); }
  bool contains(float lat, float lon) const
    { return lat >= minLat && lat < maxLat && lon >= minLon && lon < maxLon; }

  /*! Approximate distance from pos to area rectangle in km
   * The returned value is not correct for large distances.
   */
  double distance(const miCoordinates& pos) const;

private:
  float minLat;
  float maxLat;
  float minLon;
  float maxLon;
  std::vector<StationArea> areas;  // subareas of this area
  std::vector<Station*> stations;  // pointers to stations are owned by the StationPlot
};

/**
   \brief Plot pickable stations/positions on the map

   Contains data for a set of stations.
   Stations can be displayed by an image or a simple drawing
   with or without name/text. Misc options to set/change "state"
   of each position in the set.
*/
class StationPlot: public Plot{

public:
  StationPlot(){}
  /// constructor with station longitudes and latitudes
  StationPlot(const std::vector <float> & lons, const std::vector <float> & lats);
  /// constructor with station names,longitudes and latitudes
  StationPlot(const std::vector <std::string> & names,const std::vector <float> & lons,
      const std::vector <float> & lats);
  StationPlot(const std::vector <stationInfo> & stations);
  /// constructor with station names,longitudes,latitudes and images
  StationPlot(const std::vector <std::string> & names,const std::vector <float> & lons,
        const std::vector <float> & lats, const std::vector <std::string>& images);
  /// constructor with stations
  StationPlot(const std::vector <Station*> &stations);
  StationPlot(const std::string& commondesc,
      const std::string& common,
      const std::string& description,
      int from,
      const  std::vector<std::string>& data);
  ~StationPlot();


  /// plot stations
  void plot(DiGLPainter* gl, PlotOrder zorder);

  /// hide stations
  void hide();
  // show stations
  void show();
  /// unselects all stations
  void unselect();
  /// returns true if stationplot visible
  bool isVisible();
  /// change stationplot projection
  bool changeProjection();

  /// Returns the stations in the plot object
  std::vector<Station*> getStations() const;

  /// Returns the station at phys position x and y
  Station* stationAt(int phys_x, int phys_y);

  /// Returns all stations at phys position x and y
  std::vector<Station*> stationsAt(int x, int y, float radius=100.0, bool useAllStation=false);

  /// Returns a std::vector containing the names of the stations at position x and y
  std::vector<std::string> findStation(int x, int y, bool add=false);

  std::vector<std::string> findStations(int x, int y);

  /// Returns the selected stations in the plot object
  std::vector<Station*> getSelectedStations() const;
  /// set stations with name station to selected<br>
  void setSelectedStations(const std::vector<std::string>& station);
  /// set station with name station to selected<br> if add is false, unselect all stations first
  int setSelectedStation(std::string station, bool add=false);
  /// set station number i to selected<br> if add is false, unselect all stations first
  int setSelectedStation(int i, bool add=false);
  /// get annotation
  void getAnnotation(std::string &str,Colour &col) const;
  /// set annotation
  void setStationPlotAnnotation(const std::string &str);
  /// set UseImage
  void setUseImage(bool _useImage) { useImage = _useImage;}
  /// set name/plotname
  void setName(const std::string& nm);
  /// return name
  const std::string& getName() const
    { return name; }
  /// set id to i
  void setId(int i)
    { id = i; }
  /// return id
  int getId() const
    { return id; }
  /**
   * Get image scale for a station
   * @param i Index of station
   * @return Current image scale for station i
   */
  float getImageScale(int i);
  /// set normal and selected image to im1
  void setImage(std::string im1);
  /// set normal image to im1, selected image to im2
  void setImage(std::string im1,std::string im2);
  /// set new scale for all images
  void setImageScale(float new_scale);
  /// clears all text
  void clearText();
  /// if normal=true write name on all plotted stations, if selected=true write name on all selected stations
  void setUseStationName(bool normal, bool selected);
  void setIcon(const std::string& icon)
    { iconName = icon; }
  const std::string& getIcon() const
    { return iconName; }
  bool stationCommand(const std::string& Command,
      const std::vector<std::string>& data,
      const std::string& misc="");
  bool stationCommand(const std::string& Command);
  std::vector<std::string> stationRequest(const std::string& Command);

  friend bool operator==(const StationPlot& lhs, const StationPlot& rhs)
    { return (lhs.stations.size()==rhs.stations.size()) ; }

  friend bool operator>(const StationPlot& lhs, const StationPlot& rhs)
    { return (lhs.stations.size()>rhs.stations.size()); }

  friend bool operator<(const StationPlot& lhs, const StationPlot& rhs)
    { return (lhs.stations.size()<rhs.stations.size()); }

private:
  std::vector<Station*> stations; //stations, name, lon, lat etc...
  std::vector<StationArea> stationAreas;  // areas containing stations

  //  void addStation(const std::string names);
  void addStation(float lon, float lat, const std::string& name="",
      const std::string& image="", int alpha=255, float scale=1.0);
  void addStation(Station* station);
  void defineCoordinates();
  void init();
  void plotStation(DiGLPainter* gl, int i);
  void glPlot(DiGLPainter* gl, Station::Status tp, float x, float y, bool selected = false);
  void plotWind(DiGLPainter* gl, int i, float x, float y, bool classic=false, float scale=1);

  std::vector <float> xplot; //x-positions to plot in current projection
  std::vector <float> yplot; //y-positions to plot in current projection

  bool visible;
  std::string annotation;
  std::string name; //f.ex. "vprof"
  int id;
  bool useImage;
  bool useStationNameNormal,useStationNameSelected;
  bool showText;
  int textSize;
  Colour textColour;
  std::string textStyle;
  std::string imageNormal,imageSelected;
  std::string iconName;
  int index; //last selected

  ImageGallery ig;
};

#endif
