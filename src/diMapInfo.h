#ifndef DIMAPINFO_H
#define DIMAPINFO_H

#include <string>
#include <vector>

/**
   \brief GUI data for one map file
*/
struct MapFileInfo {
  std::string fname;     ///< filename
  double sizelimit; ///< viewsize limit in km's
};

/**
   \brief GUI data for one map element
*/
struct MapElementOption {
  bool ison;              ///< element is on
  std::string linecolour; ///< line color
  std::string fillcolour; ///< fill color
  std::string linetype;   ///< line type
  std::string linewidth;  ///< line width
  int zorder;             ///< z-position on map
  float density;          ///< density in degrees (latlon)
  bool showvalue;         ///< plot value string (latlon)
  std::string value_pos;  ///< value position (0=left, 1=bottom, 2=both) (latlon)
  float fontsize;         ///< fontsize for value plotting (latlon)

  MapElementOption() : ison(false), zorder(1), density(10), showvalue(false), fontsize(10) {}
};

/**
   \brief GUI data for one map
*/
struct MapInfo {
  std::string name;          ///< name of map
  std::string type;          ///< type of mapsource(s)
  bool logok;                ///< ok to log
  std::vector<MapFileInfo> mapfiles; ///< the file(s)
  MapElementOption contour;  ///< contourline options
  MapElementOption land;     ///< filled land options
  MapElementOption lon;      ///< lon-lines options
  MapElementOption lat;      ///< lat-lines options
  MapElementOption frame;    ///< area-frame options
  bool special;              ///< plot a symbol instead of a point (special case)
  int symbol;                ///< symbol number in ttf file (special case)
  std::string dbfcol;        ///< column name in dbf file, values to be plottet
                             ///< (special case)

  void reset();
};

/**
   \brief GUI data for all maps
*/
struct MapDialogInfo {
  std::vector<std::string> areas; ///< all defined areas (names)
  std::string default_area;  ///< default area-name
  std::vector<MapInfo> maps;   ///< all defined maps
  std::vector<std::string> default_maps; ///< list of default maps (names)
  std::string backcolour;    ///< background colour
};

#endif // DIMAPINFO_H
