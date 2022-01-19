/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2021 met.no

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

#ifndef MANAGER_H
#define MANAGER_H

#include "diPlot.h"
#include "diPlotCommand.h"
#include "diPlotElement.h"
#include "diPlotStatus.h"
#include "diTimeTypes.h"

#include <QObject>

#include <string>
#include <vector>

class PlotModule;
class EventResult;
class QKeyEvent;
class QMouseEvent;

class DiCanvas;
class DiGLPainter;

class Manager : public QObject
{
  Q_OBJECT

public:
  Manager();
  virtual ~Manager();

  virtual bool parseSetup() = 0;

  virtual plottimes_t getTimes() const = 0;
  virtual void changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize) = 0;
  virtual void changeTime(const miutil::miTime& time) = 0;
  virtual PlotStatus getStatus() = 0;

  virtual void setCanvas(DiCanvas* canvas);
  DiCanvas* canvas() const
    { return mCanvas; }

  virtual void plot(DiGLPainter* gl, bool under, bool over) = 0;
  virtual void plot(DiGLPainter* gl, PlotOrder zorder) { plot(gl, zorder == PO_LINES, zorder == PO_OVERLAY); }

  virtual bool processInput(const PlotCommand_cpv& inp) = 0;

  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res) = 0;
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) = 0;

  virtual std::vector<std::string> getAnnotations() const = 0;
  virtual std::vector<PlotElement> getPlotElements() = 0;
  virtual QString plotElementTag() const = 0;

  //! enable a plot element; returns true iff some change happened (plot found and change in enabled state)
  virtual bool enablePlotElement(const PlotElement &) = 0;

  virtual bool isEnabled() const;
  virtual bool isEditing() const;
  virtual bool hasFocus() const;

public slots:
  virtual void setEnabled(bool enable);
  virtual void setEditing(bool enable);
  virtual void setFocus(bool enable);

Q_SIGNALS:
  void repaintNeeded(bool updateBackgroundBuffer);

private:
  // Whether the manager has a finished product to show.
  bool enabled;
  // Whether the manager is being used to edit a product interactively.
  bool editing;
  // Whether the manager should accept key events instead of other components.
  bool focus;

  DiCanvas* mCanvas;
};

#endif
