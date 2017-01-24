#ifndef AREAOBJECTSCLUSTER_H
#define AREAOBJECTSCLUSTER_H

#include "diAreaObjects.h"
#include "diPlot.h"

class PlotModule;
class StaticPlot;

class AreaObjectsCluster
{
public:
  AreaObjectsCluster(PlotModule* pm);
  ~AreaObjectsCluster();

  void plot(DiGLPainter* gl, Plot::PlotOrder zorder);

  void addPlotElements(std::vector<PlotElement>& pel);

  bool enablePlotElement(const PlotElement& pe);

  ///put area into list of area objects
  void makeAreaObjects(std::string name, std::string areastring, int id);

  ///send command to right area object
  void areaObjectsCommand(const std::string& command, const std::string& dataSet,
      const std::vector<std::string>& data, int id);

  ///find areas in position x,y
  std::vector<selectArea> findAreaObjects(int x, int y, bool newArea = false);

private:
  PlotModule* plot_;

  typedef std::vector<AreaObjects> areaobjects_v;
  areaobjects_v vareaobjects;  //QED areas
};

#endif // AREAOBJECTSCLUSTER_H
