/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2020 met.no

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

#include "diTrajectoryGenerator.h"

#include "diFieldPlot.h"
#include "diFieldPlotManager.h"
#include "diKVListPlotCommand.h"
#include "diStaticPlot.h"

#include <diField/diField.h>

#define MILOGGER_CATEGORY "diana.TrajectoryGenerator"
#include <miLogger/miLogging.h>

namespace {
Field_cp duplicateFieldWidthData(Field_cp f, const float* data)
{
  METLIBS_LOG_SCOPE();
  Field_p frx = std::make_shared<Field>();
  frx->area=   f->area;
  frx->data = new float[frx->area.gridSize()];
  for (int i=0; i<frx->area.gridSize(); ++i)
    frx->data[i] = data[i];
  frx->checkDefined();
  return frx;
}
} // namespace

TrajectoryGenerator::TrajectoryGenerator(FieldPlotManager* m, const FieldPlot* p
    , const miutil::miTime& t)
  : fpm(m)
  , fp(p)
  , mPositionTime(t)
  , numIterations(5)
  , timeStep(900)

  , xt(0)
  , yt(0)
  , u1(0)
  , v1(0)
  , u2(0)
  , v2(0)
  , rx(0)
  , ry(0)
  , frx(0)
  , fry(0)
{
}

TrajectoryGenerator::~TrajectoryGenerator()
{
  delete[] xt;
  delete[] yt;
  delete[] u1;
  delete[] v1;
  delete[] u2;
  delete[] v2;
  delete[] rx;
  delete[] ry;
}

TrajectoryData_v TrajectoryGenerator::compute()
{
  METLIBS_LOG_SCOPE(LOGVAL(mStartPositions.size()));
  trajectories.clear();
  if (mStartPositions.empty())
    return trajectories;

  // 1. get field times
  const plottimes_t ftimes = fp->getFieldTimes();
  const std::vector<miutil::miTime> times(ftimes.begin(), ftimes.end());
  const int nTimes = times.size();
  METLIBS_LOG_DEBUG(LOGVAL(nTimes));
  if (nTimes == 0)
    return TrajectoryData_v();

  // find out when to go backward and when forward
  int iStart = 0;
  METLIBS_LOG_DEBUG(LOGVAL(mPositionTime));
  while (iStart < nTimes && times[iStart] < mPositionTime)
    iStart += 1;

  // start time not found
  if (iStart >= nTimes)
    return TrajectoryData_v();

  METLIBS_LOG_DEBUG(LOGVAL(times[iStart]));
  reprojectStartPositions();
  initAborted();
  calculateMapFields();

  const int npos = mStartPositions.size();
  u1 = new float[npos];
  v1 = new float[npos];
  u2 = new float[npos];
  v2 = new float[npos];
  rx = new float[npos];
  ry = new float[npos];
  for (int i=0; i<npos; i++) {
    trajectories.push_back(TrajectoryData());
    trajectories.back().mPoints.push_back(TrajectoryPoint(mPositionTime, mStartPositions[i]));
    trajectories.back().mStartIndex = 0;
  }

  // go backward in time from startTime to times[0]
  if (iStart > 0) {
    const std::vector<bool> backupAborted(mAborted);
    METLIBS_LOG_DEBUG("backward loop");
    timeLoop(iStart, -1, times);

    delete[] xt;
    delete[] yt;
    reprojectStartPositions();
    mAborted = backupAborted;
  }

  // go forward in time from startTime to times.back()
  if (iStart < nTimes - 1) {
    METLIBS_LOG_DEBUG("forward loop");
    timeLoop(iStart, +1, times);
  }

  METLIBS_LOG_DEBUG(LOGVAL(trajectories.size()));
  return trajectories;
}

void TrajectoryGenerator::timeLoop(int i0, int di, const std::vector<miutil::miTime>& times)
{
  METLIBS_LOG_SCOPE(LOGVAL(i0) << LOGVAL(di));
  const int nTimes = times.size();
  if (i0 < 0 || i0 >= nTimes)
    return;
  if (!haveTrajectories())
    return;

  Field_pv fv0, fv1;
  fpm->makeFields(fp->command(), times[i0], fv0);
  while (fv0.size() < 2 && i0 >= 0 && i0-di >= 0 && i0 < nTimes && i0-di < nTimes) {
    i0 += di;
  }

  for (int i1 = i0+di; i1 >= 0 && i1-di >= 0 && i1 < nTimes && i1-di < nTimes; i1 += di) {
    if (!haveTrajectories())
      break;
    METLIBS_LOG_DEBUG(LOGVAL(i1));
    const miutil::miTime& t1 = times[i1];
    fpm->makeFields(fp->command(), t1, fv1);
    if (fv1.size() >= 2) {
      const miutil::miTime& t0 = times[i0];
      computeSingleStep(t0, t1, fv0, fv1, trajectories);
      std::swap(fv0, fv1);
      i0 = i1;
    }
  }
}

void TrajectoryGenerator::reprojectStartPositions()
{
  METLIBS_LOG_SCOPE();
  const int npos = mStartPositions.size();
  xt = new float[npos], yt = new float[npos];
  for (int i=0; i<npos; i++) {
    xt[i] = mStartPositions[i].lonDeg();
    yt[i] = mStartPositions[i].latDeg();
  }

  Field_cp fu1 = fp->getFields().front();
  fu1->area.P().convertFromGeographic(npos, xt, yt);
  fu1->convertToGrid(npos, xt, yt);
}

void TrajectoryGenerator::initAborted()
{
  METLIBS_LOG_SCOPE();
  const int npos = mStartPositions.size();
  mAborted.clear();
  mAborted.reserve(npos);
  float *su = new float[npos];
  Field_cp fu1 = fp->getFields().front();
  fu1->interpolate(npos, xt, yt, su, Field::I_BESSEL);
  for (int i=0; i<npos; i++) {
    mAborted.push_back(su[i] == fieldUndef);
    METLIBS_LOG_DEBUG(LOGVAL(mAborted[i]));
  }
  delete[] su;
}

TrajectoryGenerator::LonLat_v TrajectoryGenerator::reprojectStepPositions()
{
  METLIBS_LOG_SCOPE();
  const int npos = mStartPositions.size();
  float *llx = new float[npos], *lly = new float[npos]; // TODO avoid allocation for each step
  for (int i=0; i<npos; i++) {
    llx[i] = xt[i];
    lly[i] = yt[i];
  }

  Field_cp fu1 = fp->getFields().front();
  fu1->convertFromGrid(npos, llx, lly);
  fu1->area.P().convertToGeographic(npos, llx, lly);

  LonLat_v points;
  points.reserve(npos);
  for (int i=0; i<npos; i++)
    points.push_back(LonLat::fromDegrees(llx[i], lly[i]));

  delete[] llx;
  delete[] lly;

  return points;
}

void TrajectoryGenerator::calculateMapFields()
{
  METLIBS_LOG_SCOPE();
  Field_cp fu1 = fp->getFields().front();
  MapFields_cp mf = StaticPlot::gc.getMapFields(fu1->area);
  if (!mf) {
    METLIBS_LOG_ERROR("getMapFields ERROR, cannot compute trajectories!");
    return;
  }

  frx = duplicateFieldWidthData(fu1, mf->xmapr);
  fry = duplicateFieldWidthData(fu1, mf->ymapr);
}

void TrajectoryGenerator::reprojectRoundTripLonLat()
{
  Field_cp fu1 = fp->getFields().front();
  const int npos = mStartPositions.size();
  for (int i=0; i<npos; i++) {
    if (mAborted[i])
      continue;
    if (xt[i] < 0 || xt[i] >= fu1->area.nx-1 || yt[i] < 0 || yt[i] >= fu1->area.ny-1) {
      METLIBS_LOG_DEBUG(i << ": before reprojection lon-lat and back: x=" << xt[i] << " y=" << yt[i]);
      float& x = xt[i], y = yt[i];
      fu1->convertFromGrid(1, &x, &y);
      fu1->area.P().convertToGeographic(1, &x, &y);
      METLIBS_LOG_DEBUG(i << ": during reprojection lon-lat =" << xt[i] << " / " << yt[i]);
      fu1->area.P().convertFromGeographic(1, &x, &y);
      fu1->convertToGrid(1, &x, &y);
      METLIBS_LOG_DEBUG(i << ": after reprojection lon-lat and back: x=" << xt[i] << " y=" << yt[i]);
    }
  }
}

bool TrajectoryGenerator::haveTrajectories() const
{
  const int npos = mStartPositions.size();
  bool haveTrajectories = false;
  for (int i=0; i<npos && !haveTrajectories; i++) {
    haveTrajectories |= !mAborted[i];
  }
  return haveTrajectories;
}

void TrajectoryGenerator::computeSingleStep(const miutil::miTime& t1, const miutil::miTime& t2, const Field_pv& fields1, const Field_pv& fields2,
                                            TrajectoryData_v& tracjectories)
{
  const int npos = mStartPositions.size();

  const float seconds = miutil::miTime::secDiff(t2, t1);
  const bool forward = (seconds > 0);
  const int nstep = std::max(int(fabsf(seconds)/timeStep + 0.5), 1);
  const float nstepi =  1.0/nstep, tStep = seconds*nstepi, dt = tStep * 0.5;

  if (fields1.size() < 2 || fields2.size() < 2) {
    METLIBS_LOG_DEBUG("fields1/2 too short");
    return;
  }

  Field_cp fu1 = fields1[0], fv1 = fields1[1];
  Field_cp fu2 = fields2[0], fv2 = fields2[1];

  if (!(fu1 && fv1 && fu2 && fv2)) {
    METLIBS_LOG_DEBUG("missing one of fu1 / fv1 / fu2 / fv2");
    return;
  }

  // stop trajectories if it is more than one grid point outside the field
  reprojectRoundTripLonLat();
  for (int i=0; i<npos; i++) {
    if (mAborted[i])
      continue;
    if (xt[i] < -1 || xt[i]>fu1->area.nx || yt[i] < -1 || yt[i] > fu1->area.ny) {
      mAborted[i] = true;
      METLIBS_LOG_DEBUG(LOGVAL(xt[i]) << LOGVAL(fu1->area.nx)
          << LOGVAL(yt[i]) << LOGVAL(fu1->area.ny));
    }
  }

  float* xa = new float[npos], *ya = new float[npos];
  for (int istep=0; istep<nstep; istep++) {
    const float ct1b = istep * nstepi, ct1a = 1.0 - ct1b,
        ct2b = (istep+1)*nstepi, ct2a = 1.0 - ct2b;

    // iteration no. 0 to get a first guess (then the real iterations)

    for (int iter=0; iter<=numIterations; iter++) {
      fu1->interpolate(npos, xt, yt, u1, Field::I_BESSEL_EX);
      fv1->interpolate(npos, xt, yt, v1, Field::I_BESSEL_EX);
      fu2->interpolate(npos, xt, yt, u2, Field::I_BESSEL_EX);
      fv2->interpolate(npos, xt, yt, v2, Field::I_BESSEL_EX);
      frx->interpolate(npos, xt, yt, rx, Field::I_BESSEL_EX);
      fry->interpolate(npos, xt, yt, ry, Field::I_BESSEL_EX);

      for (int i=0; i<npos; i++) {
        if (mAborted[i])
          continue;
        if (u1[i]==fieldUndef || v1[i]==fieldUndef || u2[i]==fieldUndef || v2[i]==fieldUndef) {
          METLIBS_LOG_DEBUG(LOGVAL(u1[i]) << LOGVAL(v1[i]) << LOGVAL(u2[i]) << LOGVAL(v2[i]));
          mAborted[i] = true;
          continue;
        }

        if (iter==0) {
          const float u0 = ct1a * u1[i] + ct1b * u2[i],
              v0 = ct1a * v1[i] + ct1b * v2[i];
          xa[i] = xt[i] + rx[i] * u0 * dt;
          ya[i] = yt[i] + ry[i] * v0 * dt;
        }

        const float u = ct2a * u1[i] + ct2b * u2[i],
            v = ct2a * v1[i] + ct2b * v2[i];
        xt[i] = xa[i] + rx[i] * u * dt;
        yt[i] = ya[i] + ry[i] * v * dt;
      }
      reprojectRoundTripLonLat();
    }  // end of iteration loop

    const LonLat_v ll = reprojectStepPositions();

    miutil::miTime t = t1;
    t.addSec(roundf((istep+1) * tStep)); // tStep may be < 0

    for (int i=0; i<npos; i++) {
      METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(mAborted[i]));
      if (mAborted[i])
        continue;

      const TrajectoryPoint p(t, ll[i]);
      TrajectoryData& d = tracjectories[i];
      if (forward) {
        d.mPoints.push_back(p);
      } else {
        d.mPoints.insert(d.mPoints.begin(), p);
        d.mStartIndex += 1;
      }
    }
  }
  delete[] xa;
  delete[] ya;
}
