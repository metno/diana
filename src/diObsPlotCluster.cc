#include "diObsPlotCluster.h"

#include "diEditManager.h"
#include "diObsManager.h"
#include "diObsPlot.h"
#include "diUtilities.h"
#include "util/was_enabled.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsPlotCluster"
#include <miLogger/miLogging.h>

namespace {
const std::string OBS = "OBS";
} // namespace

ObsPlotCluster::ObsPlotCluster(ObsManager* obsm, EditManager* editm)
  : hasDevField_(false)
  , collider_(new ObsPlotCollider)
  , obsm_(obsm)
  , editm_(editm)
{
}

ObsPlotCluster::~ObsPlotCluster()
{
}

const std::string& ObsPlotCluster::plotCommandKey() const
{
  return OBS;
}

void ObsPlotCluster::prepare(const std::vector<std::string>& inp)
{
  diutil::was_enabled plotenabled;
  for (unsigned int i = 0; i < plots_.size(); i++)
    plotenabled.save(plots_[i]);

  // for now -- erase all obsplots etc..
  //first log stations plotted
  for (size_t i = 0; i < plots_.size(); i++)
    at(i)->logStations();
  cleanup();
  hasDevField_ = false;

  for (size_t i = 0; i < inp.size(); i++) {
    ObsPlot *op = obsm_->createObsPlot(inp[i]);
    if (op) {
      plotenabled.restore(op);
      op->setCanvas(canvas_);
      op->setCollider(collider_.get());
      plots_.push_back(op);
      hasDevField_ |= op->mslp();
    }
  }
  collider_->clear();
}

bool ObsPlotCluster::update(bool ifNeeded, const miutil::miTime& t)
{
  bool havedata = false;

  if (!ifNeeded) {
    for (size_t i = 0; i < plots_.size(); i++)
      at(i)->logStations();
  }
  for (size_t i = 0; i < plots_.size(); i++) {
    ObsPlot* op = at(i);
    if (!ifNeeded || op->updateObs()) {
      if (obsm_->prepare(op, t)) {
        havedata = true;
      }
    }
    //update list of positions ( used in "PPPP-mslp")
    // TODO this is kind of prepares changeProjection of all ObsPlot's with mslp() == true, to be used in EditManager::interpolateEditFields
    op->updateObsPositions();
  }
  return havedata;
}

void ObsPlotCluster::plot(DiGLPainter* gl, Plot::PlotOrder zorder)
{
  if (zorder != Plot::LINES && zorder != Plot::OVERLAY)
    return;

  collider_->clear();

  // plot observations (if in fieldEditMode  and the option obs_mslp is true, plot observations in overlay)

  const bool obsedit = (hasDevField_ && editm_->isObsEdit());
  const bool plotoverlay = (zorder == Plot::OVERLAY && obsedit);
  const bool plotunderlay = (zorder == Plot::LINES && !obsedit);

  if (plotoverlay) {
    for (size_t i = 0; i < plots_.size(); i++) {
      ObsPlot* op = at(i);
      if (editm_->interpolateEditField(op->getObsPositions()))
        op->updateFromEditField();
    }
  }
  if (plotunderlay || plotoverlay)
    PlotCluster::plot(gl, zorder);
}

void ObsPlotCluster::getDataAnnotations(std::vector<std::string>& anno) const
{
  for (size_t i = 0; i < plots_.size(); i++) {
    at(i)->getDataAnnotations(anno);
  }
}

void ObsPlotCluster::getExtraAnnotations(std::vector<AnnotationPlot*>& vap)
{
  //get obs annotations
  for (size_t i = 0; i < plots_.size(); i++) {
    if (!at(i)->isEnabled())
      continue;
    std::vector<std::string> obsinfo = at(i)->getObsExtraAnnotations();
    for (size_t j = 0; j < obsinfo.size(); j++) {
      AnnotationPlot* ap = new AnnotationPlot(obsinfo[j]);
      vap.push_back(ap);
    }
  }
}

std::vector<miutil::miTime> ObsPlotCluster::getTimes()
{
  std::vector<std::string> pinfos;
  for (size_t i = 0; i < plots_.size(); i++)
    pinfos.push_back(plots_[i]->getPlotInfo());
  if (pinfos.size() > 0) {
    return obsm_->getObsTimes(pinfos);
  } else {
    return std::vector<miutil::miTime>();
  }
}

const std::string& ObsPlotCluster::keyPlotElement() const
{
  return OBS;
}

bool ObsPlotCluster::findObs(int x, int y)
{
  bool found = false;

  for (size_t i = 0; i < plots_.size(); i++)
    if (at(i)->showpos_findObs(x, y))
      found = true;

  return found;
}

bool ObsPlotCluster::getObsName(int x, int y, std::string& name)
{
  for (size_t i = 0; i < plots_.size(); i++)
    if (at(i)->getObsName(x, y, name))
      return true;

  return false;
}

std::string ObsPlotCluster::getObsPopupText(int x, int y)
{
  size_t n = plots_.size();
  std::string obsText = "";

  for (size_t i = 0; i < n; i++)
    if (at(i)->getObsPopupText(x, y, obsText))
      return obsText;

  return obsText;
}

void ObsPlotCluster::nextObs(bool next)
{
  for (size_t i = 0; i < plots_.size(); i++)
    at(i)->nextObs(next);
}

std::vector<ObsPlot*> ObsPlotCluster::getObsPlots() const
{
  std::vector<ObsPlot*> obsplots;
  obsplots.reserve(plots_.size());
  for (std::vector<Plot*>::const_iterator it = plots_.begin(); it != plots_.end(); ++it)
    obsplots.push_back(static_cast<ObsPlot*>(*it));
  return obsplots;
}

ObsPlot* ObsPlotCluster::at(size_t i) const
{
  return static_cast<ObsPlot*>(plots_[i]);
}
