/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diana_config.h"

#include "qtVcrossInterface.h"

#include "miSetupParser.h"
#include "qtVcrossWindow.h"

#define MILOGGER_CATEGORY "diana.VcrossWindowInterface"
#include <miLogger/miLogging.h>

using namespace vcross;

VcrossWindowInterface::VcrossWindowInterface()
  : vcrossm(std::make_shared<QtManager>())
  , quickmenues(new VcrossQuickmenues(vcrossm))
  , window(0)
{
  connect(quickmenues.get(), &VcrossQuickmenues::quickmenuUpdate,
      this, &VcrossWindowInterface::quickMenuStrings);

  { vcross::QtManager* m = vcrossm.get();
    connect(m, &vcross::QtManager::crossectionListChanged, this, &VcrossWindowInterface::crossectionListChangedSlot);
    connect(m, &vcross::QtManager::crossectionIndexChanged, this, &VcrossWindowInterface::crossectionChangedSlot);
    connect(m, &vcross::QtManager::timeListChanged, this, &VcrossWindowInterface::timeListChangedSlot);
    connect(m, &vcross::QtManager::timeIndexChanged, this, &VcrossWindowInterface::timeChangedSlot);
  }
}

VcrossWindowInterface::~VcrossWindowInterface()
{
  delete window;
}

bool VcrossWindowInterface::checkWindow()
{
  if (!window) {
    window = new VcrossWindow(vcrossm);

    connect(window, &VcrossWindow::VcrossHide, this, &VcrossWindowInterface::onVcrossHide);

    connect(window, &VcrossWindow::requestHelpPage, this, &VcrossWindowInterface::requestHelpPage);

    connect(window, &VcrossWindow::requestVcrossEditor, this, &VcrossWindowInterface::requestVcrossEditor);

    connect(window, &VcrossWindow::vcrossHistoryPrevious, this, &VcrossWindowInterface::vcrossHistoryPrevious);

    connect(window, &VcrossWindow::vcrossHistoryNext, this, &VcrossWindowInterface::vcrossHistoryNext);
  }
  return window != 0;
}


void VcrossWindowInterface::onVcrossHide()
{
  vcrossm->cleanup();
  Q_EMIT VcrossHide();
}


void VcrossWindowInterface::makeVisible(bool visible)
{
  if (visible && !window)
    checkWindow();
  if (window)
    window->makeVisible(visible);
}


void VcrossWindowInterface::parseSetup()
{
  METLIBS_LOG_SCOPE();
  string_v sources, computations, plots;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_FILES", sources);
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_COMPUTATIONS", computations);
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_PLOTS", plots);
  vcrossm->parseSetup(sources,computations,plots);
}


void VcrossWindowInterface::changeCrossection(const QString& csName)
{
  vcrossm->setCrossectionIndex(vcrossm->findCrossectionIndex(csName));
}


void VcrossWindowInterface::showTimegraph(const LonLat& position)
{
  vcrossm->switchTimeGraph(true);
  vcrossm->setTimeGraph(position);
}


void VcrossWindowInterface::mainWindowTimeChanged(const miutil::miTime& t)
{
  vcrossm->setTimeToBestMatch(t);
}

void VcrossWindowInterface::parseQuickMenuStrings(const PlotCommand_cpv& qm_strings)
{
  quickmenues->parse(qm_strings);
}

void VcrossWindowInterface::writeLog(LogFileIO& logfile)
{
  if (checkWindow())
    window->writeLog(logfile);
}

void VcrossWindowInterface::readLog(const LogFileIO& logfile, const std::string& thisVersion, const std::string& logVersion,
    int displayWidth, int displayHeight)
{
  if (checkWindow())
    window->readLog(logfile, thisVersion, logVersion, displayWidth, displayHeight);
}

void VcrossWindowInterface::editManagerChanged(const QVariantMap &props)
{
  if (checkWindow())
    window->dynCrossEditManagerChange(props);
}

void VcrossWindowInterface::editManagerRemoved(int id)
{
  if (checkWindow())
    window->dynCrossEditManagerRemoval(id);
}

void VcrossWindowInterface::editManagerEditing(bool editing)
{
  if (checkWindow())
    window->slotCheckEditmode(editing);
}


void VcrossWindowInterface::timeListChangedSlot()
{
  METLIBS_LOG_SCOPE();

  plottimes_t times;
  const int count = vcrossm->getTimeCount();
  METLIBS_LOG_DEBUG(LOGVAL(count));
  for (int i=0; i<count; ++i) {
    METLIBS_LOG_DEBUG(LOGVAL(i));
    times.insert(vcrossm->getTimeValue(i));
  }

  Q_EMIT emitTimes("vcross", times);
}


void VcrossWindowInterface::timeChangedSlot(int current)
{
  if (current >= 0)
    Q_EMIT setTime("vcross", vcrossm->getTimeValue(current));
}


void VcrossWindowInterface::crossectionListChangedSlot()
{
  METLIBS_LOG_SCOPE();

  Q_EMIT crossectionSetChanged(vcrossm->getCrossections());
  Q_EMIT crossectionChanged(vcrossm->getCrossectionLabel());
}


void VcrossWindowInterface::crossectionChangedSlot(int current)
{
  METLIBS_LOG_SCOPE();
  // send name of current crossection (to mainWindow)
  if (current >= 0)
    Q_EMIT crossectionChanged(vcrossm->getCrossectionLabel());
}
