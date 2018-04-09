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
#ifndef DIANA_DIMAPAREANAVIGATOR_H
#define DIANA_DIMAPAREANAVIGATOR_H

#include "diField/diArea.h"

#include <deque>
#include <vector>

class EventResult;
class PlotModule;
class QKeyEvent;
class QMouseEvent;

class MapAreaNavigator
{
public:
  MapAreaNavigator(PlotModule* plotm);
  ~MapAreaNavigator();

  //! Zoom at a specified position.
  /*! \param steps number of steps to zoom, positive for zooming in, negative for zooming out
   *  \param frac_x relative position on map window (x=left to right)
   *  \param frac_y relative position on map window (y=bottom to top)
   */
  void zoomAt(int steps, float frac_x, float frac_y);

  //! Zoom out one step at center.
  /*! \see zoomAt */
  void zoomOut();

  //! Zoom in one step at center.
  /*! \see zoomAt */
  void zoomIn();

  void areaHome();

  void togglePanStepDirection();

  //! Pan stepwise.
  /*! The panning direction may be toggled using togglePanStepDirection().
   * \param dx pan left if below zero, right if above zero
   * \param dx pan down if below zero, up if above zero
   */
  void panStep(int dx, int dy);

  void defineUserArea();

  void recallUserArea();

  bool recallFkeyArea(const std::string& fkey);

  bool recallPreviousArea();

  bool recallNextArea();

  bool sendMouseEvent(QMouseEvent* me, EventResult& res);

  bool sendKeyboardEvent(QKeyEvent* ke, EventResult& res);

  // return settings formatted for log file
  std::vector<std::string> writeLog();

  // read settings from log file data
  void readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion);

private:
  /// push a new area onto the area history stack
  void areaInsert(bool);

  void startRubberOrClick(int x, int y, EventResult& res);
  void moveRubberOrClick(int x, int y, EventResult& res);
  void stopRubberOrClick(int x, int y, EventResult& res);
  void startPanning(int x, int y, EventResult& res);
  void movePanning(int x, int y, EventResult& res);
  void stopPanning(int x, int y, EventResult& res);

  /// create a Rectangle from staticPlot phys size
  Rectangle getPhysRectangle() const;

private:
  PlotModule* plotm;

  Area myArea;
  std::deque<Area> areaQ;
  int areaIndex;
  bool areaSaved;

  int arrowKeyDirection;

  float panPreviousX, panPreviousY;
  Rectangle rubberband;
};

#endif
