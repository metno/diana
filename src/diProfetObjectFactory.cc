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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diProfetObjectFactory.h>

using namespace::miutil;

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
  for ( unsigned int i=0; i<rl.size(); i++ ){
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
  LOG4CXX_DEBUG(logger,"getGuiComponents(fetBaseObject)");

  fetCodeExecutor executor; ///< the object executor
  vector<fetCodeExecutor::responce> responcel;
  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys;
  bool onlygui=true;

  // compile and fetch gui components from code
  bool ok = executor.compile( baseobj, responcel, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }

  return executor.getGuiComponents();
}


vector<fetDynamicGui::GuiComponent>
ProfetObjectFactory::getGuiComponents(const fetObject& fetobj)
{
  LOG4CXX_DEBUG(logger,"getGuiComponents(fetObject) for id="<< fetobj.id());

  fetCodeExecutor executor; ///< the object executor
  fetBaseObject baseobject = fetobj.baseObject();
  vector<fetCodeExecutor::responce> responcel;
  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys = fetobj.guiElements();
  bool onlygui = true;

  // compile and fetch gui components from code
  bool ok = executor.compile( baseobject, responcel, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }

  return executor.getGuiComponents();
}


/**
   ===================================================
   Make a new object from baseobject, polygon and more..
   ===================================================
*/
fetObject
ProfetObjectFactory::makeObject( const fetBaseObject& baseObj,
				 ProjectablePolygon polygon,
				 const miString parameter,
				 const miTime   validtime,
				 const miString reason,
				 const miString user,
				 const miTime   sessionreftime,
				 const miString parent )
{
  LOG4CXX_DEBUG(logger,"makeObject(fetBaseObject)");

  fetObject fetObj;
  if ( nx==0 || ny==0 || fieldArea.R().width()==0 ){
    LOG4CXX_ERROR(logger,"Field-projection and size not set - undefined object!");
    return fetObj;
  }

  fetCodeExecutor executor; ///< the object executor
  vector<fetCodeExecutor::responce> responcel;
  map<miString,miString> guikeys;
  bool onlygui=false;
  miString id;
  miTime edittime = miTime::nowTime();

  // compile and fetch gui components from code
  bool ok = executor.compile( baseObj, responcel, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }
  vector<fetDynamicGui::GuiComponent> components = executor.getGuiComponents();

  // make object
  fetObj.setFromBaseObject(baseObj,
			   sessionreftime,parent,reason,polygon,nx,ny,fieldArea,
			   executor.cleanCode(),executor.cleanCode(),user,
			   validtime,edittime,parameter,guikeys,id);

  // set local values
  fetObj.setTimeVariables(executor.timevariables());
  fetObj.setValuesForZeroImpact(executor.valuesforzeroimpact());
  fetObj.setGuiComponents(components);

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

  LOG4CXX_DEBUG(logger,"setGuiValues");

  fetCodeExecutor executor; ///< the object executor
  return executor.prepareCode(fetObj,components);

}

void ProfetObjectFactory::setPolygon(fetObject& fetObj,
				     ProjectablePolygon pp)
{

  fetObj.setPolygon(pp);

}

// process the changes from a time edit..
bool ProfetObjectFactory::processTimeValuesOnObject(fetObject& fetObj)
{

  // set elements in fetObjects: std::map<miString,float> parametersFromTimeValues_;
  // to real stuff ...

  fetCodeExecutor executor;
  bool ok;

  // add changes to guielements
  ok = executor.changeGuiElements(fetObj,fetObj.parametersFromTimeValues());

  // compile and fetch gui components from code
  vector<fetCodeExecutor::responce> responcel;
  map<miString,miString> guikeys;
  bool onlygui=false;
  ok = executor.compile( fetObj.baseObject(), responcel, fetObj.guiElements(), onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
  }
  vector<fetDynamicGui::GuiComponent> components = executor.getGuiComponents();

  // set local values
  fetObj.setTimeVariables(executor.timevariables());
  fetObj.setValuesForZeroImpact(executor.valuesforzeroimpact());

  // actuallly set the values!!
  setGuiValues(fetObj,components);

  // remove time values
  fetObj.clearParametersFromTimeValues();

  return ok;
}




