#ifndef FIELDPLOTCLUSTER_H
#define FIELDPLOTCLUSTER_H

#include "diPlotCluster.h"
#include "diAnnotationPlot.h" // AnnotationPlot::Annotation

class FieldManager;
class FieldPlot;
class FieldPlotManager;

class FieldPlotCluster : public PlotCluster
{
public:
  FieldPlotCluster(FieldManager* fieldm, FieldPlotManager* fieldplotm);
  ~FieldPlotCluster();

  const std::string& plotCommandKey() const override;

  void prepare(const PlotCommand_cpv& cmds) override;

  //! returns true iff there are fields with data
  bool update();

  void getDataAnnotations(std::vector<std::string>& anno) const;

  std::vector<miutil::miTime> getTimes();

  const std::string& keyPlotElement() const override;

  std::vector<miutil::miTime> fieldAnalysisTimes() const;

  int getVerticalLevel() const;

  //! set area equal to first EXISTING field-area ("all timesteps"...)
  bool getRealFieldArea(Area&) const;

  bool MapToGrid(const Projection& plotproj, float xmap, float ymap, float& gridx, float& gridy) const;

  miutil::miTime getFieldReferenceTime() const;

  std::vector<std::string> getTrajectoryFields();

  const FieldPlot* findTrajectoryPlot(const std::string& fieldname);

  std::vector<FieldPlot*> getFieldPlots() const;

protected:
  FieldPlot* at(size_t i) const;

private:
  FieldManager* fieldm_;
  FieldPlotManager* fieldplotm_;
};

#endif // FIELDPLOTCLUSTER_H
