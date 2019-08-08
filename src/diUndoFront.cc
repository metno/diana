/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2019 met.no

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

#include <diUndoFront.h>
#include <diWeatherFront.h>
#include <diWeatherSymbol.h>
#include <diWeatherArea.h>

UndoFront::UndoFront()
    : Last(nullptr)
    , Next(nullptr)
{
}

UndoFront::~UndoFront()
{
  for (saveObject& so : saveobjects)
    delete so.object;
}

void UndoFront::undoAdd(action todo, ObjectPlot* UndoObject, int place)
{
  //add object to undo buffer
  saveObject so;
  so.todo = todo;
  if (!UndoObject)
    so.object = 0;
  else if (UndoObject->objectIs(wFront))
    so.object = new WeatherFront(*((WeatherFront*)(UndoObject)));
  else if (UndoObject->objectIs(wSymbol))
    so.object = new WeatherSymbol(*((WeatherSymbol*)(UndoObject)));
  else if (UndoObject->objectIs(wArea))
    so.object = new WeatherArea(*((WeatherArea*)(UndoObject)));
  else
    so.object = 0;
  so.place = place;
  saveobjects.push_back(so);
}
