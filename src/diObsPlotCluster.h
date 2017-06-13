#ifndef OBSPLOTCLUSTER_H
#define OBSPLOTCLUSTER_H

#include "diPlotCluster.h"

class EditManager;
class ObsManager;
class ObsPlot;
struct ObsPlotCollider;

class ObsPlotCluster : public PlotCluster
{
public:
  ObsPlotCluster(ObsManager* obsm, EditManager* editm);
  ~ObsPlotCluster();

  const std::string& plotCommandKey() const override;

  void prepare(const PlotCommand_cpv& cmds) override;

  //! returns true iff there are any obs plots with data
  bool update(bool ifNeeded, const miutil::miTime& t);

  void plot(DiGLPainter* gl, Plot::PlotOrder zorder) override;

  void getDataAnnotations(std::vector<std::string>& anno) const;

  void getExtraAnnotations(std::vector<AnnotationPlot*>& vap);

  std::vector<miutil::miTime> getTimes();

  const std::string& keyPlotElement() const override;

  bool findObs(int x, int y);

  bool getObsName(int x, int y, std::string& name);

  std::string getObsPopupText(int x, int y);

  void nextObs(bool next);

  std::vector<ObsPlot*> getObsPlots() const;

protected:
  ObsPlot* at(size_t i) const;

private:
  bool hasDevField_;
  std::unique_ptr<ObsPlotCollider> collider_;

  ObsManager* obsm_;
  EditManager* editm_;
};

#endif // OBSPLOTCLUSTER_H
