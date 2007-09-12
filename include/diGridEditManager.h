#ifndef DIGRIDEDITMANAGER_H_
#define DIGRIDEDITMANAGER_H_

#include <diMapMode.h>
#include <diGridAreaManager.h>
#include <diGridConverter.h>
#include <diFieldManager.h>
#include <fetCodeExecutor.h>
#include <diFieldPlot.h>
#include <fetGate.h>
#include <fetParameter.h>
#include <fetSession.h>
#include <fetModel.h>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

using namespace std;

class PlotModule;

class GridEditManager {

private:
#ifndef NOLOG4CXX
  log4cxx::LoggerPtr logger;
#endif

  struct EditField {
    miString name;
    vector<Field*> origField;
    map<miString,fetObject> object;
    FieldPlot *fp;
  };
  
  struct parameter_time {
    miString parameter;
    miTime time;
    miString pin;
  };

  vector<parameter_time> toPlot; //parameters/times to plot
  map<miString,Polygon > polygonMap; //all polygons to plot
  
  map<miString, map<miTime,EditField> > editMap;
 
  vector<fetBaseObject> baseObject;
  

  PlotModule* plotm; 
  FieldManager* fieldm;
  GridAreaManager areaManager;
  mapMode mapmode;
  GridConverter gc;   // gridconverter class

  //Session
  Area area;
  vector<miTime>       vtime;
  vector<fetParameter> profetParameter;
  fetSession           profetSession;
  fetModel             profetModel;
  fetGate  gate;
  bool firsttime;

  //current
  miString currentParameter;
  miTime   currentTime;
  fetObject rawObject;      // current object without GUI components
  fetObject currentObject;  // current object ready for execution
  bool autoexecute;

  fetCodeExecutor executor;
  vector<fetCodeExecutor::responce> responcel; //local ?
  vector<fetDynamicGui::GuiComponent> guicomponents;

public:
  GridEditManager(PlotModule*, FieldManager*);
  ~GridEditManager();
  GridAreaManager *getGridAreaManager();

  /// handle mouse event
  void sendMouseEvent(const mouseEvent& me, EventResult& res);
  /// handle keyboard event
  void sendKeyboardEvent(const keyboardEvent& me, EventResult& res);

  bool prepare(const miString& pin);
  bool setData(const miTime& time);
  bool plot(bool under);
  bool initSession(vector<miTime>& timeLabel,
		   vector<miString>& parameterLabel,
		   vector< vector<bool> >& mark,
		   miString& sessionHeader);
  miString getCurrentParameter(){return currentParameter;}
  miTime getCurrentTime(){return currentTime;}
  fetObject getCurrentObject(){return rawObject;}
  miString getObjectId(){return rawObject.id();}
  bool currentObjectOK(){return rawObject.exists();}
  void setCurrent(miString p, miTime t);
  void setCurrentObject(miString id);
  bool getFieldGroups(const miString& modelNameRequest, 
		      miString& modelName, 
		      vector<FieldGroupInfo>& vfgi);
  bool isEditField(const vector<FieldTimeRequest>& request);
  bool getPlotElements(vector<PlotElement>& pel);
  bool enablePlot(const PlotElement& pe);

  /// return available times for the requested models and fields
  vector<miTime> getFieldTime(const vector<FieldTimeRequest>& request,
			      bool allTimeSteps);
  void clear(){  toPlot.clear();}

  void outputExecuteResponce(vector<fetCodeExecutor::responce> & rl);
  void setObject(const fetObject& fobj);
  vector<fetDynamicGui::GuiComponent> setObject(const fetBaseObject& fobj,
						bool onlygui=false);
  void prepareObject(const vector<fetDynamicGui::GuiComponent>& components);
  bool execute(bool apply=false);

  vector<fetBaseObject> getBaseObjects();
  map<miString,fetObject> getObjects();

  bool isAreaSelected(){return areaManager.isAreaSelected();}
  miString areaId(){return areaManager.getCurrentId();}

  bool changeArea();
  void addArea(bool newArea);

  void setArea(miString id);
  bool autoExecute(){return autoexecute;}
  void setAutoExecute(bool on){autoexecute=on;}
  void deleteObject();

  void refresh();

  void quit();

};

#endif /*DIGRIDEDITMANAGER_H_*/
