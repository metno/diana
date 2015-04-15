
#ifndef WebMapWMS_h
#define WebMapWMS_h 1

#include "WebMapService.h"
#include "WebMapTile.h"

#include <diField/diProjection.h>

#include <QImage>
#include <QUrl>

class WebMapTile;

class QDomElement;
class QNetworkAccessManager;
class QNetworkReply;

struct WebMapWmsCrsBoundingBox {
  std::string crs;
  Projection projection;
  double metersPerUnit;
  Rectangle boundingbox;
  WebMapWmsCrsBoundingBox(const std::string& c, const Rectangle& bb);
};

typedef std::vector<WebMapWmsCrsBoundingBox> WebMapWmsCrsBoundingBox_v;

// ========================================================================

class WebMapWMSLayer : public WebMapLayer {
public:

public:
  WebMapWMSLayer(const std::string& identifier);

  void setDimensions(const WebMapDimension_v& dimensions)
    { mDimensions = dimensions; }

  void setDefaultStyle(const std::string& styleId)
    { mDefaultStyle = styleId; }

  void setZoomRange(int minZoom, int maxZoom)
    { mMinZoom = minZoom; mMaxZoom = maxZoom; }

  void addCRS(const std::string& crs, const Rectangle& bbox)
    { mCRS.push_back(WebMapWmsCrsBoundingBox(crs, bbox)); }

  size_t countCRS() const
    { return mCRS.size(); }

  const std::string& CRS(size_t idx) const
    { return crsBoundingBox(idx).crs; }

  const WebMapWmsCrsBoundingBox& crsBoundingBox(size_t idx) const
    { return mCRS.at(idx); }

  int minZoom() const
    { return mMinZoom; }

  int maxZoom() const
    { return mMaxZoom; }

  const std::string& defaultStyle() const
    { return mDefaultStyle; }

private:
  WebMapWmsCrsBoundingBox_v mCRS;
  int mMinZoom;
  int mMaxZoom;
  std::string mDefaultStyle;
};

typedef WebMapWMSLayer* WebMapWMSLayer_x;
typedef const WebMapWMSLayer* WebMapWMSLayer_cx;

// ========================================================================

class WebMapWMS;
typedef WebMapWMS* WebMapWMS_x;
typedef const WebMapWMS* WebMapWMS_cx;

class WebMapWMSRequest : public WebMapRequest {
  Q_OBJECT;

public:
  WebMapWMSRequest(WebMapWMS_x service, WebMapWMSLayer_cx layer, int crsIndex, int zoom);

  ~WebMapWMSRequest();

  void addTile(int tileX, int tileY);

  void setDimensionValue(const std::string& dimName, const std::string& dimValue);

  /*! start fetching data */
  void submit();

  /*! stop fetching data */
  void abort();

  /*! number of tiles */
  size_t countTiles() const
    { return mTiles.size(); }

  /*! rectangle of one tile, in tileProjection coordinates */
  const Rectangle& tileRect(size_t idx) const;

  /*! image data of one tile; might have isNull() == true */
  const QImage& tileImage(size_t idx) const;

  const Projection& tileProjection() const
    { return mLayer->crsBoundingBox(mCrsIndex).projection; }

private Q_SLOTS:
  void tileFinished(WebMapTile*);

private:
  WebMapWMS_x mService;
  WebMapWMSLayer_cx mLayer;
  int mCrsIndex;
  int mZoom;
  std::map<std::string, std::string> mDimensionValues;

  std::vector<WebMapTile*> mTiles;
  size_t mUnfinished;
};

typedef boost::shared_ptr<WebMapRequest> WebMapRequest_p;

// ========================================================================

class WebMapWMS : public WebMapService
{
  Q_OBJECT;

public:
  /* set with URL pointing to GetCapabilities. need to call refresh to
   * actually fetch data */
  WebMapWMS(const std::string& identifier, const QUrl& service, QNetworkAccessManager* network);

  ~WebMapWMS();

  /*! suitable refresh interval. negative for no refresh */
  int refreshInterval() const;

  /*! create a request object for the specified layer. may be null,
   *  e.g. if unknown layer or; ownership is transferred to caller */
  WebMapRequest_x createRequest(const std::string& layer,
      const Rectangle& viewRect, const Projection& viewProj, double viewScale);

  QNetworkReply* submitRequest(WebMapWMSLayer_cx layer,
      const std::map<std::string, std::string>& dimensionValues,
      const std::string& crs, WebMapTile* tile);

  QNetworkReply* submitUrl(const QUrl& url);

  void refresh();

  const std::string& tileFormat() const
    { return mTileFormat; }

private:
  typedef std::map<std::string, Rectangle> crs_bbox_m;
  enum WmsVersion { WMS_111, WMS_130 };

  bool parseReply();
  bool parseLayer(QDomElement& eLayer, std::string style, crs_bbox_m crs_bboxes,
      WebMapDimension_v dimensions);
  void parseDimensionValues(WebMapDimension& dim, const QString& text, const QString& defaultValue);

private Q_SLOTS:
  void refreshReplyFinished();

private:
  QUrl mServiceURL;
  std::string mTileFormat;
  WmsVersion mVersion;

  QNetworkAccessManager* mNetworkAccess;
  long mNextRefresh;
  QNetworkReply* mRefeshReply;
};

#endif // WebMapWMS_h
