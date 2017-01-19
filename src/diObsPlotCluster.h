#ifndef OBSPLOTCLUSTER_H
#define OBSPLOTCLUSTER_H

#include "diAnnotationPlot.h" // AnnotationPlot::Annotation
#include "diCommonTypes.h" // PlotElement
#include "diPlot.h" // Plot::PlotOrder

#include <vector>

class EditManager;
class ObsManager;
class ObsPlot;
class ObsPlotCollider;

class ObsPlotCluster
{
public:
  ObsPlotCluster(ObsManager* obsm, EditManager* editm);
  ~ObsPlotCluster();

  void cleanup();

  const std::string& plotCommandKey() const;

  void prepare(const std::vector<std::string>& cmds);

  void setCanvas(DiCanvas* canvas);

  //! returns true iff there are any obs plots with data
  bool update(bool ifNeeded, const miutil::miTime& t);

  void plot(DiGLPainter* gl, Plot::PlotOrder zorder);

  void getAnnotations(std::vector<AnnotationPlot::Annotation>& annotations);

  void getDataAnnotations(std::vector<std::string>& anno);

  void getExtraAnnotations(std::vector<AnnotationPlot*>& vap);

  std::vector<miutil::miTime> getTimes();

  void addPlotElements(std::vector<PlotElement>& pel);

  bool enablePlotElement(const PlotElement& pe);

  bool findObs(int x, int y);

  bool getObsName(int x, int y, std::string& name);

  std::string getObsPopupText(int x, int y);

  void nextObs(bool next);

  const std::vector<ObsPlot*>& getObsPlots() const
    { return plots_; }

private:
  std::vector<ObsPlot*> plots_; // vector of observation plots
  bool hasDevField_;
  std::auto_ptr<ObsPlotCollider> collider_;

  ObsManager* obsm_;
  EditManager* editm_;
  DiCanvas* canvas_;
};

#endif // OBSPLOTCLUSTER_H
