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

const miString OBJECTS_ANALYSIS= "Analyse";
const miString OBJECTS_SIGMAPS=  "Sigkart";


  /**
     \brief Database info
  */
struct editDBinfo {
  miString host;
  miString user;
  miString pass;
  miString base;
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
  miString productName;
  miString pid;      ///< productid    ("VA")
  miTime ptime;      ///< product time
  dataSource source; ///< source
  int  element;      ///< -1=objects, 0,1,...=field number
  miString filename; ///< full filename
  miString selectObjectTypes;   ///< fronts,symbols,areas to read
};


  /**
     \brief Product id
  */
struct EditProductId {
  miString name;                    ///< VA,VNN etc..
  bool sendable;                    ///< 'send' enabled
  bool combinable;                  ///< may be used to combine ids
  vector<miString> combineids;
};

  /**
     \brief Edit Product Field defined in setup
  */
struct EditProductField {
  // gui <--> controller
  miString name;                    ///< field-name
  // gui --> controller
  bool fromfield;                   ///< make from field
  miString fromfname;               ///< from-field: model,name,..
  savedProduct fromprod;            ///< from-product
  // only used by EditManager
  miString filenamePart;            ///< "ANAmslp"
  int vcoord,param,level,level2;    ///< fieldfile identification
  float minValue,maxValue;          ///< check min,max value if not fieldUndef
  vector<miString> editTools;       ///< standard/classes/numbers
};


  /**
     \brief Edit Product defined in setup
  */
struct EditProduct {
  // gui <--> controller
  miString name;                    ///< productname
  miString db_name;                 ///< productname in database
  vector<miString> drawtools;       ///< tools to use
  vector<EditProductId> pids;       ///< legal product-id's
  vector <savedProduct> objectprods;///< products to fetch objects from
  vector<EditProductField> fields;  ///< required fields
  editDBinfo dbi;                   ///< Database info
  // only used by EditManager
  miString savedir;                 ///< directory for saved product
  vector<miString> inputdirs;       ///< savedir is always the first ???
  vector<miString> inputproducts;   ///< products for input objects/fields
  vector<miString> combinedirs;     ///< directory for combined product
  miString combineBorders;          ///< "ANAborders."  (ANAborders.DNMI etc.)
  miString objectsFilenamePart;     ///< "ANAdraw"
  miString commentFilenamePart;     ///< "ANAcomm"
  vector <miString> labels;         ///< annotations
  vector <miString> OKstrings;      ///< define map background and area and other OKStrings
  miString commandFilename;         ///< file to read okstrings...
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
