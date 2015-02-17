/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015 met.no

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

#include "VcrossQuickmenues.h"

#include "diUtilities.h"
#include <puTools/miStringFunctions.h>
//#include <boost/algorithm/string/case_conv.hpp>
//#include <boost/algorithm/string/join.hpp>

#define MILOGGER_CATEGORY "diana.VcrossQuickmenues"
#include <miLogger/miLogging.h>

namespace /* anonymous */ {

const std::string EMPTY_STRING;
const char KEY_CROSSECTION[] = "CROSSECTION";
const char KEY_TIMEGRAPH[]   = "TIMEGRAPH";
const char KEY_CROSSECTION_LONLAT[] = "CROSSECTION_LONLAT_DEG";
const char KEY_TIMEGRAPH_LONLAT[]   = "TIMEGRAPH_LONLAT_DEG";

} // namespace anonymous

namespace vcross {

VcrossQuickmenues::VcrossQuickmenues(vcross::QtManager_p vcm)
  : mManager(vcm)
  , mInFieldChangeGroup(false)
  , mUpdatesFromScript(false)
  , mFieldsChanged(false)
{
  vcross::QtManager* m = mManager.get();
  connect(m, SIGNAL(fieldChangeBegin(bool)),
      this, SLOT(onFieldChangeBegin(bool)));
  connect(m, SIGNAL(fieldAdded(int)),
      this, SLOT(onFieldAdded(int)));
  connect(m, SIGNAL(fieldRemoved(int)),
      this, SLOT(onFieldRemoved(int)));
  connect(m, SIGNAL(fieldOptionsChanged(int)),
      this, SLOT(onFieldOptionsChanged(int)));
  connect(m, SIGNAL(fieldVisibilityChanged(int)),
      this, SLOT(onFieldVisibilityChanged(int)));
  connect(m, SIGNAL(fieldChangeEnd()),
      this, SLOT(onFieldChangeEnd()));

  connect(m, SIGNAL(crossectionIndexChanged(int)),
      this, SLOT(onCrossectionChanged(int)));
}


VcrossQuickmenues::~VcrossQuickmenues()
{
}


void VcrossQuickmenues::parse(const std::vector<std::string>& qm_lines)
{
  parse(mManager, qm_lines);
}

// static
void VcrossQuickmenues::parse(QtManager_p manager, const std::vector<std::string>& qm_lines)
{
  METLIBS_LOG_SCOPE();

  string_v vcross_data, vcross_options;
  std::string name_cs_tg, lonlat_cs_tg;
  bool is_cs = false, is_tg = false, is_dyn = false;

  for (string_v::const_iterator it = qm_lines.begin(); it != qm_lines.end(); ++it) { // copy because it has to be trimmed
    const std::string line = miutil::trimmed(*it);
    if (line.empty())
      continue;

    const std::vector<std::string> split_eq = miutil::split(line, 1, "=", false);
    if (split_eq.size() == 2) {
      const std::string up0 = miutil::to_upper(split_eq[0]);

      if (up0 == KEY_CROSSECTION || up0 == KEY_CROSSECTION_LONLAT) {
        is_cs = true;
        is_tg = false;
      }
      if (up0 == KEY_TIMEGRAPH || up0 == KEY_TIMEGRAPH_LONLAT) {
        is_cs = false;
        is_tg = true;
      }
      if (up0 == KEY_CROSSECTION || up0 == KEY_TIMEGRAPH) {
        name_cs_tg = split_eq[1];
        METLIBS_LOG_DEBUG(LOGVAL(name_cs_tg));
        continue;
      }
      if (up0 == KEY_CROSSECTION_LONLAT || up0 == KEY_TIMEGRAPH_LONLAT) {
        lonlat_cs_tg = split_eq[1];
        is_dyn = true;
        METLIBS_LOG_DEBUG(LOGVAL(lonlat_cs_tg));
        continue;
      }
    }
    const std::string upline = miutil::to_upper(line);
    if (diutil::startswith(upline, "VCROSS ")) {
      vcross_data.push_back(line);
    } else {
      // assume setup options
      vcross_options.push_back(line);
    }
  }

  manager->getOptions()->readOptions(vcross_options);
  manager->selectFields(vcross_data);

  METLIBS_LOG_DEBUG(LOGVAL(is_cs) << LOGVAL(is_tg));
  if (is_cs || is_tg) {
    manager->switchTimeGraph(is_tg);
    if (!is_dyn) {
      const int idx = manager->findCrossectionIndex(QString::fromStdString(name_cs_tg));
      manager->setCrossectionIndex(idx);
    } else {
      // dynamic cs / timegraph
      const std::string label = name_cs_tg.empty() ? std::string("dyn_qm") : name_cs_tg;
      LonLat_v points;
      const std::vector<std::string> split_ll = miutil::split(lonlat_cs_tg, " ");
      points.reserve(split_ll.size());
      for (std::vector<std::string>::const_iterator it = split_ll.begin(); it != split_ll.end(); ++it) {
        METLIBS_LOG_DEBUG(LOGVAL(*it));
        const std::vector<std::string> split_numbers = miutil::split(*it, ",", false);
        if (split_numbers.size() >= 2) {
          const float lon = miutil::to_float(split_numbers[0]);
          const float lat = miutil::to_float(split_numbers[1]);
          points.push_back(LonLat::fromDegrees(lon, lat));
        }
      }
      if ((is_cs && points.size() >= 2) || (is_tg && points.size() == 1))
        manager->addDynamicCrossection(QString::fromStdString(label), points);
    }
  }
}


std::vector<std::string> VcrossQuickmenues::get() const
{
  METLIBS_LOG_SCOPE();

  string_v qm;

  const int n = mManager->getFieldCount();
  if (n > 0) {
    qm.push_back("VCROSS");

    const string_v options = mManager->getOptions()->writeOptions();
    qm.insert(qm.end(), options.begin(), options.end());

    for (int i=0; i<n; ++i) {
      std::ostringstream field;
      field << "VCROSS model=" << mManager->getModelAt(i)
            << " reftime=" << mManager->getReftimeAt(i).isoTime("T")
            << " field=" << mManager->getFieldAt(i)
            << " " << mManager->getOptionsAt(i);
      qm.push_back(field.str());
    }

    const QString cs_label = mManager->getCrossectionLabel();
    if (!cs_label.isEmpty()) {
      const std::string cs_tg = mManager->isTimeGraph()
          ? KEY_TIMEGRAPH : KEY_CROSSECTION;
      qm.push_back(cs_tg + std::string("=") + cs_label.toStdString());

      const LonLat_v points = mManager->getDynamicCrossectionPoints(cs_label);
      if (!points.empty()) {
        std::ostringstream pstr;
        pstr << (mManager->isTimeGraph() ? KEY_TIMEGRAPH_LONLAT : KEY_CROSSECTION_LONLAT)
             << '=';
        for (LonLat_v::const_iterator it = points.begin(); it != points.end(); ++it)
          pstr << it->lonDeg() << ',' << it->latDeg() << ' ';
        qm.push_back(pstr.str());
      }
    }
  }

  return qm;
}


std::string VcrossQuickmenues::getQuickMenuTitle() const
{
  METLIBS_LOG_SCOPE();
  std::ostringstream shortname;
  std::string previousModel;

  for (size_t i=0; i<mManager->getFieldCount(); ++i) {
    const std::string& mdl = mManager->getModelAt(i);
    if (mdl != previousModel) {
      if (i != 0)
        shortname << ' ';
      shortname << mdl;
      previousModel = mdl;
    }

    shortname << ' ' << mManager->getFieldAt(i);
  }

  if (shortname.tellp() > 0) {
    const QString cs = mManager->getCrossectionLabel();
    if (!cs.isEmpty())
      shortname << ' ' << cs.toStdString();
  }

  return shortname.str();
}


void VcrossQuickmenues::onFieldChangeBegin(bool fromScript)
{
  mInFieldChangeGroup = true;
  mUpdatesFromScript = fromScript;
}


void VcrossQuickmenues::onFieldAdded(int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));
  emitQmIfNotInGroup();
}


void VcrossQuickmenues::onFieldRemoved(int position)
{
  emitQmIfNotInGroup();
}


void VcrossQuickmenues::onFieldOptionsChanged(int position)
{
  METLIBS_LOG_SCOPE(LOGVAL(position));
  emitQmIfNotInGroup();
}


void VcrossQuickmenues::onFieldVisibilityChanged(int position)
{
}


void VcrossQuickmenues::onFieldChangeEnd()
{
  if (!mUpdatesFromScript && mFieldsChanged)
    emitQm();
  mInFieldChangeGroup = mUpdatesFromScript = mFieldsChanged = false;
}


void VcrossQuickmenues::onCrossectionChanged(int currentCs)
{
  METLIBS_LOG_SCOPE(LOGVAL(currentCs));
  if (!mUpdatesFromScript)
    emitQm();
}


void VcrossQuickmenues::emitQmIfNotInGroup()
{
  METLIBS_LOG_SCOPE(LOGVAL(mInFieldChangeGroup));
  if (!mInFieldChangeGroup)
    emitQm();
  else
    mFieldsChanged = true;
}


void VcrossQuickmenues::emitQm()
{
  const string_v qm = get();
  if (!qm.empty())
    Q_EMIT quickmenuUpdate(getQuickMenuTitle(), qm);
}

} // namespace vcross
