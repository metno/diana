/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diController.cc 3685 2013-09-11 17:19:09Z davidb $

  Copyright (C) 2013 met.no

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

#include "diManager.h"
#include <puTools/miStringFunctions.h>

Manager::Manager()
  : enabled(false), editing(false), focus(false), mCanvas(0)
{
}

Manager::~Manager()
{
}

void Manager::setCanvas(DiCanvas* canvas)
{
  mCanvas = canvas;
}

bool Manager::isEnabled() const
{
  return enabled;
}

void Manager::setEnabled(bool enable)
{
  enabled = enable;
}

bool Manager::isEditing() const
{
  return editing;
}

void Manager::setEditing(bool enable)
{
  editing = enable;
}

bool Manager::hasFocus() const
{
  return focus;
}

void Manager::setFocus(bool enable)
{
  focus = enable;
}
