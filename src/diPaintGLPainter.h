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

  void setVpGlSize(float vpw, float vph, float glw, float glh);
  bool setFont(const std::string& font, float size, FontFace face=F_NORMAL);
  bool setFont(const std::string& font);
  bool setFontFace(FontFace face);
  bool setFontSize(float size);
  bool getCharSize(char ch, float& w, float& h);
  bool getTextSize(const std::string& text, float& w, float& h);

  QImage convertToGLFormat(const QImage& i);

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
  void Begin(GLenum mode);
  void Color3d(GLdouble red, GLdouble green, GLdouble blue);
  void Color3f(GLfloat red, GLfloat green, GLfloat blue);
  void Color3fv(const GLfloat *v);
  void Color3ub(GLubyte red, GLubyte green, GLubyte blue);
  void Color3ubv(const GLubyte *v);
  void Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
  void Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
  void Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
  void Color4fv(const GLfloat *v);
  void Color4ubv(const GLubyte *v);
  void End();
  void Indexi(GLint c);
  void RasterPos2f(GLfloat x, GLfloat y);
  void Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
  void TexCoord2f(GLfloat s, GLfloat t);
  void Vertex2dv(const GLdouble *v);
  void Vertex2f(GLfloat x, GLfloat y);
  void Vertex2i(GLint x, GLint y);
  void Vertex3f(GLfloat x, GLfloat y, GLfloat z);
  void Vertex3i(GLint x, GLint y, GLint z);
  void DrawArrays(GLenum mode, GLint first, GLsizei count);
  void VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
  void BlendFunc(GLenum sfactor, GLenum dfactor);
  void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
  void Clear(GLbitfield mask);
  void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
  void Disable(GLenum cap);
  void DisableClientState(GLenum cap); /* 1.1 */
  void DrawBuffer(GLenum mode);
  void EdgeFlag(GLboolean flag);
  void Enable(GLenum cap);
  void EnableClientState(GLenum cap); /* 1.1 */
  void Flush(void);
  void GetFloatv(GLenum pname, GLfloat *params);
  void GetIntegerv(GLenum pname, GLint *params);
  GLboolean IsEnabled(GLenum cap);
  void LineStipple(GLint factor, GLushort pattern);
  void LineWidth(GLfloat width);
  void PointSize(GLfloat size);
  void PolygonMode(GLenum face, GLenum mode);
  void PolygonStipple(const GLubyte *mask);
  void PopAttrib(void);
  void PushAttrib(GLbitfield mask);
  void Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
  void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
      GLdouble near_val, GLdouble far_val);
  void LoadIdentity(void);
  void PopMatrix(void);
  void PushMatrix(void);
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
  void Scalef(GLfloat x, GLfloat y, GLfloat z);
  void Translatef(GLfloat x, GLfloat y, GLfloat z);
  void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
  void ShadeModel(GLenum mode);
  void Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
      GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
  void DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
      const GLvoid *pixels);
  void PixelStorei(GLenum pname, GLint param);
  void PixelTransferf(GLenum pname, GLfloat param);
  void PixelZoom(GLfloat xfactor, GLfloat yfactor);
  void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      GLenum format, GLenum type, GLvoid *pixels);
  void DepthMask(GLboolean flag);
  void ClearStencil(GLint s);
  void StencilFunc(GLenum func, GLint ref, GLuint mask);
  void StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
  void BindTexture(GLenum target, GLuint texture);
  void DeleteTextures(GLsizei n, const GLuint *textures);
  void GenTextures(GLsizei n, GLuint *textures);
  void TexEnvf(GLenum target, GLenum pname, GLfloat param);
  void TexParameteri(GLenum target, GLenum pname, GLint param);
  void TexImage2D(GLenum target, GLint level, GLint internalFormat,
      GLsizei width, GLsizei height, GLint border,
      GLenum format, GLenum type, const GLvoid *pixels);
  bool supportsReadPixels() const
    { return false; }
  // end DiGLPainter interface

  // begin DiPainter interface
  bool drawText(const std::string& text, float x, float y, float angle = 0);
  void drawCircle(float centerx, float centery, float radius);
  void fillCircle(float centerx, float centery, float radius);
  void drawRect(float x1, float y1, float x2, float y2);
  void fillRect(float x1, float y1, float x2, float y2);
  void drawLine(float x1, float y1, float x2, float y2);
  void drawPolyline(const QPolygonF& points);
  void drawPolygon(const QPolygonF& points);
  void drawPolygons(const QList<QPolygonF>& polygons);
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
};

/* Definitions from GL.h and paintgl.h. */

typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void            GLvoid;
typedef signed char     GLbyte;     /* 1-byte signed */
typedef short           GLshort;    /* 2-byte signed */
typedef int             GLint;      /* 4-byte signed */
typedef unsigned char   GLubyte;    /* 1-byte unsigned */
typedef unsigned short  GLushort;   /* 2-byte unsigned */
typedef unsigned int    GLuint;     /* 4-byte unsigned */
typedef int             GLsizei;    /* 4-byte signed */
typedef float           GLfloat;    /* single precision float */
typedef float           GLclampf;   /* single precision float in [0,1] */
typedef double          GLdouble;   /* double precision float */
typedef double          GLclampd;   /* double precision float in [0,1] */

#  define GLP_FIT_TO_PAGE	 1	// Fit the output to the page
#  define GLP_AUTO_CROP		 2	// Automatically crop to geometry
#  define GLP_GREYSCALE		 4	// Output greyscale rather than color
#  define GLP_REVERSE		 8	// Reverse grey shades
#  define GLP_DRAW_BACKGROUND	16	// Draw the background color
#  define GLP_BLACKWHITE        32      // Draw in black
#  define GLP_AUTO_ORIENT       64      // Automatic page orientation
#  define GLP_PORTRAIT         128      // Portrait
#  define GLP_LANDSCAPE        256      // Landscape

#  define GLP_RGBA		0	// RGBA mode window
#  define GLP_COLORINDEX	1	// Color index mode window

#  define GLP_SUCCESS		0	// Success - no error occurred
#  define GLP_OUT_OF_MEMORY	-1	// Out of memory
#  define GLP_NO_FEEDBACK	-2	// No feedback data available
#  define GLP_ILLEGAL_OPERATION	-3	// Illegal operation of some kind

typedef GLfloat GLPrgba[4];	// GLPrgba array structure for sanity

class GLPcontext
{
protected:
  enum {MAXIMAGES= 1000};
  float linescale;              // scale of line thickness
  float pointscale;             // scale of point size
  GLint	viewport[4];            // the OpenGL viewport

public:
  GLPcontext() {}
  virtual ~GLPcontext() {}

  virtual int StartPage(int mode = GLP_RGBA) { return GLP_SUCCESS; }
  virtual int StartPage(int /* mode */,
                        int /* size */,
                        GLPrgba * /* rgba */) { return 0; }
  virtual int UpdatePage(GLboolean /* more */) { return GLP_SUCCESS; }
  virtual int EndPage() { return GLP_SUCCESS; }

  void addReset() {}
  void setViewport(int /* x */, int /* y */, int /* width */, int /* height */) {}
  // add image - x/y in pixel coordinates
  virtual bool AddImage(const GLvoid*,// pixels
                        GLint,GLint,GLint, // size,nx,ny
                        GLfloat,GLfloat,GLfloat,GLfloat, // x,y,sx,sy
                        GLint,GLint,GLint,GLint,   // start,stop
                        GLenum,GLenum, // format, type
                        GLboolean =false) { return true; }

  void setScales(const float lsc =0.5, const float psc =0.5)
  {linescale= lsc; pointscale= psc; }
  bool addClipPath(const int& size,
                   const float* x, const float* y,
                   const bool rect = false);
  void addStencil(const int& /* size */, const float* /* x */, const float* /* y */) {}
  void addScissor(const double /* x0 */, const double /* y0 */,
                  const double /* w */, const double /* h */) {}
  void addScissor(const int /* x0 */, const int /* y0 */,
                  const int /* w */, const int /* h */) {}

  bool removeClipping() { return true; }
};

#endif // PAINTGLPAINTER_H
