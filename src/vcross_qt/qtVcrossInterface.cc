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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtVcrossInterface.h"

#include "qtVcrossWindow.h"

VcrossWindowInterface::VcrossWindowInterface()
  : window(new VcrossWindow())
{
  connect(window, SIGNAL(VcrossHide()),
      this, SIGNAL(VcrossHide()));

  connect(window, SIGNAL(requestHelpPage(const std::string&, const std::string&)),
      this, SIGNAL(requestHelpPage(const std::string&, const std::string&)));

  connect(window, SIGNAL(requestLoadCrossectionFiles(const QStringList&)),
      this, SIGNAL(requestLoadCrossectionFiles(const QStringList&)));

  connect(window, SIGNAL(requestVcrossEditor(bool)),
      this, SIGNAL(requestVcrossEditor(bool)));

  connect(window, SIGNAL(crossectionSetChanged(const LocationData&)),
      this, SIGNAL(crossectionSetChanged(const LocationData&)));

  connect(window, SIGNAL(crossectionChanged(const QString &)),
      this, SIGNAL(crossectionChanged(const QString &)));

  connect(window, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)),
      this, SIGNAL(emitTimes(const std::string&, const std::vector<miutil::miTime>&)));

  connect(window, SIGNAL(setTime(const std::string&, const miutil::miTime&)),
      this, SIGNAL(setTime(const std::string&, const miutil::miTime&)));

  connect(window, SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)),
      this, SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)));

  connect(window, SIGNAL(prevHVcrossPlot()),
      this, SIGNAL(prevHVcrossPlot()));

  connect(window, SIGNAL(nextHVcrossPlot()),
      this, SIGNAL(nextHVcrossPlot()));
}

VcrossWindowInterface::~VcrossWindowInterface()
{
  delete window;
}

void VcrossWindowInterface::makeVisible(bool visible)
{
  window->makeVisible(visible);
}

void VcrossWindowInterface::parseSetup()
{
  window->parseSetup();
}

bool VcrossWindowInterface::changeCrossection(const std::string& csName)
{
  return window->changeCrossection(csName);
}

void VcrossWindowInterface::mainWindowTimeChanged(const miutil::miTime& t)
{
  window->mainWindowTimeChanged(t);
}

void VcrossWindowInterface::parseQuickMenuStrings(const std::vector<std::string>& vstr)
{
  window->parseQuickMenuStrings(vstr);
}

void VcrossWindowInterface::writeLog(LogFileIO& logfile)
{
  window->writeLog(logfile);
}

void VcrossWindowInterface::readLog(const LogFileIO& logfile, const std::string& thisVersion, const std::string& logVersion,
    int displayWidth, int displayHeight)
{
  window->readLog(logfile, thisVersion, logVersion, displayWidth, displayHeight);
}

void VcrossWindowInterface::editManagerChanged(const QVariantMap &props)
{
  window->dynCrossEditManagerChange(props);
}

void VcrossWindowInterface::editManagerRemoved(int id)
{
  window->dynCrossEditManagerRemoval(id);
}

void VcrossWindowInterface::editManagerEditing(bool editing)
{
  window->slotCheckEditmode(editing);
}
