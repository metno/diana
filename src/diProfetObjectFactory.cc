/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diProfetObjectFactory.cc 17 2007-09-13 12:32:31Z lisbethb $

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

#include <diProfetObjectFactory.h>


ProfetObjectFactory::ProfetObjectFactory()
  : nx(0), ny(0)
{
#ifndef NOLOG4CXX
  logger = log4cxx::Logger::getLogger("diana.ProfetObjectFactory");
#endif 
}


void ProfetObjectFactory::outputExecuteResponce( vector<fetCodeExecutor::responce> & rl)
{
  ostringstream ost;
  for ( int i=0; i<rl.size(); i++ ){
    if ( rl[i].level != fetCodeExecutor::INFO )
      ost << ( rl[i].level == fetCodeExecutor::ERROR  
	       ? "ERROR:" : "WARNING:") 
	  << rl[i].message << ", line " << rl[i].linenum
	  << " in finished object-code" << endl;
  }
  LOG4CXX_ERROR(logger, ost.str() );
}


void ProfetObjectFactory::initFactory(Area a, int size_x, int size_y)
{
  fieldArea = a;
  nx= size_x;
  ny= size_y;
}


vector<fetDynamicGui::GuiComponent>
ProfetObjectFactory::getGuiComponents(const fetBaseObject& baseobj)

{
  LOG4CXX_INFO(logger,"getGuiComponents(fetBaseObject)");

  vector<fetCodeExecutor::responce> responcel; 
  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys;
  bool onlygui=true;

  // compile and fetch gui components from code
  bool ok = executor.compile( baseobj, responcel, components, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }

  return components;
}

vector<fetDynamicGui::GuiComponent>
ProfetObjectFactory::getGuiComponents(const fetObject& fetobj) 
{
  LOG4CXX_INFO(logger,"getGuiComponents(fetObject) for id="<< fetobj.id());

  fetBaseObject baseobject = fetobj.baseObject();
  vector<fetCodeExecutor::responce> responcel; 
  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys = fetobj.guiElements();
  bool onlygui = true;//false;
  
  // compile and fetch gui components from code
  bool ok = executor.compile( baseobject, responcel, components, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }

//   fetObj.setAlgorithm(executor.cleanCode());

//   setGuiValues(fetObj, components);  

  return components;
}


/**
   ===================================================
   Make a new object from baseobject, polygon and more..
   ===================================================
*/
fetObject
ProfetObjectFactory::makeObject(const fetBaseObject& baseObj,
				ProjectablePolygon polygon,
				const miString parameter,
				const miTime   validtime,
				const miString reason,
				const miString user)
{
  LOG4CXX_INFO(logger,"makeObject(fetBaseObject)");

  fetObject fetObj;
  if ( nx==0 || ny==0 || fieldArea.R().width()==0 ){
    LOG4CXX_ERROR(logger,"Field-projection and size not set - undefined object!");
    return fetObj;
  }
  
  vector<fetCodeExecutor::responce> responcel; 
  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys;
  bool onlygui=false;
  miString id;
  miTime edittime = miTime::nowTime();

  // compile and fetch gui components from code
  bool ok = executor.compile( baseObj, responcel, components, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }

  // make object
  fetObj.setFromBaseObject(baseObj,
			   reason,polygon,nx,ny,fieldArea,
			   executor.cleanCode(),executor.cleanCode(),user,
			   validtime,edittime,parameter,guikeys,id);

  setGuiValues(fetObj,components);

  return fetObj;

}



/**
   =================================================
   set Gui components in fetObject code 
   =================================================
*/
bool ProfetObjectFactory::setGuiValues(fetObject& fetObj, 
				      const vector<fetDynamicGui::GuiComponent>& components )
{

  LOG4CXX_INFO(logger,"setGuiValues");

  return executor.prepareCode(fetObj,components);

}

void ProfetObjectFactory::setPolygon(fetObject& fetObj, 
				     ProjectablePolygon pp)
{
  
  fetObj.setPolygon(pp);
  
}






