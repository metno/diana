/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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
#ifndef diVprofPlot_h
#define diVprofPlot_h

#include "diGLPainter.h"
#include "diVprofTables.h"
#include "diVprofOptions.h"

#include <puTools/miTime.h>
#include <vector>

/**
   \brief Plots observed or prognostic Vertical Profiles (soundings)

   Data preparation and plotting. A few computations can be done.
   An object holds data for one sounding.
*/
class VprofPlot : public VprofTables
{

  friend class VprofData;
  friend class VprofPilot;
#ifdef BUFROBS
  friend class ObsBufr;
#endif
#ifdef ROADOBS
  friend class VprofRTemp;
  friend class VprofPilot;
#endif

public:
  VprofPlot();
  ~VprofPlot();
  bool plot(DiGLPainter* gl, VprofOptions *vpopt, int nplot);
  void setName(const std::string& name) { text.posName=name; }

private:
  bool idxForValue(float& v, int& i) const;
  float tabForValue(const float* tab, float x) const;

private:
  VprofText text;
  bool   prognostic;
  bool   windInKnots;
  size_t maxLevels;

  std::vector<float> ptt, tt;
  std::vector<float> ptd, td;
  std::vector<float> puv, uu, vv;
  std::vector<float> pom, om;
  std::vector<int>   dd, ff, sigwind;

  std::vector<float> pcom, tcom, tdcom; // common T and Td levels
  std::vector<float> rhum, duct;

  void  relhum(const std::vector<float>& tt,
	       const std::vector<float>& td);
  void ducting(const std::vector<float>& pp,
	       const std::vector<float>& tt,
	       const std::vector<float>& td);
  void  kindex(const std::vector<float>& pp,
	       const std::vector<float>& tt,
	       const std::vector<float>& td);
};

#endif
