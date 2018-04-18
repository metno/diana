/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2018 met.no

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

#include "diMapAreaNavigator.h"

#include "diEventResult.h"
#include "diMapAreaSetup.h"
#include "diPlotModule.h"
#include "diStaticPlot.h"
#include "util/math_util.h"

#include <puTools/miStringFunctions.h>

#include <QMouseEvent>

#define MILOGGER_CATEGORY "diana.MapAreaNavigator"
#include <miLogger/miLogging.h>

namespace {

// minimum rubberband size for zooming (in pixels)
const float RUBBER_LIMIT = 16;

const float PAN_FRACTION = 1 / 8.0f;
const float ZOOM_FACTOR = 1.3f;

} // anonymous namespace

MapAreaNavigator::MapAreaNavigator(PlotModule* pm)
    : plotm(pm)
    , areaIndex(-1)
    , areaSaved(false)
    , arrowKeyDirection(1)
{
}

MapAreaNavigator::~MapAreaNavigator()
{
}

void MapAreaNavigator::areaInsert(bool newArea)
{
  if (newArea && areaSaved) {
    areaSaved = false;
    return;
  }
  if (!newArea) {
    if (areaSaved)
      return;
    else
      areaSaved = true;
  }

  if (areaIndex > -1) {
    areaQ.erase(areaQ.begin() + areaIndex + 1, areaQ.end());
  }
  if (areaQ.size() > 20)
    areaQ.pop_front();
  else
    areaIndex++;

  areaQ.push_back(plotm->getMapArea());
}

void MapAreaNavigator::defineUserArea()
{
  myArea = plotm->getStaticPlot()->getMapArea();
}

void MapAreaNavigator::recallUserArea()
{
  areaInsert(true);
  plotm->getStaticPlot()->setMapArea(myArea);
}

bool MapAreaNavigator::recallPreviousArea()
{
  areaInsert(false);
  if (areaIndex < 1)
    return false;
  areaIndex--;
  plotm->setMapArea(areaQ[areaIndex]);
  return true;
}

bool MapAreaNavigator::recallNextArea()
{
  areaInsert(false);
  if (areaIndex + 2 > int(areaQ.size()))
    return false;
  areaIndex++;
  plotm->setMapArea(areaQ[areaIndex]);
  return true;
}

bool MapAreaNavigator::recallFkeyArea(const std::string& fkey)
{
  Area area;
  if (!MapAreaSetup::instance()->getMapAreaByFkey(fkey, area))
    return false;
  plotm->setMapArea(area);
  return true;
}

void MapAreaNavigator::areaHome()
{
  const bool kca = plotm->isKeepCurrentArea();
  plotm->setKeepCurrentArea(false);
  plotm->updatePlots();
  plotm->setKeepCurrentArea(kca);
}

void MapAreaNavigator::togglePanStepDirection()
{
  arrowKeyDirection *= -1;
}

void MapAreaNavigator::panStep(int dx, int dy)
{
  if (dx == 0 && dy == 0)
    return;

  areaInsert(true);
  dx *= arrowKeyDirection * PAN_FRACTION * plotm->getStaticPlot()->getPhysWidth();
  dy *= arrowKeyDirection * PAN_FRACTION * plotm->getStaticPlot()->getPhysHeight();
  Rectangle r = diutil::translatedRectangle(getPhysRectangle(), dx, dy);
  plotm->setMapAreaFromPhys(r);
}

void MapAreaNavigator::startRubberOrClick(int x, int y, EventResult& res)
{
  plotm->setRubberband(true);
  rubberband.x2 = rubberband.x1 = x;
  rubberband.y2 = rubberband.y1 = y;
  plotm->setRubberbandRectangle(rubberband);

  res.enable_background_buffer = true;
  res.update_background_buffer = false;
  res.repaint = true;
}

void MapAreaNavigator::moveRubberOrClick(int x, int y, EventResult& res)
{
  rubberband.x2 = x;
  rubberband.y2 = y;
  plotm->setRubberbandRectangle(rubberband);

  res.enable_background_buffer = true;
  res.update_background_buffer = false;
  res.action = quick_browsing;
  res.repaint = true;
}

void MapAreaNavigator::stopRubberOrClick(int x, int y, EventResult& res)
{
  float x1 = rubberband.x1, y1 = rubberband.y1, x2 = x, y2 = y;
  if (fabsf(x2 - x1) > RUBBER_LIMIT && fabsf(y2 - y1) > RUBBER_LIMIT) {
    if (plotm->isRubberband()) {
      // define new plotarea, first save the old one
      diutil::sort2(x1, x2);
      diutil::sort2(y1, y2);
      areaInsert(true);
      plotm->setMapAreaFromPhys(Rectangle(x1, y1, x2, y2));

      res.enable_background_buffer = false;
      res.update_background_buffer = true;
      res.repaint = true;
    }
  } else {
    res.enable_background_buffer = false;
    res.update_background_buffer = false;
    res.action = pointclick;
  }

  plotm->setRubberband(false);
}

void MapAreaNavigator::startPanning(int x, int y, EventResult& res)
{
  panPreviousX = x;
  panPreviousY = y;

  areaInsert(true);
  plotm->getStaticPlot()->setPanning(true);
  res.newcursor = paint_move_cursor;
}

void MapAreaNavigator::movePanning(int x, int y, EventResult& res)
{
  const float dx = panPreviousX - x, dy = panPreviousY - y;
  plotm->setMapAreaFromPhys(diutil::translatedRectangle(getPhysRectangle(), dx, dy));
  panPreviousX = x;
  panPreviousY = y;

  res.enable_background_buffer = true;
  res.update_background_buffer = true;
  res.action = quick_browsing;
  res.repaint = true;
  res.newcursor = paint_move_cursor;
}

void MapAreaNavigator::stopPanning(int, int, EventResult& res)
{
  plotm->getStaticPlot()->setPanning(false);
  res.enable_background_buffer = false;
  res.update_background_buffer = false;
  res.repaint = true;
}

// keyboard/mouse events
bool MapAreaNavigator::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
  if (me->type() == QEvent::MouseButtonPress) {
    if (me->button() == Qt::LeftButton) {
      startRubberOrClick(me->x(), me->y(), res);
    } else if (me->button() == Qt::MidButton) {
      startPanning(me->x(), me->y(), res);
    } else if (me->button() == Qt::RightButton) {
      res.action = rightclick;
    } else {
      return false;
    }
  } else if (me->type() == QEvent::MouseMove) {
    res.action = browsing;
    if (plotm->isRubberband()) {
      moveRubberOrClick(me->x(), me->y(), res);
    } else if (plotm->getStaticPlot()->isPanning()) {
      movePanning(me->x(), me->y(), res);
    } else {
      return false;
    }
  } else if (me->type() == QEvent::MouseButtonRelease) {
    if (me->button() == Qt::LeftButton) {
      stopRubberOrClick(me->x(), me->y(), res);
    } else if (me->button() == Qt::MidButton) {
      stopPanning(me->x(), me->y(), res);
    } else if (me->button() == Qt::RightButton) { // zoom out
      // end of popup
    } else {
      return false;
    }
  } else if (me->type() == QEvent::MouseButtonDblClick) {
    res.action = doubleclick;
  }
  return true;
}

bool MapAreaNavigator::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
  if (ke->type() != QEvent::KeyPress)
    return false;

  const int key = ke->key();
  if (key == Qt::Key_R && (ke->modifiers() & ~Qt::ShiftModifier) == 0) { // ignore Shift modifier
    togglePanStepDirection();
    // no repaint needed
    return true;
  }

  bool handled = false;
  if ((ke->modifiers() & Qt::AltModifier) == 0) {
    handled = true;
    if (key == Qt::Key_F2) {
      if (ke->modifiers() & Qt::ShiftModifier)
        defineUserArea();
      else
        recallUserArea();
    } else if (key == Qt::Key_F3) {
      recallPreviousArea();
    } else if (key == Qt::Key_F4) {
      recallNextArea();
    } else if (key == Qt::Key_F5) {
      recallFkeyArea("F5");
    } else if (key == Qt::Key_F6) {
      recallFkeyArea("F6");
    } else if (key == Qt::Key_F7) {
      recallFkeyArea("F7");
    } else if (key == Qt::Key_F8) {
      recallFkeyArea("F8");
    } else {
      // if (key == Qt::Key_F9)
      //    METLIBS_LOG_WARN("F9 - not defined");
      // if (key == Qt::Key_F10){
      //    METLIBS_LOG_WARN("Show previus plot (apply)");
      // if (key == Qt::Key_F11){
      //    METLIBS_LOG_WARN("Show next plot (apply)");
      handled = false;
    }
  }

  if (!handled && !(ke->modifiers() & Qt::ControlModifier) && !(ke->modifiers() & Qt::GroupSwitchModifier)) { // "Alt Gr" modifier
    handled = true;
    if (key == Qt::Key_Home)
      areaHome();
    else if (key == Qt::Key_Left)
      panStep(-1, 0);
    else if (key == Qt::Key_Right)
      panStep(+1, 0);
    else if (key == Qt::Key_Down)
      panStep(0, -1);
    else if (key == Qt::Key_Up)
      panStep(0, +1);
    else if (key == Qt::Key_X)
      zoomOut();
    else if (key == Qt::Key_Z)
      zoomIn();
    else
      handled = false;
  }

  if (handled) {
    res.repaint = true;
    res.update_background_buffer = true;
  }
  return handled;
}

void MapAreaNavigator::zoomAt(int steps, float frac_x, float frac_y)
{
  if (steps == 0)
    return;

  const float factor = std::pow(ZOOM_FACTOR, -steps); // factor < 1 => zoom in, > 1 => zoom out

  // get visible map rectangle in map projection coordinates
  const Rectangle& ps = plotm->getPlotSize();

  // convert these to coordinates in map projection
  float we_map_x = ps.x1 + frac_x * ps.width();
  float we_map_y = ps.y1 + frac_y * ps.height();

  // calculate new rectangle such that mouse stays in the same place on the map
  const float left = (we_map_x - ps.x1) * factor;
  const float right = (ps.x2 - we_map_x) * factor;
  const float down = (we_map_y - ps.y1) * factor;
  const float up = (ps.y2 - we_map_y) * factor;
  const Rectangle r(we_map_x - left, we_map_y - down, we_map_x + right, we_map_y + up);
  plotm->setMapAreaFromMap(r);
}

void MapAreaNavigator::zoomOut()
{
  areaInsert(true);
  zoomAt(-1, 0.5, 0.5);
}

void MapAreaNavigator::zoomIn()
{
  areaInsert(true);
  zoomAt(+1, 0.5, 0.5);
}

std::vector<std::string> MapAreaNavigator::writeLog()
{
  // put last area in areaQ
  areaInsert(true);

  std::vector<std::string> vstr;

  // Write self-defined area (F2)
  std::string aa = "name=F2 " + myArea.getAreaString();
  vstr.push_back(aa);

  // Write all araes in list (areaQ)
  for (size_t i = 0; i < areaQ.size(); i++) {
    aa = "name=" + miutil::from_number(int(i)) + " " + areaQ[i].getAreaString();
    vstr.push_back(aa);
  }

  return vstr;
}

void MapAreaNavigator::readLog(const std::vector<std::string>& vstr, const std::string&, const std::string&)
{
  areaQ.clear();
  Area area;
  for (const std::string& l : vstr) {
    if (!area.setAreaFromString(l)) {
      continue;
    }
    if (area.Name() == "F2") {
      myArea = area;
    } else {
      areaQ.push_back(area);
    }
  }

  areaIndex = areaQ.size() - 1;
}

Rectangle MapAreaNavigator::getPhysRectangle() const
{
  const float pw = plotm->getStaticPlot()->getPhysWidth(), ph = plotm->getStaticPlot()->getPhysHeight();
  return Rectangle(0, 0, pw, ph);
}
