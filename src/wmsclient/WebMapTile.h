
#ifndef WebMapTile_h
#define WebMapTile_h 1

#include <diField/diRectangle.h>

#include <QImage>
#include <QObject>

class QNetworkReply;


class WebMapImage : public QObject {
  Q_OBJECT;

public:
  WebMapImage();

  ~WebMapImage();

  const QImage& image() const
    { return mImage; }

  void submit(QNetworkReply* reply);

  void abort();

  bool loadImage(const char* format);
  bool loadImage(const std::string& format)
    { return loadImage(format.c_str()); }

  const QNetworkReply* reply() const
    { return mReply; }

protected Q_SLOTS:
  virtual void replyFinished();

Q_SIGNALS:
  void finishedImage(WebMapImage* self);

private:
  void dropRequest();

protected:
  QImage mImage;
  QNetworkReply* mReply;
};

// ========================================================================

class WebMapTile : public WebMapImage {
  Q_OBJECT;

public:
  WebMapTile(int column, int row, const Rectangle& rect);

  ~WebMapTile();

  int column() const
    { return mColumn; }
  int row() const
    { return mRow; }

  const Rectangle& rect() const
    { return mRect; }

  void dummyImage(int tw, int th);

protected /*Q_SLOTS*/:
  void replyFinished() /* Q_DECL_OVERRIDE */;

Q_SIGNALS:
  void finished(WebMapTile* self);

protected:
  int mColumn;
  int mRow;
  Rectangle mRect;
};

#endif // WebMapTile_h
