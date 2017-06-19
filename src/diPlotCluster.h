#ifndef PLOTCLUSTER_H
#define PLOTCLUSTER_H

#include "diAnnotationPlot.h" // AnnotationPlot::Annotation
#include "diCommonTypes.h" // PlotElement
#include "diPlot.h" // Plot, Plot::PlotOrder
#include "diPlotCommand.h" // PlotCommand

#include <vector>

class PlotCluster
{
public:
  PlotCluster();
  virtual ~PlotCluster();

  virtual void cleanup();

  virtual const std::string& plotCommandKey() const = 0;

  virtual void prepare(const PlotCommand_cpv& cmds) = 0;

  virtual void setCanvas(DiCanvas* canvas);

  virtual void plot(DiGLPainter* gl, Plot::PlotOrder zorder);

  virtual void addAnnotations(std::vector<AnnotationPlot::Annotation>& annotations);

  virtual const std::string& keyPlotElement() const = 0;

  virtual void addPlotElements(std::vector<PlotElement>& pel);

  virtual bool enablePlotElement(const PlotElement& pe);

protected:
  Plot* at(size_t i);

protected:
  std::vector<Plot*> plots_; // vector of observation plots
  DiCanvas* canvas_;
};

#endif // PLOTCLUSTER_H
