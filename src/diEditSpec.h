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
#ifndef _diEditSpec_h
#define _diEditSpec_h

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diField/diArea.h>
#include <vector>
#include <set>

const miutil::miString OBJECTS_ANALYSIS= "Analyse";
const miutil::miString OBJECTS_SIGMAPS=  "Sigkart";


  /**
     \brief Database info
  */
struct editDBinfo {
  miutil::miString host;
  miutil::miString user;
  miutil::miString pass;
  miutil::miString base;
  unsigned int port;
  bool loggedin;
};


  /**
     \brief saved product source
  */
enum dataSource {
  data_local,
  data_server,
  data_db
};

  /**
     \brief saved "products" (product elements)
  */
struct savedProduct {
  miutil::miString productName;
  miutil::miString pid;      ///< productid    ("VA")
  miutil::miTime ptime;      ///< product time
  dataSource source; ///< source
  int  element;      ///< -1=objects, 0,1,...=field number
  miutil::miString filename; ///< full filename
  miutil::miString selectObjectTypes;   ///< fronts,symbols,areas to read
};


  /**
     \brief Product id
  */
struct EditProductId {
  miutil::miString name;                    ///< VA,VNN etc..
  bool sendable;                    ///< 'send' enabled
  bool combinable;                  ///< may be used to combine ids
  vector<miutil::miString> combineids;
};

  /**
     \brief Edit Product Field defined in setup
  */
struct EditProductField {
  // gui <--> controller
  miutil::miString name;                    ///< field-name
  // gui --> controller
  bool fromfield;                   ///< make from field
  miutil::miString fromfname;               ///< from-field: model,name,..
  savedProduct fromprod;            ///< from-product
  // only used by EditManager
  miutil::miString filenamePart;            ///< "ANAmslp"
  int vcoord,param,level,level2;    ///< fieldfile identification
  float minValue,maxValue;          ///< check min,max value if not fieldUndef
  vector<miutil::miString> editTools;       ///< standard/classes/numbers
};


  /**
     \brief Edit Product defined in setup
  */
struct EditProduct {
  // gui <--> controller
  miutil::miString name;                    ///< productname
  miutil::miString db_name;                 ///< productname in database
  vector<miutil::miString> drawtools;       ///< tools to use
  vector<EditProductId> pids;       ///< legal product-id's
  vector <savedProduct> objectprods;///< products to fetch objects from
  vector<EditProductField> fields;  ///< required fields
  editDBinfo dbi;                   ///< Database info
  // only used by EditManager
  miutil::miString savedir;                 ///< directory for saved product
  vector<miutil::miString> inputdirs;       ///< savedir is always the first ???
  vector<miutil::miString> inputproducts;   ///< products for input objects/fields
  vector<miutil::miString> combinedirs;     ///< directory for combined product
  miutil::miString combineBorders;          ///< "ANAborders."  (ANAborders.DNMI etc.)
  miutil::miString objectsFilenamePart;     ///< "ANAdraw"
  miutil::miString commentFilenamePart;     ///< "ANAcomm"
  vector <miutil::miString> labels;         ///< annotations
  vector <miutil::miString> OKstrings;      ///< define map background and area and other OKStrings
  miutil::miString commandFilename;         ///< file to read okstrings...
  int   producer, gridnum;          ///< common field idents
  Area  area;                       ///< area/projection if gridnum>0 !
  bool  areaminimize;               ///< minimize area due to undef. values
  int standardSymbolSize;           ///< default symbol size for this product
  int complexSymbolSize;            ///< default complex symbol size
  int frontLineWidth;               ///< default front line width
  int areaLineWidth;                ///< default front line width
  bool  startEarly;                 ///< earliest start time set, or not
  bool  startLate;                  ///< latest   start time set, or not
  int   minutesStartEarly;          ///< earliest start time offset (+/-minutes)
  int   minutesStartLate;           ///< latest   start time offset (+/-minutes)
};

#endif
