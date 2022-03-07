/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2022 met.no

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

#include "diStreamlinePlot.h"

#include "diField/diField.h"
#include "diPainter.h"
#include "diPlotArea.h"
#include "diPlotOptions.h"
#include "diRandomStreamlineSeed.h"
#include "diStreamlineGenerator.h"
#include "diStreamlineLengthPainter.h"
#include "diStreamlinePalettePainter.h"
#include "util/debug_timer.h"

#define MILOGGER_CATEGORY "diana.StreamlinePlot"
#include <miLogger/miLogging.h>

namespace {
class HitMap
{
public:
  enum Hit { CLEAR, COLLISION, OUTSIDE };

public:
  HitMap(int blocksize, const diutil::PointI& physSize);

  Hit mark(const diutil::PointI& blck);
  Hit test(const diutil::PointI& blck) const;

  diutil::PointI block(XY phys) const { return diutil::PointI(phys.x() / blocksize_, phys.y() / blocksize_); }

  int size() const { return hit_nx_ * hit_ny_; }

private:
  int index(const diutil::PointI& blck) const;

private:
  int blocksize_;
  int hit_nx_;
  int hit_ny_;
  std::vector<char> hitmap_;
};

HitMap::HitMap(int blocksize, const diutil::PointI& physSize)
    : blocksize_(blocksize)
    , hit_nx_((physSize.x() + blocksize - 1) / blocksize)
    , hit_ny_((physSize.y() + blocksize - 1) / blocksize)
    , hitmap_(hit_nx_ * hit_ny_, 0)
{
}

HitMap::Hit HitMap::test(const diutil::PointI& blck) const
{
  const int idx = index(blck);
  if (idx < 0)
    return OUTSIDE;
  else if (hitmap_[idx])
    return COLLISION;
  return CLEAR;
}

HitMap::Hit HitMap::mark(const diutil::PointI& blck)
{
  const int idx = index(blck);
  if (idx < 0)
    return OUTSIDE;
  else if (hitmap_[idx])
    return COLLISION;
  hitmap_[idx] = 1;
  return CLEAR;
}

int HitMap::index(const diutil::PointI& blck) const
{
  const bool visible = blck.x() >= 0 && blck.x() < hit_nx_ && blck.y() >= 0 && blck.y() < hit_ny_;
  if (!visible)
    return -1;
  return blck.y() * hit_nx_ + blck.x();
}
} // namespace

void renderStreamlines(DiGLPainter* gl, const PlotArea& pa_, Field_cp u, Field_cp v, const PlotOptions& poptions_)
{
  StreamlinePainter_p slpainter;
  if (!poptions_.palettecolours.empty()) {
    slpainter = std::make_shared<StreamlinePalettePainter>(gl, pa_.getPhysToMapScale().x(), poptions_);
  } else {
    slpainter = std::make_shared<StreamlineLengthPainter>(gl, poptions_);
  }

  const int blocksize = 2 * poptions_.linewidth + 3;
  HitMap hitmap(blocksize, pa_.getPhysSize());

  StreamlineSeed_p seed = std::make_shared<RandomStreamlineSeed>(u, v);
  StreamlineGenerator_p slg = std::make_shared<StreamlineGenerator>(u, v);

  diutil::Timer t_gen;
  diutil::Timer t_seed;

  const int seed_attempts = 100; // problem: seed points outside map area -- high zoom vs field outside map
  const int segment_size = 40;
  const int segment_min_clear = segment_size * 3 / 4; // segments with fewer clear blocks will cause a new seed

  std::vector<float> map_x(segment_size), map_y(segment_size);

  int count_bad = 0;

  // repeat a fixed number of times, like hitmap.size() * 0.8
  // -- cannot use hitmap fill, complete field might be small on screen or completely outside
  for (int count = hitmap.size(); count >= 0; --count) {
    diutil::PointF next;
    {
      diutil::TimerSection sec_seed(t_seed);
      // generate new seed position
#if 1
      next = seed->next();
#else
      int i = 0;
      for (i = 0; i < seed_attempts; ++i) {
        next = seed->next();
        // check if there is a collision for the seed position
        float seed_map_x = u->area.fromGridX(next.x()), seed_map_y = u->area.fromGridY(next.y());
        if (u->area.P() != pa_.getMapProjection()) {
          pa_.getMapProjection().convertPoints(u->area.P(), 1, &seed_map_x, &seed_map_y);
        }
        const auto hit = hitmap.test(hitmap.block(pa_.MapToPhys({seed_map_x, seed_map_y})));
        if (hit == HitMap::CLEAR) {
          break;
        }
      }
      // METLIBS_LOG_DEBUG(LOGVAL(i) << LOGVAL(next));
      if (i == seed_attempts) {
        break;
      }
#endif
    }

    while (true) {
      // generate line segment
      t_gen.start();
      const auto sl = slg->generate(next, segment_size);
      t_gen.stop();

      if (sl.size() < 2 || sl.size() > segment_size)
        break;

      // convert to map projection
      for (size_t i = 0; i < sl.size(); ++i) {
        map_x[i] = u->area.fromGridX(sl[i].pos.x());
        map_y[i] = u->area.fromGridY(sl[i].pos.y());
      }
      if (u->area.P() != pa_.getMapProjection()) {
        pa_.getMapProjection().convertPoints(u->area.P(), sl.size(), &map_x[0], &map_y[0]);
      }

      // draw streamline segment, pause for collisions
      int n_clear = 0, n_coll = 0;
      for (size_t i = 0; i < sl.size(); ++i) {
        const auto blck = hitmap.block(pa_.MapToPhys({map_x[i], map_y[i]}));

        if (i > 0) {
          // it is necessary (for coarse grids or high zoom levels) to interpolate line segments
          // that span >=2 blocks and perform hit detection for all intermediate points

          const auto blck0 = hitmap.block(pa_.MapToPhys({map_x[i - 1], map_y[i - 1]}));
          const auto delta = std::max(std::abs(blck.x() - blck0.x()), std::abs(blck.y() - blck0.y()));
          if (delta > 1) {
            // line segment between i-1 and i spans >= 2 blocks
            auto blck_prev = blck0;
            for (int j = 1; j < delta; ++j) {
              const float f1 = j / float(delta);
              const float f0 = 1 - f1;
              const float map_x_j = map_x[i - 1] * f0 + map_x[i] * f1;
              const float map_y_j = map_y[i - 1] * f0 + map_y[i] * f1;
              const auto blck_j = hitmap.block(pa_.MapToPhys({map_x_j, map_y_j})); // intermediate point
              if (blck_j == blck) {
                break;
              } else if (blck_j != blck_prev) {
                blck_prev = blck_j;
                const auto hit_j = hitmap.mark(blck_j);
                if (hit_j != HitMap::CLEAR) {
                  const int k = j - 1;
                  const float g1 = k / float(delta);
                  const float g0 = 1 - g1;
                  const float map_x_k = map_x[i - 1] * g0 + map_x[i] * g1;
                  const float map_y_k = map_y[i - 1] * g0 + map_y[i] * g1;
                  const float speed_k = sl[i - 1].speed * g0 + sl[i - 1].speed * g1;
                  slpainter->add(map_x_k, map_y_k, speed_k);
                  slpainter->close();
                  break;
                }
              }
            }
          }
        }

        const auto hit = hitmap.mark(blck);
        if (hit == HitMap::CLEAR) {
          slpainter->add(map_x[i], map_y[i], sl[i].speed);
          n_clear += 1;
        } else {
          if (hit == HitMap::COLLISION)
            n_coll += 1;
          slpainter->close();
        }

        // handle all points that stay in the same hitmap block
        for (; i + 1 < sl.size(); ++i) {
          if (blck != hitmap.block(pa_.MapToPhys({map_x[i + 1], map_y[i + 1]})))
            break;
          if (hit == HitMap::CLEAR) { // original hit when entering block
            slpainter->add(map_x[i + 1], map_y[i + 1], sl[i + 1].speed);
            n_clear += 1;
          } else if (hit == HitMap::COLLISION) {
            n_coll += 1;
          }
        }
      }

      // update start positions for next segment
      next = sl.back().pos;
      // METLIBS_LOG_DEBUG(LOGVAL(sl.size()) << LOGVAL(n_clear) << LOGVAL(next));

      // segments are short in case of loops or low speed
      if (sl.size() < segment_size)
        break;

      if (n_clear < segment_min_clear)
        break;

      if (n_clear < n_coll) {
        count_bad += 1;
        if (count_bad >= 10 * segment_size)
          break;
      }
      count_bad = 0;
    }

    slpainter->close();

    if (count_bad > segment_size * 10)
      break;
  }
  METLIBS_LOG_DEBUG(LOGVAL(t_gen.elapsed()) << LOGVAL(t_gen.count()) << LOGVAL(t_seed.elapsed()) << LOGVAL(t_seed.count()));
}
