#include "diObsPlotCluster.h"

#include "diEditManager.h"
#include "diObsManager.h"
#include "diObsPlot.h"
#include "diPlotModule.h" // for was_enabled
#include "diUtilities.h"

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ObsPlotCluster"
#include <miLogger/miLogging.h>

namespace {
const std::string OBS = "OBS";
} // namespace

ObsPlotCluster::ObsPlotCluster(ObsManager* obsm, EditManager* editm)
  : hasDevField_(false)
  , obsm_(obsm)
  , editm_(editm)
  , canvas_(0)
{
}

ObsPlotCluster::~ObsPlotCluster()
{
  cleanup();
}

void ObsPlotCluster::cleanup()
{
  diutil::delete_all_and_clear(plots_);
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
    plots_[i]->logStations();
  diutil::delete_all_and_clear(plots_);
  hasDevField_ = false;

  for (size_t i = 0; i < inp.size(); i++) {
    ObsPlot *op = obsm_->createObsPlot(inp[i]);
    if (op) {
      plotenabled.restore(op);
      op->setCanvas(canvas_);
      plots_.push_back(op);
      hasDevField_ |= op->mslp();
    }
  }
}

void ObsPlotCluster::setCanvas(DiCanvas* canvas)
{
  if (canvas == canvas_)
    return;
  canvas_ = canvas;
  for (size_t i = 0; i < plots_.size(); i++) {
    ObsPlot* op = plots_[i];
    op->setCanvas(canvas_);
  }
}

bool ObsPlotCluster::update(bool ifNeeded, const miutil::miTime& t)
{
  bool havedata = false;

  if (!ifNeeded) {
    for (size_t i = 0; i < plots_.size(); i++)
      plots_[i]->logStations();
  }
  for (size_t i = 0; i < plots_.size(); i++) {
    if (!ifNeeded || plots_[i]->updateObs()) {
      if (obsm_->prepare(plots_[i], t)) {
        havedata = true;
      }
    }
    //update list of positions ( used in "PPPP-mslp")
    // TODO this is kind of prepares changeProjection of all ObsPlot's with mslp() == true, to be used in EditManager::interpolateEditFields
    plots_[i]->updateObsPositions();
  }
  return havedata;
}

void ObsPlotCluster::plot(DiGLPainter* gl, Plot::PlotOrder zorder)
{
  if (zorder != Plot::LINES && zorder != Plot::OVERLAY)
    return;

  ObsPlot::clearPos();

  // plot observations (if in fieldEditMode  and the option obs_mslp is true, plot observations in overlay)

  const bool obsedit = (hasDevField_ && editm_->isObsEdit());
  const bool plotoverlay = (zorder == Plot::OVERLAY && obsedit);

  for (size_t i = 0; i < plots_.size(); i++) {
    ObsPlot* op = plots_[i];
    if (plotoverlay) {
      if (editm_->interpolateEditField(op->getObsPositions()))
        op->updateFromEditField();
    }
    op->plot(gl, zorder);
  }
}

void ObsPlotCluster::getAnnotations(std::vector<AnnotationPlot::Annotation>& annotations)
{
  Colour col;
  std::string str;
  AnnotationPlot::Annotation ann;
  for (size_t j = 0; j < plots_.size(); j++) {
    if (!plots_[j]->isEnabled())
      continue;
    plots_[j]->getObsAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }
}

void ObsPlotCluster::getDataAnnotations(std::vector<std::string>& anno)
{
  for (size_t j = 0; j < plots_.size(); j++) {
    plots_[j]->getDataAnnotations(anno);
  }
}

void ObsPlotCluster::getExtraAnnotations(std::vector<AnnotationPlot*>& vap)
{
  //get obs annotations
  for (size_t i = 0; i < plots_.size(); i++) {
    if (!plots_[i]->isEnabled())
      continue;
    std::vector<std::string> obsinfo = plots_[i]->getObsExtraAnnotations();
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

void ObsPlotCluster::addPlotElements(std::vector<PlotElement>& pel)
{
  for (size_t j = 0; j < plots_.size(); j++) {
    std::string str = plots_[j]->getPlotName() + "# " + miutil::from_number(int(j));
    bool enabled = plots_[j]->isEnabled();
    pel.push_back(PlotElement(OBS, str, OBS, enabled));
  }
}

bool ObsPlotCluster::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != OBS)
    return false;
  for (unsigned int i = 0; i < plots_.size(); i++) {
    std::string str = plots_[i]->getPlotName() + "# " + miutil::from_number(int(i));
    if (str == pe.str) {
      Plot* op = plots_[i];
      if (op->isEnabled() != pe.enabled) {
        op->setEnabled(pe.enabled);
        return true;
      } else {
        break;
      }
    }
  }
  return false;
}

bool ObsPlotCluster::findObs(int x, int y)
{
  bool found = false;

  for (size_t i = 0; i < plots_.size(); i++)
    if (plots_[i]->showpos_findObs(x, y))
      found = true;

  return found;
}

bool ObsPlotCluster::getObsName(int x, int y, std::string& name)
{
  for (size_t i = 0; i < plots_.size(); i++)
    if (plots_[i]->getObsName(x, y, name))
      return true;

  return false;
}

std::string ObsPlotCluster::getObsPopupText(int x, int y)
{
  size_t n = plots_.size();
  std::string obsText = "";

  for (size_t i = 0; i < n; i++)
    if (plots_[i]->getObsPopupText(x, y, obsText))
      return obsText;

  return obsText;
}

void ObsPlotCluster::nextObs(bool next)
{
  for (size_t i = 0; i < plots_.size(); i++)
    plots_[i]->nextObs(next);
}
