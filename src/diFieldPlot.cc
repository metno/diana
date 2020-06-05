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

#include "diFieldPlot.h"

#include "diana_config.h"

#include "diField/diField.h"
#include "diFieldPlotManager.h"
#include "diFieldRenderer.h"
#include "diGLPainter.h"
#include "diStaticPlot.h"
#include "util/misc_util.h"
#include "util/string_util.h"

#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <boost/range/adaptor/reversed.hpp>

#include <sstream>

#define MILOGGER_CATEGORY "diana.FieldPlot"
#include <miLogger/miLogging.h>

FieldPlot::FieldPlot(FieldPlotManager* fieldplotm)
    : fieldplotm_(fieldplotm)
    , renderer_(std::make_shared<FieldRenderer>())
{
  METLIBS_LOG_SCOPE();
  renderer_->setGridConverter(&getStaticPlot()->gc);
}

FieldPlot::~FieldPlot()
{
  METLIBS_LOG_SCOPE();
  clearFields();
}

void FieldPlot::clearFields()
{
  METLIBS_LOG_SCOPE();
  renderer_->clearData();
  fields.clear();
}

std::string FieldPlot::getEnabledStateKey() const
{
  std::ostringstream ost;
  ost << cmd_->field.model << ':' << cmd_->field.name() << ':' << cmd_->field.reftime;
  return ost.str();
}

const Area& FieldPlot::getFieldArea() const
{
  if (fields.size() && fields[0])
    return fields[0]->area;
  else
    return getStaticPlot()->getMapArea(); // return master-area
}

//  return area for existing field
bool FieldPlot::getRealFieldArea(Area& a) const
{
  if (!hasData())
    return false;
  a = fields[0]->area;
  return true;
}

int FieldPlot::getLevel() const
{
  if (!cmd_)
    return 0;
  return atoi(cmd_->field.vlevel.c_str()); // cannot use miutil::to_int here because the string might also contain the unit
}

void FieldPlot::changeTime(const miutil::miTime& mapTime)
{
  const bool update = (ftime.undef() || (ftime != mapTime && cmd_->time.empty()) || fields.empty());
  if (update && fieldplotm_ != 0) {
    Field_pv fv;
    fieldplotm_->makeFields(cmd_, mapTime, fv);
    setData(fv, mapTime);
  }
}

void FieldPlot::changeProjection(const Area& mapArea, const Rectangle& /*plotSize*/, const diutil::PointI& physSize)
{
  PlotArea pa;
  pa.setMapArea(mapArea);
  pa.setPhysSize(physSize.x(), physSize.y());
  renderer_->setMap(pa);
}

void FieldPlot::getAnnotation(std::string& s, Colour& c) const
{
  if (poptions.options_1)
    c = poptions.linecolour;
  else
    c = poptions.fillcolour;

  s = getPlotName();
}

bool FieldPlot::prepare(const std::string& fname, const FieldPlotCommand_cp& cmd)
{
  METLIBS_LOG_SCOPE(LOGVAL(fname));
  PlotOptions setupoptions;
  miutil::KeyValue_v setupopts;
  fieldplotm_->getFieldPlotOptions(fname, setupoptions, setupopts);
  return prepare(setupoptions, setupopts, cmd);
}

bool FieldPlot::prepare(const PlotOptions& setupoptions, const miutil::KeyValue_v& setupopts, const FieldPlotCommand_cp& cmd)
{
  METLIBS_LOG_SCOPE(LOGVAL(cmd->toString()));
  poptions = setupoptions;
  miutil::KeyValue_v opts = setupopts;
  METLIBS_LOG_DEBUG(LOGVAL(poptions.toKeyValueList()) << LOGVAL(opts) << LOGVAL(cmd->options()));
  diutil::insert_all(opts, cmd->options());
  setPlotInfo(opts);

  // FIXME merging options should be done earlier, we should no longer modify the plot command here
  FieldPlotCommand_p cmd_opts = std::make_shared<FieldPlotCommand>(*cmd);
  cmd_opts->clearOptions();
  cmd_opts->addOptions(ooptions);
  ooptions = cmd->options();
  METLIBS_LOG_DEBUG(LOGVAL(cmd_opts->toString()) << LOGVAL(ooptions));
  cmd_ = cmd_opts;

  renderer_->setPlotOptions(poptions); // required for arrow annotations

  if (poptions.maxDiagonalInMeters > -1) {
    const double diagonal = getStaticPlot()->getRequestedarea().getDiagonalInMeters();
    METLIBS_LOG_INFO("requestedarea.getDiagonalInMeters():" << diagonal << "  max:" << poptions.maxDiagonalInMeters);
    if (diagonal > poptions.maxDiagonalInMeters)
      return false;
  }

  return true;
}

const Field_pv& FieldPlot::getFields() const
{
  return fields;
}

FieldPlotCommand_cp FieldPlot::command() const
{
  return cmd_;
}

const miutil::miTime& FieldPlot::getAnalysisTime() const
{
  static const miutil::miTime UNDEF;
  if (!fields.empty())
    return fields[0]->analysisTime;
  else
    return UNDEF;
}

plottimes_t FieldPlot::getFieldTimes() const
{
  return fieldplotm_->getFieldTime(std::vector<FieldPlotCommand_cp>(1, cmd_), false);
}

miutil::miTime FieldPlot::getReferenceTime() const
{
  return fieldplotm_->getFieldReferenceTime(cmd_);
}

void FieldPlot::setData(const Field_pv& vf, const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(vf.size()) << LOGVAL(t.isoTime()));

  clearFields();

  fields = vf;
  ftime = t;

  if (fields.empty()) {
    setPlotName("");
  } else {
    Field_cp f0 = fields[0];
    setPlotName(f0->fulltext);
    vcross::Values_p p0 = f0->palette;
    if (p0 && p0->npoint() > 2) {
      poptions.palettecolours.clear();
      bool alpha = (p0->npoint() == 4);
      for (size_t i = 0; i<p0->nlevel(); ++i) {
        unsigned char r = p0->value(0,i);
        unsigned char g = p0->value(1,i);
        unsigned char b = p0->value(2,i);
        unsigned char a = 255;
        if (alpha)
          a = p0->value(3,i);
        poptions.palettecolours.push_back(Colour(r,g,b,a));
      }
    }
  }

  renderer_->setData(fields, t);
}

namespace {
struct aTable {
  std::string colour;
  std::string pattern;
  std::string text;
};
} // namespace

void FieldPlot::getTableAnnotations(std::vector<std::string>& annos) const
{
  METLIBS_LOG_SCOPE();

  if (!hasData())
    return;

  if (!isEnabled())
    return;

  if (!(plottype() == fpt_fill_cell || plottype() == fpt_contour
        || plottype() == fpt_contour1 || plottype() == fpt_contour2))
    return;

  if (poptions.table == 0 || (poptions.palettecolours.empty() && poptions.patterns.empty()))
    return;

  std::vector<std::string> annos_new;
  for (const std::string& anno : annos) {
    if (miutil::contains(anno, "table")) {
      std::string endString;
      std::string startString;
      const size_t nn = anno.find_first_of(",");
      if (nn != std::string::npos) {
        endString = anno.substr(nn);
        startString = anno.substr(0, nn);
      } else {
        startString = anno;
      }

      //if asking for spesific field
      if (miutil::contains(anno, "table=")) {
        std::string name = startString.substr(startString.find_first_of("=") + 1);
        if (name[0] == '"')
          miutil::remove(name, '"');
        miutil::trim(name);
        if (!miutil::contains(fields[0]->fieldText, name))
          continue;
      }

      //find min/max if repeating colours
      float cmin = fieldUndef;
      float cmax = -fieldUndef;
      if (!poptions.repeat) {
        cmin = poptions.base;
      } else {
        int ndata = fields[0]->area.gridSize();
        for (int i = 0; i < ndata; ++i) {
          if (fields[0]->data[i] != fieldUndef)
            miutil::minimaximize(cmin, cmax, fields[0]->data[i]);
        }
      }

      aTable table;
      std::vector<aTable> vtable;

      size_t ncolours = poptions.palettecolours.size();
      size_t ncold = poptions.palettecolours_cold.size();
      size_t npatterns = poptions.patterns.size();
      size_t nlines = poptions.linevalues.size();
      size_t nloglines = poptions.loglinevalues.size();
      size_t ncodes = std::max(ncolours, npatterns);

      if (cmin > poptions.base)
        ncold = 0;

      const std::vector<std::string> classSpec = miutil::split(poptions.classSpecifications, ",");

      // if class specification is given, do not plot more entries than class specifications
      if (classSpec.size() && ncodes > classSpec.size()) {
        ncodes = classSpec.size();
      }

      if (nlines > 0 && nlines <= ncodes)
        ncodes = nlines;

      for (int i = ncold - 1; i >= 0; i--) {
        table.colour = poptions.palettecolours_cold[i].Name();
        if (npatterns > 0) {
          int ii = (npatterns - 1) - i % npatterns;
          table.pattern = poptions.patterns[ii];
        }
        vtable.push_back(table);
      }

      for (size_t i = 0; i < ncodes; i++) {
        if (ncolours > 0) {
          int ii = i % ncolours;
          table.colour = poptions.palettecolours[ii].Name();
        } else {
          table.colour = poptions.fillcolour.Name();
        }
        if (npatterns > 0) {
          int ii = i % npatterns;
          table.pattern = poptions.patterns[ii];
        }
        vtable.push_back(table);
      }

      const std::string unit = poptions.legendunits.empty() ? std::string() : (" " + poptions.legendunits);

      if (poptions.discontinuous == 1 && classSpec.size()
          && poptions.lineinterval > 0.99 && poptions.lineinterval < 1.01) {
        for (size_t i = 0; i < ncodes; i++) {
          const std::vector<std::string> tstr = miutil::split(classSpec[i], ":");
          if (tstr.size() > 1) {
            vtable[i].text = tstr[1];
          } else {
            vtable[i].text = tstr[0];
          }
        }

      } else if (nlines > 0) {
        if (!classSpec.size()) {
          for (size_t i = 0; i < ncodes - 1; i++) {
            float min = poptions.linevalues[i];
            float max = poptions.linevalues[i + 1];
            std::ostringstream ostr;
            ostr << min << " - " << max << unit;
            vtable[i].text = ostr.str();
          }
          float min = poptions.linevalues[ncodes - 1];
          std::ostringstream ostr;
          ostr << "> " << min << unit;
          vtable[ncodes - 1].text = ostr.str();
        } else {
          for (size_t i = 0; i < ncodes; i++) {
            std::ostringstream ostr;
            ostr << classSpec[i];
            vtable[i].text = ostr.str();
          }
        }

      } else if (nloglines > 0) {
        if (!classSpec.size()) {
          std::vector<float> vlog;
          for (size_t n = 0; n < ncodes; n++) {
            float slog = powf(10.0, n);
            for (size_t i = 0; i < nloglines; i++) {
              vlog.push_back(slog * poptions.loglinevalues[i]);
            }
          }
          for (size_t i = 0; i < ncodes - 1; i++) {
            float min = vlog[i];
            float max = vlog[i + 1];
            std::ostringstream ostr;
            ostr << min << " - " << max << unit;
            vtable[i].text = ostr.str();
          }
          float min = vlog[ncodes - 1];
          std::ostringstream ostr;
          ostr << "> " << min << unit;
          vtable[ncodes - 1].text = ostr.str();
        } else {
          for (size_t i = 0; i < ncodes; i++) {
            std::ostringstream ostr;
            ostr << classSpec[i];
            vtable[i].text = ostr.str();
          }
        }

      } else {
        float max = poptions.base + poptions.lineinterval * (ncodes - 1);
        if (max < poptions.maxvalue) {
          std::ostringstream ostr;
          ostr << "> " << max << unit;
          vtable[ncodes + ncold - 1].text = ostr.str();
        }
        float min = poptions.base - poptions.lineinterval * (int(ncold) - 1);
        if (min > poptions.minvalue) {
          std::ostringstream ostr;
          ostr << "< " << min << unit;
          vtable[0].text = ostr.str();
        }

        //cold colours
        max = poptions.base;
        for (int i = ncold - 1; i > 0; i--) {
          min = max - poptions.lineinterval;
          std::ostringstream ostr;
          if (fabs(min) < poptions.lineinterval / 10)
            min = 0.;
          if (fabs(max) < poptions.lineinterval / 10)
            max = 0.;
          ostr << min << " - " << max << unit;
          max = min;
          vtable[i].text = ostr.str();
          if (max < poptions.minvalue) {
            std::vector<aTable>::iterator it = vtable.begin();
            it += i;
            vtable.erase(vtable.begin(), it);
            ncold -= i;
            break;
          }
        }

        //colours
        if (cmin > poptions.base) {
          float step = ncodes * poptions.lineinterval;
          min = int((cmin - poptions.base) / step) * step + poptions.base;
          if (cmax + cmin > 2 * (min + step))
            min += step;
        } else {
          min = poptions.base + poptions.lineinterval;
        }

        size_t i = 1;
        if (ncold > 0) {
          i = ncold;
          min -= poptions.lineinterval;
        }
        for (; i < ncodes + ncold - 1; i++) {
          max = min + poptions.lineinterval;
          std::ostringstream ostr;
          ostr << min << " - " << max << unit;
          min = max;
          vtable[i].text = ostr.str();
          if (min > poptions.maxvalue) {
            std::vector<aTable>::iterator it = vtable.begin();
            it += i;
            vtable.erase(it, vtable.end());
            break;
          }
        }
      }

      std::ostringstream out;
      out << "table=\"";
      if (poptions.legendtitle.empty()) {
        out << fields[0]->fieldText;
      } else {
        out << poptions.legendtitle;
      }
      for (const auto& t : boost::adaptors::reverse(vtable))
        out << ";" << t.colour << ";" << t.pattern << ";" << t.text;
      out << "\"";
      out << endString;

      annos_new.push_back(out.str());
    }
  }
  diutil::insert_all(annos, annos_new);
}

void FieldPlot::getDataAnnotations(std::vector<std::string>& annos) const
{
  METLIBS_LOG_SCOPE(LOGVAL(annos.size()));

  if (!hasData())
    return;

  std::vector<std::string> new_annos;
  for (std::string& anno : annos) {
    std::string lg;
    size_t k = anno.find("$lg=");
    if (k != std::string::npos) {
      lg = anno.substr(k, 7) + " ";
    }

    if (miutil::contains(anno, "$referencetime")) {
      std::string refString = getAnalysisTime().format("%Y%m%d %H", "", true);
      miutil::replace(anno, "$referencetime", refString);
    }
    if (miutil::contains(anno, "$forecasthour")) {
      std::ostringstream ost;
      ost << fields[0]->forecastHour;
      miutil::replace(anno, "$forecasthour", ost.str());
    }
    miutil::replace(anno, "$currenttime", fields[0]->timetext);
    if (miutil::contains(anno, "$validtime")) {
      const std::string vtime = fields[0]->validFieldTime.format("%Y%m%d %A %H" + lg, "", true);
      miutil::replace(anno, "$validtime", vtime);
    }
    miutil::replace(anno, "$model", fields[0]->modelName);
    miutil::replace(anno, "$idnum", fields[0]->idnumtext);
    miutil::replace(anno, "$level", fields[0]->leveltext);

    if (miutil::contains(anno, "arrow") && renderer_->hasVectorAnnotation()) {
      if (miutil::contains(anno, "arrow="))
        continue;

      std::string endString;
      std::string startString;
      if (miutil::contains(anno, ",")) {
        size_t nn = anno.find_first_of(",");
        endString = anno.substr(nn);
        startString = anno.substr(0, nn);
      } else {
        startString = anno;
      }

      float vas = 0;
      std::string vat;
      renderer_->getVectorAnnotation(vas, vat);
      if (vas > 0) {
        std::string str = "arrow=" + miutil::from_number(vas) + ",tcolour=" + poptions.linecolour.Name() + endString;
        new_annos.push_back(str);
        str = "text=\" " + vat + "\"" + ",tcolour=" + poptions.linecolour.Name() + endString;
        new_annos.push_back(str);
      }
    }
  }
  diutil::insert_all(annos, new_annos);

  getTableAnnotations(annos);
}

void FieldPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  if (!isEnabled())
    return;

  // possibly smooth all "single" fields
  for (Field_p f : fields) {
    if (f && f->data) {
      if (f->numSmoothed < poptions.fieldSmooth) {
        int nsmooth = poptions.fieldSmooth - f->numSmoothed;
        f->smooth(nsmooth);
      } else if (f->numSmoothed > poptions.fieldSmooth) {
        METLIBS_LOG_ERROR("field smoothed too much !!! numSmoothed = " << f->numSmoothed << ", fieldSmooth=" << poptions.fieldSmooth);
      }
    }
  }

  // avoid background colour
  poptions.bordercolour = getStaticPlot()->notBackgroundColour(poptions.bordercolour);
  poptions.linecolour   = getStaticPlot()->notBackgroundColour(poptions.linecolour);
  for (Colour& c : poptions.colours)
    c = getStaticPlot()->notBackgroundColour(c);

  if (poptions.antialiasing)
    gl->Enable(DiGLPainter::gl_MULTISAMPLE);
  else
    gl->Disable(DiGLPainter::gl_MULTISAMPLE);

  renderer_->setPlotOptions(poptions);
  renderer_->setBackgroundColour(getStaticPlot()->getBackgroundColour());
  renderer_->plot(gl, zorder);
}

bool FieldPlot::centerOnGridpoint() const
{
  return plottype() == fpt_alpha_shade || plottype() == fpt_rgb || plottype() == fpt_alarm_box || plottype() == fpt_fill_cell;
}

/*
 plot field values as numbers in each gridpoint
 skip plotting if too many (or too small) numbers
 */
bool FieldPlot::plotNumbers(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();
  return renderer_->plotNumbers(gl);
}

std::string FieldPlot::getModelName()
{
  if (hasData())
    return fields[0]->modelName;
  else
    return std::string();
}

std::string FieldPlot::getTrajectoryFieldName()
{
  std::string str;
  unsigned int nf = 0;
  if (plottype() == fpt_wind)
    nf = 2;
  if (plottype() == fpt_vector)
    nf = 2;

  if (nf >= 2 && fields.size() >= nf) {
    bool ok = true;
    for (unsigned int i = 0; i < nf; i++) {
      if (!fields[i])
        ok = false;
      else if (!fields[i]->data)
        ok = false;
    }
    if (ok)
      str = fields[0]->fieldText;
  }
  return str;
}

bool FieldPlot::hasData() const
{
  if (fields.empty())
    return false;

  const Field_p f0 = fields[0];
  return (f0 && f0->data);
}
