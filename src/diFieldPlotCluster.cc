#include "diFieldPlotCluster.h"

#include "diField/diFieldManager.h"
#include "diFieldPlot.h"
#include "diFieldPlotManager.h"
#include "util/was_enabled.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.FieldPlotCluster"
#include <miLogger/miLogging.h>

namespace {
const std::string FIELD = "FIELD";
}

// Field deletion at the end is done in the cache. The cache destructor is called by
// FieldPlotManagers destructor, which comes before this destructor. Basically we try to
// destroy something in a dead pointer here....

FieldPlotCluster::FieldPlotCluster(FieldManager* fieldm, FieldPlotManager* fieldplotm)
  : fieldm_(fieldm)
  , fieldplotm_(fieldplotm)
{
}

FieldPlotCluster::~FieldPlotCluster()
{
}

const std::string& FieldPlotCluster::plotCommandKey() const
{
  return FIELD;
}

void FieldPlotCluster::prepare(const PlotCommand_cpv& inp)
{
  diutil::was_enabled plotenabled;

  // for now -- erase all fieldplots
  for (unsigned int i = 0; i < plots_.size(); i++)
    plotenabled.save(plots_[i]);
  cleanup();

  // NOTE: If we use the fieldCache, we must clear it here
  // to avoid memory consumption!
  if (inp.empty()) {
    // No fields will be used any more...
    fieldm_->flushCache();
    return;
  }

  for (size_t i=0; i < inp.size(); i++) {
    std::unique_ptr<FieldPlot> fp(fieldplotm_->createPlot(inp[i]));
    if (fp.get()) {
      plotenabled.restore(fp.get());
      fp.get()->setCanvas(canvas_);
      plots_.push_back(fp.release());
    }
  }
}

bool FieldPlotCluster::update()
{
  bool haveFieldData = false;
  for (size_t i = 0; i < plots_.size(); i++)
    haveFieldData |= at(i)->updateIfNeeded();
  return haveFieldData;
}

void FieldPlotCluster::getDataAnnotations(std::vector<std::string>& anno) const
{
  for (Plot* pp : plots_) {
    static_cast<FieldPlot*>(pp)->getDataAnnotations(anno);
  }
}

std::vector<miutil::miTime> FieldPlotCluster::getTimes()
{
  std::vector<miutil::KeyValue_v> pinfos;
  for (size_t i = 0; i < plots_.size(); i++) {
    pinfos.push_back(plots_[i]->getPlotInfo());
    METLIBS_LOG_DEBUG("Field plotinfo:" << plots_[i]->getPlotInfo());
  }
  if (!pinfos.empty())
    return fieldplotm_->getFieldTime(pinfos, false);
  else
    return std::vector<miutil::miTime>();
}

const std::string& FieldPlotCluster::keyPlotElement() const
{
  return FIELD;
}

std::vector<miutil::miTime> FieldPlotCluster::fieldAnalysisTimes() const
{
  std::vector<miutil::miTime> fat;
  for (size_t i = 0; i < plots_.size(); i++) {
    fat.push_back(at(i)->getAnalysisTime());
  }
  return fat;
}

int FieldPlotCluster::getVerticalLevel() const
{
  if (!plots_.empty())
    return at(plots_.size()-1)->getLevel();
  else
    return 0;
}

bool FieldPlotCluster::getRealFieldArea(Area& newMapArea) const
{
  if (plots_.empty())
    return false;

  // set area equal to first EXISTING field-area ("all timesteps"...)
  const int n = plots_.size();
  int i = 0;
  while (i < n && !at(i)->getRealFieldArea(newMapArea))
    i++;
  return (i < n);
}


bool FieldPlotCluster::MapToGrid(const Projection& plotproj, float xmap, float ymap, float& gridx, float& gridy) const
{
  if (plots_.empty())
    return false;

  FieldPlot* fp = at(0);
  if (plotproj == fp->getFieldArea().P()) {
    const std::vector<Field*>& ff = fp->getFields();
    if (!ff.empty()) {
      gridx = ff[0]->area.toGridX(xmap);
      gridy = ff[0]->area.toGridY(ymap);
      return true;
    }
  }
  return false;
}

miutil::miTime FieldPlotCluster::getFieldReferenceTime() const
{
  if (!plots_.empty()) {
    const miutil::KeyValue_v& pinfo = at(0)->getPlotInfo();
    return fieldplotm_->getFieldReferenceTime(pinfo);
  } else {
    return miutil::miTime();
  }
}

std::vector<std::string> FieldPlotCluster::getTrajectoryFields()
{
  std::vector<std::string> vstr;
  for (size_t i = 0; i < plots_.size(); i++) {
    std::string fname = at(i)->getTrajectoryFieldName();
    if (!fname.empty())
      vstr.push_back(fname);
  }
  return vstr;
}

const FieldPlot* FieldPlotCluster::findTrajectoryPlot(const std::string& fieldname)
{
  for (size_t i = 0; i < plots_.size(); ++i) {
    if (miutil::to_lower(at(i)->getTrajectoryFieldName()) == fieldname)
      return at(i);
  }
  return 0;
}

std::vector<FieldPlot*> FieldPlotCluster::getFieldPlots() const
{
  std::vector<FieldPlot*> fieldplots;
  fieldplots.reserve(plots_.size());
  for (std::vector<Plot*>::const_iterator it = plots_.begin(); it != plots_.end(); ++it)
    fieldplots.push_back(static_cast<FieldPlot*>(*it));
  return fieldplots;
}

FieldPlot* FieldPlotCluster::at(size_t i) const
{
  return static_cast<FieldPlot*>(plots_[i]);
}
