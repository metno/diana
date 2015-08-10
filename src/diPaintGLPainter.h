#ifndef PAINTGLPAINTER_H
#define PAINTGLPAINTER_H

#include "diGLPainter.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPointF>
#include <QStack>
#include <QTransform>
#include <QVector>
#include <QWidget>

#include <map>
#include <set>

class QPaintDevice;

struct PaintAttributes {
  QRgb color;
  float width;
  QHash<DiGLPainter::GLenum,DiGLPainter::GLenum> polygonMode;
  QVector<qreal> dashes;
  QImage mask;
  qreal dashOffset;
  bool lineStipple;
  bool polygonStipple;
  bool antialiasing;
  QColor bias;
  bool biased;
  QColor scale;
  bool scaled;
  QPointF pixelZoom;
};

struct StencilAttributes {
  DiGLPainter::GLint clear;
  DiGLPainter::GLenum func;
  DiGLPainter::GLint ref;
  DiGLPainter::GLuint mask;
  DiGLPainter::GLenum fail;
  DiGLPainter::GLenum zfail;
  DiGLPainter::GLenum zpass;
  QPainterPath path;

  bool clip;
  bool update;
  bool enabled;
};

struct RenderItem {
  DiGLPainter::GLenum mode;
  QPainter *painter;
  QTransform transform;
  DiGLPainter::GLuint list;
};

class DiPaintGLCanvas : public DiGLCanvas
{
public:
  enum FontScaling {
    S_FIXEDSIZE, S_VIEWPORTSCALED
  };
  enum {
    MAXFONTFACES = 4, MAXFONTS = 10, MAXFONTSIZES = 40
  };

  DiPaintGLCanvas(QPaintDevice* device);
  ~DiPaintGLCanvas();

  void setVpGlSize(float vpw, float vph, float glw, float glh) Q_DECL_OVERRIDE;
  bool setFont(const std::string& font, float size, FontFace face=F_NORMAL) Q_DECL_OVERRIDE;
  bool setFont(const std::string& font) Q_DECL_OVERRIDE;
  bool setFontFace(FontFace face);
  bool setFontSize(float size) Q_DECL_OVERRIDE;
  bool getCharSize(char ch, float& w, float& h) Q_DECL_OVERRIDE;
  bool getTextSize(const std::string& text, float& w, float& h) Q_DECL_OVERRIDE;

  QImage convertToGLFormat(const QImage& i) Q_DECL_OVERRIDE;

  float fontScaleX() const
    { return mFontScaleX; }

  float fontScaleY() const
    { return mFontScaleY; }

  const QFont& font() const
    { return mFont; }

  QPaintDevice* device() const
    { return mDevice; }

private:
  bool parseFontSetup();
  bool defineFont(const std::string& font, const std::string& fontfilename,
      FontFace, int, const std::string& = "", float = 1.0f, float = 1.0f);

private:
  QPaintDevice* mDevice;
  QFont mFont;

  float mFontScaleX, mFontScaleY;
  QHash<QString,QString> fontMap;

  std::map<std::string, std::set<std::string> > enginefamilies;
  std::map<std::string, std::string> defaults;
};

class DiPaintGLPainter : public DiGLPainter
{
public:
  DiPaintGLPainter(DiPaintGLCanvas* canvas);
  ~DiPaintGLPainter();

  // begin DiGLPainter interface
  void Begin(GLenum mode) Q_DECL_OVERRIDE;
  void Color3d(GLdouble red, GLdouble green, GLdouble blue) Q_DECL_OVERRIDE;
  void Color3f(GLfloat red, GLfloat green, GLfloat blue) Q_DECL_OVERRIDE;
  void Color3fv(const GLfloat *v) Q_DECL_OVERRIDE;
  void Color3ub(GLubyte red, GLubyte green, GLubyte blue) Q_DECL_OVERRIDE;
  void Color3ubv(const GLubyte *v) Q_DECL_OVERRIDE;
  void Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) Q_DECL_OVERRIDE;
  void Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) Q_DECL_OVERRIDE;
  void Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) Q_DECL_OVERRIDE;
  void Color4fv(const GLfloat *v) Q_DECL_OVERRIDE;
  void Color4ubv(const GLubyte *v) Q_DECL_OVERRIDE;
  void End() Q_DECL_OVERRIDE;
  void Indexi(GLint c) Q_DECL_OVERRIDE;
  void RasterPos2f(GLfloat x, GLfloat y) Q_DECL_OVERRIDE;
  void Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) Q_DECL_OVERRIDE;
  void TexCoord2f(GLfloat s, GLfloat t) Q_DECL_OVERRIDE;
  void Vertex2dv(const GLdouble *v) Q_DECL_OVERRIDE;
  void Vertex2f(GLfloat x, GLfloat y) Q_DECL_OVERRIDE;
  void Vertex2i(GLint x, GLint y) Q_DECL_OVERRIDE;
  void Vertex3f(GLfloat x, GLfloat y, GLfloat z) Q_DECL_OVERRIDE;
  void Vertex3i(GLint x, GLint y, GLint z) Q_DECL_OVERRIDE;
  void DrawArrays(GLenum mode, GLint first, GLsizei count) Q_DECL_OVERRIDE;
  void VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) Q_DECL_OVERRIDE;
  void BlendFunc(GLenum sfactor, GLenum dfactor) Q_DECL_OVERRIDE;
  void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) Q_DECL_OVERRIDE;
  void Clear(GLbitfield mask) Q_DECL_OVERRIDE;
  void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) Q_DECL_OVERRIDE;
  void Disable(GLenum cap) Q_DECL_OVERRIDE;
  void DisableClientState(GLenum cap) Q_DECL_OVERRIDE; /* 1.1 */
  void DrawBuffer(GLenum mode) Q_DECL_OVERRIDE;
  void EdgeFlag(GLboolean flag) Q_DECL_OVERRIDE;
  void Enable(GLenum cap) Q_DECL_OVERRIDE;
  void EnableClientState(GLenum cap) Q_DECL_OVERRIDE; /* 1.1 */
  void Flush(void) Q_DECL_OVERRIDE;
  void GetFloatv(GLenum pname, GLfloat *params) Q_DECL_OVERRIDE;
  void GetIntegerv(GLenum pname, GLint *params) Q_DECL_OVERRIDE;
  GLboolean IsEnabled(GLenum cap) Q_DECL_OVERRIDE;
  void LineStipple(GLint factor, GLushort pattern) Q_DECL_OVERRIDE;
  void LineWidth(GLfloat width) Q_DECL_OVERRIDE;
  void PointSize(GLfloat size) Q_DECL_OVERRIDE;
  void PolygonMode(GLenum face, GLenum mode) Q_DECL_OVERRIDE;
  void PolygonStipple(const GLubyte *mask) Q_DECL_OVERRIDE;
  void PopAttrib(void) Q_DECL_OVERRIDE;
  void PushAttrib(GLbitfield mask) Q_DECL_OVERRIDE;
  void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
      GLdouble near_val, GLdouble far_val) Q_DECL_OVERRIDE;
  void LoadIdentity(void) Q_DECL_OVERRIDE;
  void PopMatrix(void) Q_DECL_OVERRIDE;
  void PushMatrix(void) Q_DECL_OVERRIDE;
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) Q_DECL_OVERRIDE;
  void Scalef(GLfloat x, GLfloat y, GLfloat z) Q_DECL_OVERRIDE;
  void Translatef(GLfloat x, GLfloat y, GLfloat z) Q_DECL_OVERRIDE;
  void Viewport(GLint x, GLint y, GLsizei width, GLsizei height) Q_DECL_OVERRIDE;
  void ShadeModel(GLenum mode) Q_DECL_OVERRIDE;
  void Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
      GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) Q_DECL_OVERRIDE;
  void DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
      const GLvoid *pixels) Q_DECL_OVERRIDE;
  void PixelStorei(GLenum pname, GLint param) Q_DECL_OVERRIDE;
  void PixelTransferf(GLenum pname, GLfloat param) Q_DECL_OVERRIDE;
  void PixelZoom(GLfloat xfactor, GLfloat yfactor) Q_DECL_OVERRIDE;
  void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      GLenum format, GLenum type, GLvoid *pixels) Q_DECL_OVERRIDE;
  void DepthMask(GLboolean flag) Q_DECL_OVERRIDE;
  void ClearStencil(GLint s) Q_DECL_OVERRIDE;
  void StencilFunc(GLenum func, GLint ref, GLuint mask) Q_DECL_OVERRIDE;
  void StencilOp(GLenum fail, GLenum zfail, GLenum zpass) Q_DECL_OVERRIDE;
  void BindTexture(GLenum target, GLuint texture) Q_DECL_OVERRIDE;
  void DeleteTextures(GLsizei n, const GLuint *textures) Q_DECL_OVERRIDE;
  void GenTextures(GLsizei n, GLuint *textures) Q_DECL_OVERRIDE;
  void TexEnvf(GLenum target, GLenum pname, GLfloat param) Q_DECL_OVERRIDE;
  void TexParameteri(GLenum target, GLenum pname, GLint param) Q_DECL_OVERRIDE;
  void TexImage2D(GLenum target, GLint level, GLint internalFormat,
      GLsizei width, GLsizei height, GLint border,
      GLenum format, GLenum type, const GLvoid *pixels) Q_DECL_OVERRIDE;
  bool supportsReadPixels() const
    { return false; }
  // end DiGLPainter interface

  // begin DiPainter interface
  bool drawText(const std::string& text, float x, float y, float angle = 0) Q_DECL_OVERRIDE;
  void drawCircle(float centerx, float centery, float radius) Q_DECL_OVERRIDE;
  void fillCircle(float centerx, float centery, float radius) Q_DECL_OVERRIDE;
  void drawRect(float x1, float y1, float x2, float y2) Q_DECL_OVERRIDE;
  void fillRect(float x1, float y1, float x2, float y2) Q_DECL_OVERRIDE;
  void drawLine(float x1, float y1, float x2, float y2) Q_DECL_OVERRIDE;
  void drawPolyline(const QPolygonF& points) Q_DECL_OVERRIDE;
  void drawPolygon(const QPolygonF& points) Q_DECL_OVERRIDE;
  void drawPolygons(const QList<QPolygonF>& polygons) Q_DECL_OVERRIDE;
  void drawReprojectedImage(const QImage& image, const float* mapPositionsXY,
      const diutil::Rect_v& imageparts, bool smooth) Q_DECL_OVERRIDE;
  // end DiPainter interface

  void begin(QPainter *painter);
  bool isPainting() const;
  void end();

  void renderPrimitive();
  void setViewportTransform();

  void setClipPath();
  void unsetClipPath();

  GLuint bindTexture(const QImage &image);
  void drawTexture(const QPointF &pos, GLuint texture);

  QPainter *painter;

  QStack<GLenum> stack;
  QStack<RenderItem> renderStack;
  QStack<QTransform> transformStack;
  QStack<PaintAttributes> attributesStack;

  bool blend;
  QPainter::CompositionMode blendMode;
  bool smooth;

  bool useTexture;
  GLuint currentTexture;

  QPointF rasterPos;
  QHash<GLenum,GLint> pixelStore;
  QPointF bitmapMove;

  GLfloat pointSize;
  QColor clearColor;
  bool clear;
  bool colorMask;

  GLenum mode;
  PaintAttributes attributes;
  StencilAttributes stencil;
  QPolygonF points;
  QList<bool> validPoints;
  QList<QRgb> colors;
  QTransform transform;

  QHash<GLuint,QImage> textures;
  QHash<qint64,GLuint> textureCache;

  GLuint clientState;

  GLint vertexSize;
  GLenum vertexType;
  GLsizei vertexStride;
  const GLvoid *vertexPointer;

  QRect viewport;
  QRectF window;

  bool printing;

private:
  void plotSubdivided(const QPointF quad[], const QRgb color[], int divisions = 0);
  void setPen();
  void setPolygonColor(const QRgb &color);
  void makeCurrent();
  void paintCircle(float centerx, float centery, float radius);
  void paintRect(float x1, float y1, float x2, float y2);

  void drawReprojectedSubImage(const QImage& image, const QPolygonF& mapPositions,
      const diutil::Rect& imagepart);
};

#endif // PAINTGLPAINTER_H
