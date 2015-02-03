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
const char KEY_CROSSECTION_EQ[] = "CROSSECTION=";

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
  METLIBS_LOG_SCOPE();
  // TODO almost the same code exists in bdiana_capi

  string_v vcross_data, vcross_options;
  std::string crossection;

  for (string_v::const_iterator it = qm_lines.begin(); it != qm_lines.end(); ++it) { // copy because it has to be trimmed
    const std::string line = miutil::trimmed(*it);
    if (line.empty())
      continue;

    const std::string upline = miutil::to_upper(line);

    if (diutil::startswith(upline, KEY_CROSSECTION_EQ)) {
      // next line assumes that miutil::to_upper does not change character count
      crossection = line.substr(sizeof(KEY_CROSSECTION_EQ)-1);
      if (miutil::contains(crossection, "\""))
        miutil::remove(crossection, '\"');
    } else if (diutil::startswith(upline, "VCROSS ")) {
      vcross_data.push_back(line);
    } else {
      // assume setup options
      vcross_options.push_back(line);
    }
  }

  mManager->getOptions()->readOptions(vcross_options);
  mManager->selectFields(vcross_data);
  if (!crossection.empty()) {
    const int idx = mManager->findCrossectionIndex(QString::fromStdString(crossection));
    mManager->setCrossectionIndex(idx);
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

    const QString cs = mManager->getCrossectionLabel();
    if (!cs.isEmpty())
      qm.push_back(KEY_CROSSECTION_EQ + cs.toStdString());
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
