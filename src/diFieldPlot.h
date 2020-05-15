/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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
#ifndef diFieldPlot_h
#define diFieldPlot_h

#include "diPlotOptionsPlot.h"

#include "diFieldPlotCommand.h"
#include "diFieldRendererBase.h"
#include "diTimeTypes.h"

#include <vector>

class DiGLPainter;
class FieldPlotManager;

/**
  \brief Plots one field

  Holds the (scalar) field(s) after reading and any computations.
  Contains all field plotting methodes except contouring.
*/
class FieldPlot : public PlotOptionsPlot
{
public:
  FieldPlot(FieldPlotManager* fieldplotm);
  ~FieldPlot();

  void changeTime(const miutil::miTime& mapTime) override;
  bool hasData() const override;
  void plot(DiGLPainter* gl, PlotOrder zorder) override;
  void getAnnotation(std::string&, Colour&) const override;
  void getDataAnnotations(std::vector<std::string>& anno) const override;
  std::string getEnabledStateKey() const override;

  int getLevel() const;

  //! Extract plotting-parameters from plot command
  //* Also fetches some default options from FieldPlotManager. */
  bool prepare(const std::string& fname, const FieldPlotCommand_cp&);

  bool prepare(const PlotOptions& setupoptions, const miutil::KeyValue_v& setupopts, const FieldPlotCommand_cp& cmd);

  //  set list of field-pointers, update datatime
  void setData(const Field_pv&, const miutil::miTime&);
  const Area& getFieldArea() const;
  bool getRealFieldArea(Area&) const;
  const Field_pv& getFields() const;
  FieldPlotCommand_cp command() const;

  //! time of model analysis
  const miutil::miTime& getAnalysisTime() const;

  plottimes_t getFieldTimes() const;

  miutil::miTime getReferenceTime() const;

  /*!
   plot field values as numbers in each gridpoint
   skip plotting if too many (or too small) numbers
   */
  bool plotNumbers(DiGLPainter* gl);

  std::string getModelName();
  std::string getTrajectoryFieldName();

private:
  FieldPlotManager* fieldplotm_;
  FieldPlotCommand_cp cmd_;
  Field_pv fields;               // fields, stored elsewhere
  miutil::miTime ftime;          // current field time

  void clearFields();
  void getTableAnnotations(std::vector<std::string>& anno) const;

  bool centerOnGridpoint() const;
  const std::string& plottype() const
    { return getPlotOptions().plottype; }

  FieldRendererBase_p renderer_;
};

#endif
