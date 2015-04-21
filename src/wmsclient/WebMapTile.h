
#ifndef WebMapTile_h
#define WebMapTile_h 1

#include <diField/diRectangle.h>

#include <QImage>
#include <QObject>

class QNetworkReply;


class WebMapTile : public QObject {
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

  const QImage& image() const
    { return mImage; }

  void submit(QNetworkReply* reply);

  void abort();

  bool loadImage(const char* format);
  bool loadImage(const std::string& format)
    { return loadImage(format.c_str()); }
  void dummyImage(int tw, int th);

  const QNetworkReply* reply() const
    { return mReply; }

private Q_SLOTS:
  void replyFinished();

Q_SIGNALS:
  void finished(WebMapTile* self);

private:
  void dropRequest();

protected:
  int mColumn;
  int mRow;
  Rectangle mRect;
  QImage mImage;
  QNetworkReply* mReply;
};

#endif // WebMapTile_h
