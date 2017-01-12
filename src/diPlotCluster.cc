#include "diPlotCluster.h"

#include "diUtilities.h" // delete_all_and_clear

#include <puTools/miStringFunctions.h>

PlotCluster::PlotCluster()
  : canvas_(0)
{
}

PlotCluster::~PlotCluster()
{
  cleanup();
}

void PlotCluster::cleanup()
{
  diutil::delete_all_and_clear(plots_);
}

void PlotCluster::setCanvas(DiCanvas* canvas)
{
  if (canvas == canvas_)
    return;
  canvas_ = canvas;
  for (size_t i = 0; i < plots_.size(); i++) {
    plots_[i]->setCanvas(canvas_);
  }
}

void PlotCluster::plot(DiGLPainter* gl, Plot::PlotOrder zorder)
{
  for (size_t i = 0; i < plots_.size(); i++)
    plots_[i]->plot(gl, zorder);
}

void PlotCluster::addAnnotations(std::vector<AnnotationPlot::Annotation>& annotations)
{
  Colour col;
  std::string str;
  AnnotationPlot::Annotation ann;
  for (size_t i = 0; i < plots_.size(); i++) {
    if (!at(i)->isEnabled())
      continue;
    at(i)->getAnnotation(str, col);
    if (!str.empty()) {
      ann.str = str;
      ann.col = col;
      annotations.push_back(ann);
    }
  }
}

void PlotCluster::addPlotElements(std::vector<PlotElement>& pel)
{
  const std::string& key = keyPlotElement();
  for (size_t j = 0; j < plots_.size(); j++) {
    const std::string& nm = plots_[j]->getPlotName();
    if (!nm.empty()) {
      std::string str = nm + "# " + miutil::from_number(int(j));
      bool enabled = plots_[j]->isEnabled();
      pel.push_back(PlotElement(key, str, key, enabled));
    }
  }
}

bool PlotCluster::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != keyPlotElement())
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

Plot* PlotCluster::at(size_t i)
{
  return plots_[i];
}
