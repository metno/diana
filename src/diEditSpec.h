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
#ifndef _diEditSpec_h
#define _diEditSpec_h

#include <puTools/miTime.h>
#include <diField/diArea.h>
#include <vector>
#include <set>

const std::string OBJECTS_ANALYSIS= "Analyse";
const std::string OBJECTS_SIGMAPS=  "Sigkart";



  /**
     \brief saved product source
  */
enum dataSource {
  data_local,
  data_server
};

  /**
     \brief saved "products" (product elements)
  */
struct savedProduct {
  std::string productName;
  std::string pid;      ///< productid    ("VA")
  miutil::miTime ptime;      ///< product time
  dataSource source; ///< source
  int  element;      ///< -1=objects, 0,1,...=field number
  std::string filename; ///< full filename
  std::string selectObjectTypes;   ///< fronts,symbols,areas to read
};


  /**
     \brief Product id
  */
struct EditProductId {
  std::string name;                    ///< VA,VNN etc..
  bool sendable;                    ///< 'send' enabled
  bool combinable;                  ///< may be used to combine ids
  std::vector<std::string> combineids;
};

  /**
     \brief Edit Product Field defined in setup
  */
struct EditProductField {
  // gui <--> controller
  std::string name;                    ///< field-name
  std::string filename;
  std::string localFilename;
  std::string prodFilename;
  // gui --> controller
  bool fromfield;                   ///< make from field
  std::string fromfname;               ///< from-field: model,name,..
  savedProduct fromprod;            ///< from-product
  // only used by EditManager
  std::string filenamePart;            ///< "ANAmslp"
  float minValue,maxValue;          ///< check min,max value if not fieldUndef
  std::vector<std::string> editTools;       ///< standard/classes/numbers
  std::string vcoord_cdm;       ///< vertical coordinat -  cdm syntax
  std::string vlevel_cdm;       ///< vertical level - cdm syntax
  std::string unit_cdm;             ///< parameter unit (hectopascal)
};


  /**
     \brief Edit Product defined in setup
  */
struct EditProduct {
  // gui <--> controller
  std::string name;                    ///< productname
  std::vector<std::string> drawtools;       ///< tools to use
  std::vector<EditProductId> pids;       ///< legal product-id's
  std::vector <savedProduct> objectprods;///< products to fetch objects from
  std::vector<EditProductField> fields;  ///< required fields
  std::string local_savedir;                 ///< directory for saved product
  std::string prod_savedir;                 ///< directory for saved product
  std::vector<std::string> inputdirs;       ///< savedir is always the first ???
  std::string inputFieldFormat;        ///< inputFieldFormat netcdf,felt,wdb etc
  std::string inputFieldConfig;        ///< fimex xml-config
  std::vector<std::string> combinedirs;     ///< directory for combined product
  std::string combineBorders;          ///< "ANAborders."  (ANAborders.DNMI etc.)
  std::string objectsFilenamePart;     ///< "ANAdraw"
  std::string commentFilenamePart;     ///< "ANAcomm"
  std::vector <std::string> labels;         ///< annotations
  std::vector <std::string> OKstrings;      ///< define map background and area and other OKStrings
  std::string commandFilename;         ///< file to read okstrings...
  bool  areaminimize;               ///< minimize area due to undef. values
  int standardSymbolSize;           ///< default symbol size for this product
  int complexSymbolSize;            ///< default complex symbol size
  int frontLineWidth;               ///< default front line width
  int areaLineWidth;                ///< default front line width
  int winX;                         ///< main window width, adjustment to the map area
  int winY;                         ///< main window height, adjustment to the map area
  bool  startEarly;                 ///< earliest start time set, or not
  bool  startLate;                  ///< latest   start time set, or not
  int   minutesStartEarly;          ///< earliest start time offset (+/-minutes)
  int   minutesStartLate;           ///< latest   start time offset (+/-minutes)
  std::string templateFilename;
  int autoremove;
};

#endif
