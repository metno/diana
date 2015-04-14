
#include "WebMapTile.h"

#include <QNetworkReply>

#include <QFont>
#include <QPainter>
#include <QTimer>

#define MILOGGER_CATEGORY "diana.WebMapWMS"
#include <miLogger/miLogging.h>

WebMapTile::WebMapTile(int column, int row, const Rectangle& rect)
  : mColumn(column)
  , mRow(row)
  , mRect(rect)
  , mReply(0)
{
}

WebMapTile::~WebMapTile()
{
  delete mReply;
}

void WebMapTile::submit(QNetworkReply* r)
{
  METLIBS_LOG_SCOPE();
  mReply = r;
  if (mReply) {
    METLIBS_LOG_DEBUG("waiting for " << mReply->url().toString().toStdString() << "'");
    connect(mReply, SIGNAL(finished()), this, SLOT(replyFinished()));
  } else {
    METLIBS_LOG_DEBUG("no QNetworkReply");
#if 0
    QTimer::singleShot(10, this, SLOT(replyFinished()));
#else
    replyFinished();
#endif
  }
}

void WebMapTile::abort()
{
  if (mReply)
    mReply->abort();
}

bool WebMapTile::loadImage(const char* format)
{
  METLIBS_LOG_SCOPE(LOGVAL(mColumn) << LOGVAL(mRow) << LOGVAL(format));
  if (mReply) {
    const QString ct = mReply->header(QNetworkRequest::ContentTypeHeader).toString();
    METLIBS_LOG_DEBUG("url=" << mReply->url().toString().toStdString() << "' Content-Type=" << ct.toStdString() << "'");
    bool ok = false;
    const char* fmt = 0;
    if (ct == "image/png")
      fmt = "PNG";
    else if (ct == "image/jpeg")
      fmt = "JPEG";
    if (fmt != 0) {
#if 0
      ok = mImage.load(mReply, fmt);
#else
      const QByteArray data = mReply->readAll();
      ok = mImage.loadFromData(data, fmt);
#endif
    }
    METLIBS_LOG_DEBUG(LOGVAL(mImage.width()) << LOGVAL(mImage.height()) << LOGVAL(ok));
  }
  return !mImage.isNull();
}

void WebMapTile::dummyImage(int tw, int th)
{
  mImage = QImage(tw, th, QImage::Format_ARGB32);
  mImage.fill(Qt::transparent);
  QPainter p(&mImage);
  const float fontScale = 3;
  QFont f = p.font();
  int fpx = f.pixelSize();
  if (fpx > 0)
    f.setPixelSize(fontScale*fpx);
    else
      f.setPointSizeF(fontScale*f.pointSizeF());
  p.setFont(f);
  p.fillRect(mImage.rect(), QBrush(QColor((mColumn&1) ? 80 : 255, (mRow&1) ? 80 : 255, 128, 128)));
  p.drawText(mImage.rect(), Qt::AlignLeft | Qt::AlignVCenter, QString("x=%1 y=%2").arg(mColumn).arg(mRow));
  p.end();
}

void WebMapTile::replyFinished()
{
  Q_EMIT finished(this);
}
