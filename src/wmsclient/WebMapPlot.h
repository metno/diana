
#ifndef WebMapPlot_h
#define WebMapPlot_h 1

#include "diPlot.h"

#include <QObject>

class WebMapLayer;
class WebMapRequest;
class WebMapService;

namespace diutil {
class SimpleColourTransform;
} // namespace diutil

class WebMapPlot : public QObject, public Plot
{
  Q_OBJECT;

public:
  WebMapPlot(WebMapService* service, const std::string& layer);
  ~WebMapPlot();

  std::string title() const;

  std::string attribution() const;

  /* set time tolerance in seconds */
  void setTimeTolerance(int tolerance)
    { mTimeTolerance = tolerance; }

  /* set time offset in seconds */
  void setTimeOffset(int offset)
    { mTimeOffset = offset; }

  void setCRS(const std::string& crs)
    { mCRS = crs; }

  void setStyleAlpha(float offset, float scale);

  void setStyleGrey(bool makeGrey);

  void plot(PlotOrder porder);

  void changeProjection();

  size_t countDimensions() const;

  int findDimensionByIdentifier(const std::string& dimId) const;

  const std::string& dimensionTitle(size_t idx) const;

  const std::vector<std::string>& dimensionValues(size_t idx) const;

  /*! index of time dimension; < 0 if no time dimension */
  int timeDimension() const
    { return mTimeIndex; }

  void setDimensionValue(size_t idx, const std::string& dimValue);

  /*! selet time; if invalid or not found, use default time; ignored
   *  if no time dimension */
  void setTimeValue(const miutil::miTime& time);

  /*! set to a fixed time; empty == no fixed time */
  void setFixedTime(const std::string& time);

Q_SIGNALS:
  void update();

private Q_SLOTS:
  void serviceRefreshStarting();
  void serviceRefreshFinished();
  void requestCompleted();

private:
  void dropRequest();

private:
  WebMapService* mService;
  std::string mLayerId;
  const WebMapLayer* mLayer;

  int mTimeIndex;
  int mTimeSelected;
  int mTimeTolerance; // time tolerance in seconds
  int mTimeOffset; // time offset in seconds
  std::string mFixedTime;

  std::string mCRS;
  diutil::SimpleColourTransform* mColourTransform;

  std::map<std::string, std::string> mDimensionValues;

  WebMapRequest* mRequest;
  bool mRequestCompleted;

  Area mOldArea;
};

#endif // WebMapPlot_h
