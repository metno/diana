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

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miSetupParser.h>

#define MILOGGER_CATEGORY "diana.VcrossWindowInterface"
#include <miLogger/miLogging.h>

using namespace vcross;

VcrossWindowInterface::VcrossWindowInterface()
  : vcrossm(miutil::make_shared<QtManager>())
  , quickmenues(new VcrossQuickmenues(vcrossm))
  , window(0)
{
  connect(quickmenues.get(), SIGNAL(quickmenuUpdate(const std::string&, const std::vector<std::string>&)),
      this, SIGNAL(quickMenuStrings(const std::string&, const std::vector<std::string>&)));

  { vcross::QtManager* m = vcrossm.get();
    connect(m, SIGNAL(crossectionListChanged()),
        this, SLOT(crossectionListChangedSlot()));
    connect(m, SIGNAL(crossectionIndexChanged(int)),
        this, SLOT(crossectionChangedSlot(int)));
    connect(m, SIGNAL(timeListChanged()),
        this, SLOT(timeListChangedSlot()));
    connect(m, SIGNAL(timeIndexChanged(int)),
        this, SLOT(timeChangedSlot(int)));
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

    connect(window, SIGNAL(VcrossHide()),
        this, SLOT(onVcrossHide()));

    connect(window, SIGNAL(requestHelpPage(const std::string&, const std::string&)),
        this, SIGNAL(requestHelpPage(const std::string&, const std::string&)));

    connect(window, SIGNAL(requestLoadCrossectionFiles(const QStringList&)),
        this, SIGNAL(requestLoadCrossectionFiles(const QStringList&)));

    connect(window, SIGNAL(requestVcrossEditor(bool)),
      this, SIGNAL(requestVcrossEditor(bool)));

    connect(window, SIGNAL(vcrossHistoryPrevious()),
        this, SIGNAL(vcrossHistoryPrevious()));

    connect(window, SIGNAL(vcrossHistoryNext()),
        this, SIGNAL(vcrossHistoryNext()));
  }
  return window != 0;
}


void VcrossWindowInterface::onVcrossHide()
{
  vcrossm->cleanup();
  mPredefinedCsFiles.clear();
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


void VcrossWindowInterface::changeCrossection(const std::string& csName)
{
  vcrossm->setCrossectionIndex(vcrossm->findCrossectionIndex(QString::fromStdString(csName)));
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

void VcrossWindowInterface::parseQuickMenuStrings(const std::vector<std::string>& qm_strings)
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

  std::vector<miutil::miTime> times;
  const int count = vcrossm->getTimeCount();
  METLIBS_LOG_DEBUG(LOGVAL(count));
  for (int i=0; i<count; ++i) {
    METLIBS_LOG_DEBUG(LOGVAL(i));
    times.push_back(vcrossm->getTimeValue(i));
  }

  Q_EMIT emitTimes("vcross", times);
}


void VcrossWindowInterface::timeChangedSlot(int current)
{
  Q_EMIT setTime("vcross", vcrossm->getTimeValue(current));
}


void VcrossWindowInterface::crossectionListChangedSlot()
{
  METLIBS_LOG_SCOPE();

  LocationData locations;
  vcrossm->getCrossections(locations);
  Q_EMIT crossectionSetChanged(locations);
  Q_EMIT crossectionChanged(vcrossm->getCrossectionLabel());

  loadPredefinedCS();
}


void VcrossWindowInterface::crossectionChangedSlot(int current)
{
  METLIBS_LOG_SCOPE();
  // send name of current crossection (to mainWindow)
  Q_EMIT crossectionChanged(vcrossm->getCrossectionLabel());
}


void VcrossWindowInterface::loadPredefinedCS()
{
  METLIBS_LOG_SCOPE();
  if (!vcrossm->supportsDynamicCrossections())
    return;

  typedef std::set<std::string> string_s;
  const string_s& csPredefined = vcrossm->getCrossectionPredefinitions();
  if (not csPredefined.empty()) {
    QStringList filenames;
    for (string_s::const_iterator it = csPredefined.begin(); it != csPredefined.end(); ++it) {
      if (mPredefinedCsFiles.insert(*it).second)
        filenames << QString::fromStdString(*it);
    }
    if (!filenames.isEmpty())
      Q_EMIT requestLoadCrossectionFiles(filenames);
  }
}
