
#ifndef DIPAINTER_H
#define DIPAINTER_H 1

#include "diPoint.h"

#include <QList>
#include <string>

class Colour;
class Linetype;
class Rectangle;

class QImage;
class QPolygonF;

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE
#endif

class DiCanvas {
public:
  virtual ~DiCanvas() { }

  enum FontFace {
    // same as glText.h
    F_NORMAL = 0, F_BOLD = 1, F_ITALIC = 2, F_BOLD_ITALIC = 3
  };

  virtual void setVpGlSize(float vpw, float vph, float glw, float glh) = 0;
  virtual bool setFont(const std::string& font) = 0;
  virtual bool setFont(const std::string& font, float size, FontFace face=F_NORMAL) = 0;
  bool setFont(const std::string& font, FontFace face, float size)
    { return setFont(font, size, face); }
  bool setFont(const std::string& font, const std::string& face, float size);
  virtual bool setFontSize(float size) = 0;
  virtual bool getCharSize(char ch, float& w, float& h) = 0;
  virtual bool getTextSize(const std::string& text, float& w, float& h) = 0;
};

class DiPainter {
protected:
  DiPainter(DiCanvas* canvas);

public:
  virtual ~DiPainter() { }

  DiCanvas* canvas()
    { return mCanvas; }

  void setVpGlSize(float vpw, float vph, float glw, float glh);
  bool setFont(const std::string& font);
  bool setFont(const std::string& font, float size, DiCanvas::FontFace face=DiCanvas::F_NORMAL);
  bool setFont(const std::string& font, DiCanvas::FontFace face, float size)
    { return setFont(font, size, face); }
  bool setFont(const std::string& font, const std::string& face, float size);
  bool setFontSize(float size);
  bool getCharSize(char ch, float& w, float& h);
  bool getTextSize(const std::string& text, float& w, float& h);

  virtual bool drawText(const std::string& text, float x, float y, float angle = 0) = 0;
  virtual bool drawChar(char chr, float x, float y, float angle = 0);

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
  virtual void drawRect(float x1, float y1, float x2, float y2) = 0;
  virtual void fillRect(float x1, float y1, float x2, float y2) = 0;
  virtual void drawCircle(float centerx, float centery, float radius) = 0;
  virtual void fillCircle(float centerx, float centery, float radius) = 0;

  virtual void drawRect(const Rectangle& r);
  virtual void fillRect(const Rectangle& r);

  virtual void drawCross(float x, float y, float dxy, bool diagonal = false);
  virtual void drawArrow(float x1, float y1, float x2, float y2, float headsize = 0);
  virtual void drawArrowHead(float x1, float y1, float x2, float y2, float headsize = 0);
  virtual void drawWindArrow(float u, float v, float x, float y,
      float arrowSize, bool withArrowHead) = 0;

  void drawReprojectedImage(const QImage& image, const float* mapPositionsXY, bool smooth);
  virtual void drawReprojectedImage(const QImage& image, const float* mapPositionsXY,
      const diutil::Rect_v& imageparts, bool smooth) = 0;

private:
  DiCanvas* mCanvas;
};

#endif // DIPAINTER_H
