/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diArea.h 17 2007-09-13 12:32:31Z lisbethb $

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
#ifndef diProfetObjectFactory_h
#define diProfetObjectFactory_h

#include <diField/diArea.h>
#include <profet/fetCodeExecutor.h>

#include <iostream>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

using namespace std;


/**
   
\brief Makes Profet objects from base objects, and updates them with values from gui

*/

class ProfetObjectFactory {

private:

#ifndef NOLOG4CXX
  log4cxx::LoggerPtr logger;
#endif

  Area fieldArea;           ///< field projection and covered area
  int nx,ny;                ///< field dimension
  fetCodeExecutor executor; ///< the object executor

  /// outputs responce strings from executor
  void outputExecuteResponce(vector<fetCodeExecutor::responce> & rl);


protected:
public:

  ProfetObjectFactory();
  
  /// initialise object factory to a field size and projection
  void initFactory(Area a, int size_x, int size_y);

  /// return gui-components from fetBaseObject
  vector<fetDynamicGui::GuiComponent> getGuiComponents(const fetBaseObject& baseobj);

   /// return gui-components from fetObject
  vector<fetDynamicGui::GuiComponent> getGuiComponents(const fetObject& fetobj);

  /// make fetObject from fetBaseObject, ProjectablePolygon and more
  fetObject makeObject(const fetBaseObject& baseObj,
		       ProjectablePolygon polygon,
		       const miString parameter,
		       const miTime   validtime,
		       const miString reason,
		       const miString user);

  /// update gui values in fetObject
  bool setGuiValues(fetObject& fetObj, 
		    const vector<fetDynamicGui::GuiComponent>& components);

  /// update polygon in fetObject 
  void setPolygon(fetObject& fetObj, ProjectablePolygon pp);
    
};

#endif
