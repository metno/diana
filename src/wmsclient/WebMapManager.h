
#ifndef WebMapManager_h
#define WebMapManager_h 1

#include "diManager.h"

#include <vector>

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE
#endif

class QNetworkAccessManager;
class WebMapService;
class WebMapPlot;

class WebMapManager : public Manager {
  Q_OBJECT;

private:
  WebMapManager();

public:
  ~WebMapManager();

  bool parseSetup() Q_DECL_OVERRIDE;

  std::vector<miutil::miTime> getTimes() const Q_DECL_OVERRIDE
    { return std::vector<miutil::miTime>(); }

  bool changeProjection(const Area&) Q_DECL_OVERRIDE;

  bool prepare(const miutil::miTime&) Q_DECL_OVERRIDE;

  void plot(bool, bool) Q_DECL_OVERRIDE
    { }

  void plot(Plot::PlotOrder zorder) Q_DECL_OVERRIDE;

  std::vector<PlotElement> getPlotElements() const Q_DECL_OVERRIDE;

  bool processInput(const std::vector<std::string>&) Q_DECL_OVERRIDE;

  void sendMouseEvent(QMouseEvent*, EventResult&) Q_DECL_OVERRIDE
    { }

  void sendKeyboardEvent(QKeyEvent*, EventResult&) Q_DECL_OVERRIDE
    { }

  std::vector<std::string> getAnnotations() const Q_DECL_OVERRIDE;

Q_SIGNALS:
  void webMapsReady();

private:
  WebMapPlot* createPlot(const std::string& qmstring);

private:
  QNetworkAccessManager* network;
  std::vector<WebMapService*> webmapservices;
  std::vector<WebMapPlot*> webmaps;

public:
  static WebMapManager* instance();

private:
  static WebMapManager* self;
};

#endif // WebMapManager_h
