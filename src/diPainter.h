
#ifndef DIPAINTER_H
#define DIPAINTER_H 1

#include "diPoint.h"

#include <QList>
#include <QPointF>
#include <qglobal.h>

#include <map>
#include <string>

class Colour;
class Linetype;
class Rectangle;

class QImage;
class QPolygonF;
class QString;

class DiCanvas {
public:
  DiCanvas();
  virtual ~DiCanvas() { }

  enum FontFace {
    // same as glText.h
    F_NORMAL = 0, F_BOLD = 1, F_ITALIC = 2, F_BOLD_ITALIC = 3
  };

  void parseFontSetup();
  virtual void parseFontSetup(const std::vector<std::string>& sect_fonts);
  virtual void defineFont(const std::string& font, const std::string& fontfilename,
      const std::string& face, bool use_bitmap) = 0;

  virtual void setVpGlSize(int vpw, int vph, float glw, float glh) = 0;

  virtual bool setFont(const std::string& font) = 0;
  virtual bool setFont(const std::string& font, float size, FontFace face=F_NORMAL) = 0;

  bool setFont(const std::string& font, FontFace face, float size)
    { return setFont(font, size, face); }
  bool setFont(const std::string& font, const std::string& face, float size);

  virtual bool setFontSize(float size) = 0;

  bool getCharSize(int ch, float& w, float& h);
  bool getTextSize(const char* text, float& w, float& h);
  bool getTextSize(const std::string& text, float& w, float& h);
  bool getTextRect(const char* s, float& x, float& y, float& w, float& h);
  bool getTextRect(const std::string& s, float& x, float& y, float& w, float& h);
  bool getTextSize(const QString& text, float& w, float& h);
  virtual bool getTextRect(const QString& text, float& x, float& y, float& w, float& h) = 0;

  bool isPrinting() const
    { return mPrinting; }

protected:
  std::string lookupFontAlias(const std::string& name);

protected:
  std::map<std::string, std::string> fontFamilyAliases;
  bool mPrinting;
};

class DiPainter {
protected:
  DiPainter(DiCanvas* canvas);

public:
  virtual ~DiPainter() { }

  DiCanvas* canvas()
    { return mCanvas; }

  bool isPrinting() const
    { return mCanvas->isPrinting(); }

  void setVpGlSize(int vpw, int vph, float glw, float glh);

  bool setFont(const std::string& font);
  bool setFont(const std::string& font, float size, DiCanvas::FontFace face=DiCanvas::F_NORMAL);
  bool setFont(const std::string& font, DiCanvas::FontFace face, float size)
    { return setFont(font, size, face); }
  bool setFont(const std::string& font, const std::string& face, float size);

  bool setFontSize(float size);

  bool getTextSize(const QString& text, float& w, float& h);
  bool getTextSize(const std::string& text, float& w, float& h);
  bool getTextSize(const char* text, float& w, float& h);
  bool getCharSize(int ch, float& w, float& h);

  virtual bool drawText(const QString& text, const QPointF& xy, float angle = 0) = 0;
  bool drawText(const QString& text, float x, float y, float angle = 0);
  bool drawText(const std::string& text, float x, float y, float angle = 0);
  bool drawText(const char* text, float x, float y, float angle = 0);
  bool drawChar(int chr, float x, float y, float angle = 0);

  // ========================================
  // higher level functions

  virtual void clear(const Colour& colour) = 0;
  virtual void setColour(const Colour& c, bool alpha = true) = 0;
  virtual void setLineStyle(const Colour& c, float lw=1, bool alpha = true) = 0;
  virtual void setLineStyle(const Colour& c, float lw, const Linetype& lt, bool alpha = true) = 0;

  virtual void drawLine(float x1, float y1, float x2, float y2) = 0;
  virtual void drawPolyline(const QPolygonF& points) = 0;
  /* draw a series of filled quads */
  virtual void fillQuadStrip(const QPolygonF& points) = 0;
  virtual void drawPolygon(const QPolygonF& points) = 0;
  virtual void drawPolygons(const QList<QPolygonF>& polygons) = 0;
  virtual void drawRect(bool fill, float x1, float y1, float x2, float y2) = 0;
  virtual void drawRect(bool fill, const Rectangle& r);
  virtual void drawCircle(bool fill, float centerx, float centery, float radius) = 0;
  virtual void drawTriangle(bool fill, const QPointF& p1, const QPointF& p2, const QPointF& p3) = 0;

  virtual void drawCross(float x, float y, float dxy, bool diagonal = false);
  virtual void drawArrow(float x1, float y1, float x2, float y2, float headsize = 0);
  virtual void drawArrowHead(float x1, float y1, float x2, float y2, float headsize = 0);
  virtual void drawWindArrow(float u, float v, float x, float y,
      float arrowSize, bool withArrowHead, int turnBarbs=1) = 0;

  virtual void drawScreenImage(const QPointF& point, const QImage& image) = 0;

private:
  DiCanvas* mCanvas;
};

#endif // DIPAINTER_H
