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

#include "VcrossPlotCommand.h"
#include "util/string_util.h"
#include <puTools/miStringFunctions.h>
#include <sstream>

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


void VcrossQuickmenues::parse(const PlotCommand_cpv& qm_lines)
{
  parse(mManager, qm_lines);
}

// static
void VcrossQuickmenues::parse(QtManager_p manager, const PlotCommand_cpv& qm_lines)
{
  METLIBS_LOG_SCOPE();

  std::vector<miutil::KeyValue_v> vcross_data, vcross_options;
  QString name_cs_tg;
  std::string lonlat_cs_tg;
  bool is_cs = false, is_tg = false, is_dyn = false;

  for (PlotCommand_cp pc : qm_lines) {
    VcrossPlotCommand_cp cmd = std::dynamic_pointer_cast<const VcrossPlotCommand>(pc);
    if (!cmd)
      continue;

    if (cmd->type() == VcrossPlotCommand::CROSSECTION || cmd->type() == VcrossPlotCommand::CROSSECTION_LONLAT) {
      is_cs = true;
      is_tg = false;
    }
    if (cmd->type() == VcrossPlotCommand::TIMEGRAPH || cmd->type() == VcrossPlotCommand::TIMEGRAPH_LONLAT) {
      is_cs = false;
      is_tg = true;
    }
    if (cmd->type() == VcrossPlotCommand::CROSSECTION || cmd->type() == VcrossPlotCommand::TIMEGRAPH) {
      name_cs_tg = QString::fromStdString(cmd->get(0).value());
      METLIBS_LOG_DEBUG(LOGVAL(name_cs_tg.toStdString()));
      continue;
    }
    if (cmd->type() == VcrossPlotCommand::CROSSECTION_LONLAT || cmd->type() == VcrossPlotCommand::TIMEGRAPH_LONLAT) {
      lonlat_cs_tg = cmd->get(0).value();
      is_dyn = true;
      METLIBS_LOG_DEBUG(LOGVAL(lonlat_cs_tg));
      continue;
    }
    if (cmd->type() == VcrossPlotCommand::FIELD) {
      vcross_data.push_back(cmd->all());
    } else {
      // assume setup options
      vcross_options.push_back(cmd->all());
    }
  }

  manager->fieldChangeStart(true);
  manager->getOptions()->readOptions(vcross_options);
  manager->selectFields(vcross_data);

  METLIBS_LOG_DEBUG(LOGVAL(is_cs) << LOGVAL(is_tg));
  if (is_cs || is_tg) {
    manager->switchTimeGraph(is_tg);
    if (is_dyn) {
      // dynamic cs / timegraph
      if (name_cs_tg.isEmpty())
        name_cs_tg = "dyn_qm";
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
        manager->addDynamicCrossection(name_cs_tg, points);
    }
    const int idx = manager->findCrossectionIndex(name_cs_tg);
    manager->setCrossectionIndex(idx);
  }
  manager->fieldChangeDone();
  manager->updateOptions();
}


PlotCommand_cpv VcrossQuickmenues::get() const
{
  METLIBS_LOG_SCOPE();

  PlotCommand_cpv qm;

  const int n = mManager->getFieldCount();
  if (n > 0) {
    qm.push_back(std::make_shared<VcrossPlotCommand>(VcrossPlotCommand::FIELD));

    const std::vector<miutil::KeyValue_v> options = mManager->getOptions()->writeOptions();
    for (const miutil::KeyValue_v& o : options) {
      VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>(VcrossPlotCommand::OPTIONS);
      cmd->add(o);
      qm.push_back(cmd);
    }

    for (int i=0; i<n; ++i) {
      VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>(VcrossPlotCommand::FIELD);
      cmd->add("model", mManager->getModelAt(i));
      cmd->add("reftime", mManager->getReftimeAt(i).isoTime("T"));
      cmd->add("field", mManager->getFieldAt(i));
      cmd->add(mManager->getOptionsAt(i));
      qm.push_back(cmd);
    }

    const bool is_tg = mManager->isTimeGraph();
    const QString cs_label = mManager->getCrossectionLabel();
    if (!cs_label.isEmpty()) {
      VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>
          (is_tg ? VcrossPlotCommand::TIMEGRAPH : VcrossPlotCommand::CROSSECTION);
      cmd->add(is_tg ? KEY_TIMEGRAPH : KEY_CROSSECTION, cs_label.toStdString());
      qm.push_back(cmd);

      const LonLat_v points = mManager->getDynamicCrossectionPoints(cs_label);
      if (!points.empty()) {
        VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>
            (is_tg ? VcrossPlotCommand::TIMEGRAPH_LONLAT : VcrossPlotCommand::CROSSECTION_LONLAT);
        std::ostringstream pstr;
        for (LonLat_v::const_iterator it = points.begin(); it != points.end(); ++it)
          pstr << it->lonDeg() << ',' << it->latDeg() << ' ';
        cmd->add(is_tg ? KEY_TIMEGRAPH_LONLAT : KEY_CROSSECTION_LONLAT, pstr.str());
        qm.push_back(cmd);
      }
    }

    for (size_t i=0; i<mManager->getMarkerCount(); ++i) {
      VcrossPlotCommand_p cmd = std::make_shared<VcrossPlotCommand>(VcrossPlotCommand::FIELD);
      cmd->add(miutil::KeyValue("MARKER"));
      cmd->add("text", mManager->getMarkerText(i));
      cmd->add(miutil::kv("position", mManager->getMarkerPosition(i)));
      cmd->add("colour", mManager->getMarkerColour(i));
      qm.push_back(cmd);
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
  const PlotCommand_cpv qm = get();
  if (!qm.empty())
    Q_EMIT quickmenuUpdate(getQuickMenuTitle(), qm);
}

} // namespace vcross
