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

#ifndef VPROFBOX_H
#define VPROFBOX_H

#include "diVprofAxesStandard.h"
#include "diVprofValues.h"
#include "util/diKeyValue.h"

class VprofPainter;
class VprofModelSettings;

class VprofBox
{
public:
  VprofBox();
  virtual ~VprofBox();

  //! false = all models on top of each other, true = models side by side
  virtual bool separate() const;

  virtual void setArea(const Rectangle& area) = 0;
  virtual Rectangle area() const = 0;

  //! plotting area with decoration (labels)
  Rectangle size() const;

  virtual void setVerticalAxis(vcross::detail::AxisPtr zaxis) = 0;

  void setId(const std::string& id);
  const std::string& id() const { return id_; }

  float width() const { return width_; }
  void setWidth(float width) { width_ = width; }

  virtual void configure(const miutil::KeyValue_v& options);
  virtual void updateLayout();
  virtual void plotDiagram(VprofPainter* p) = 0;
  virtual void plotDiagramFrame(VprofPainter* p) = 0;
  virtual void plotValues(VprofPainter* p, VprofValues_cp values, const VprofModelSettings& ms) = 0;

  virtual bool addGraph(const miutil::KeyValue_v& options) = 0;
  virtual size_t graphCount() = 0;
  virtual size_t graphComponentCount(size_t graph) = 0;
  virtual std::string graphComponentVarName(size_t graph, size_t component) = 0;

  static const std::string key_id;
  static const std::string key_type;
  static const std::string key_width;

public:
  std::string id_;

  Rectangle margin; //! size of decoration (labels)

private:
  //! plotting width for one model
  float width_;
};
typedef std::shared_ptr<VprofBox> VprofBox_p;

#endif // VPROFBOX_H
