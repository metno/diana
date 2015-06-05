
#ifndef DIGLPAINTER_H
#define DIGLPAINTER_H 1

#include "diPainter.h"

class QImage;

class DiGLCanvas : public DiCanvas {
public:
  typedef unsigned char   GLboolean;
  typedef unsigned int    GLuint;     /* 4-byte unsigned */
  typedef int             GLsizei;    /* 4-byte signed */

  virtual GLuint GenLists(GLsizei range);
  virtual GLboolean IsList(GLuint list);
  virtual void DeleteLists(GLuint list, GLsizei range);
  virtual bool supportsDrawLists() const;

  virtual QImage convertToGLFormat(const QImage& i) = 0;
};

// ========================================================================

class DiGLPainter : public DiPainter {
protected:
  DiGLPainter(DiGLCanvas* canvas)
    : DiPainter(canvas) { }

public:
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

  enum Errors {/* Errors */
    gl_NO_ERROR          = 0x0,
    gl_INVALID_ENUM      = 0x0500,
    gl_INVALID_VALUE     = 0x0501,
    gl_INVALID_OPERATION = 0x0502,
    gl_STACK_OVERFLOW    = 0x0503,
    gl_STACK_UNDERFLOW   = 0x0504,
    gl_OUT_OF_MEMORY     = 0x0505
  };

  enum AttributeBits {/* Attribute bits */
    gl_LINE_BIT    = 0x00000004,
    gl_POLYGON_BIT = 0x00000008
  };

  enum Buffers {/* Buffers, Pixel Drawing/Reading */
    gl_ALPHA = 0x1906,
    gl_COLOR = 0x1800,
    gl_RGB   = 0x1907,
    gl_RGBA  = 0x1908
  };

  enum DepthBuffer { /* Depth buffer */
    gl_ALWAYS     = 0x0207,
    gl_DEPTH_TEST = 0x0B71,
    gl_EQUAL      = 0x0202,
    gl_NOTEQUAL   = 0x0205
  };

  enum Polygons {/* Polygons */
    gl_BACK            = 0x0405,
    gl_LINE            = 0x1B01,
    gl_FILL            = 0x1B02,
    gl_POLYGON_STIPPLE = 0x0B42
  };

  enum Blending { /* Blending */
    gl_BLEND               = 0x0BE2,
    gl_ONE_MINUS_SRC_ALPHA = 0x0303,
    gl_SRC_ALPHA           = 0x0302
  };

  enum DisplayLists { /* Display Lists */
    gl_COMPILE             = 0x1300,
    gl_COMPILE_AND_EXECUTE = 0x1301
  };

  enum Gets { /* Gets */
    gl_CURRENT_COLOR    = 0x0B00,
    gl_MODELVIEW_MATRIX = 0x0BA6,
    gl_VIEWPORT         = 0x0BA2
  };

  enum AlphaTest { /* Alpha testing */
    gl_ALPHA_TEST = 0x0BC0
  };

  enum VertexArrays { /* Vertex Arrays */
    gl_COLOR_ARRAY         = 0x8076,
    gl_EDGE_FLAG_ARRAY     = 0x8079,
    gl_INDEX_ARRAY         = 0x8077,
    gl_NORMAL_ARRAY        = 0x8075,
    gl_TEXTURE_COORD_ARRAY = 0x8078,
    gl_VERTEX_ARRAY        = 0x8074
  };

  enum PushPopAttrib { /* glPush/PopAttrib bits */
    gl_COLOR_BUFFER_BIT   = 0x00004000,
    gl_CURRENT_BIT        = 0x00000001,
    gl_DEPTH_BUFFER_BIT   = 0x00000100,
    gl_PIXEL_MODE_BIT     = 0x00000020,
    gl_STENCIL_BUFFER_BIT = 0x00000400
  };

  enum DataTypes { /* Data types */
    gl_DOUBLE        = 0x140A,
    gl_FLOAT         = 0x1406,
    gl_INT           = 0x1404,
    gl_SHORT         = 0x1402,
    gl_UNSIGNED_BYTE = 0x1401
  };

  enum Boolean { /* Boolean values */
    gl_FALSE = 0x0,
    gl_TRUE  = 0x1
  };

  enum Lighting { /* Lighting */
    //gl_BACK           = 0x0405,
    gl_FLAT           = 0x1D00,
    gl_FRONT          = 0x0404,
    gl_FRONT_AND_BACK = 0x0408,
    gl_LIGHTING       = 0x0B50,
    gl_SMOOTH         = 0x1D01
  };

  enum Fog { /* Fog */
    gl_FOG = 0x0B60
  };

  enum Stencil { /* Stencil */
    gl_KEEP         = 0x1E00,
    gl_REPLACE      = 0x1E01,
    gl_STENCIL_TEST = 0x0B90
  };

  enum Primitives { /* Primitives */
    gl_LINES          = 0x0001,
    gl_LINE_LOOP      = 0x0002,
    gl_LINE_STRIP     = 0x0003,
    gl_POINTS         = 0x0000,
    gl_POLYGON        = 0x0009,
    gl_QUADS          = 0x0007,
    gl_QUAD_STRIP     = 0x0008,
    gl_TRIANGLE_FAN   = 0x0006,
    gl_TRIANGLE_STRIP = 0x0005,
    gl_TRIANGLES      = 0x0004
  };

  enum Lines { /* Lines */
    gl_LINE_STIPPLE= 0x0B24,
    gl_LINE_WIDTH  = 0x0B21
  };

  enum ImplementationLimits { /* Implementation limits */
    gl_MAX_VIEWPORT_DIMS = 0x0D3A
  };

  enum TextureMapping { /* Texture mapping */
    gl_NEAREST            = 0x2600,
    gl_REPEAT             = 0x2901,
    gl_TEXTURE_2D         = 0x0DE1,
    gl_TEXTURE_ENV        = 0x2300,
    gl_TEXTURE_ENV_MODE   = 0x2200,
    gl_TEXTURE_MAG_FILTER = 0x2800,
    gl_TEXTURE_MIN_FILTER = 0x2801,
    gl_TEXTURE_WRAP_S     = 0x2802,
    gl_TEXTURE_WRAP_T     = 0x2803
  };

  enum PixelModeTransfer { /* Pixel Mode / Transfer */
    gl_ALPHA_SCALE        = 0x0D1C,
    gl_BLUE_BIAS          = 0x0D1B,
    gl_GREEN_BIAS         = 0x0D19,
    gl_PACK_ALIGNMENT     = 0x0D05,
    gl_PACK_ROW_LENGTH    = 0x0D02,
    gl_PACK_SKIP_PIXELS   = 0x0D04,
    gl_PACK_SKIP_ROWS     = 0x0D03,
    gl_RED_BIAS           = 0x0D15,
    gl_UNPACK_ALIGNMENT   = 0x0CF5,
    gl_UNPACK_ROW_LENGTH  = 0x0CF2,
    gl_UNPACK_SKIP_PIXELS = 0x0CF4,
    gl_UNPACK_SKIP_ROWS   = 0x0CF3
  };

  enum Points { /* Points */
    gl_POINT_SMOOTH = 0x0B10
  };

  enum Multisampling { /* Multisampling */
    gl_MULTISAMPLE = 0x809D
  };

  enum ARB_imaging { /* GL_ARB_imaging */
    gl_TABLE_TOO_LARGE = 0x8031
  };

  DiGLCanvas* canvas()
    { return (DiGLCanvas*)DiPainter::canvas(); }

  virtual void Begin(GLenum mode) = 0;
  virtual void Color3d(GLdouble red, GLdouble green, GLdouble blue) = 0;
  virtual void Color3f(GLfloat red, GLfloat green, GLfloat blue) = 0;
  virtual void Color3fv(const GLfloat *v) = 0;
  virtual void Color3ub(GLubyte red, GLubyte green, GLubyte blue) = 0;
  virtual void Color3ubv(const GLubyte *v) = 0;
  virtual void Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) = 0;
  virtual void Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = 0;
  virtual void Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) = 0;
  virtual void Color4fv(const GLfloat *v) = 0;
  virtual void Color4ubv(const GLubyte *v) = 0;
  virtual void End() = 0;
  virtual void Indexi(GLint c) = 0;
  virtual void RasterPos2f(GLfloat x, GLfloat y) = 0;
  virtual void Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) = 0;
  virtual void TexCoord2f(GLfloat s, GLfloat t) = 0;
  virtual void Vertex2dv(const GLdouble *v) = 0;
  virtual void Vertex2f(GLfloat x, GLfloat y) = 0;
  virtual void Vertex2i(GLint x, GLint y) = 0;
  virtual void Vertex3f(GLfloat x, GLfloat y, GLfloat z) = 0;
  virtual void Vertex3i(GLint x, GLint y, GLint z) = 0;
  virtual void DrawArrays(GLenum mode, GLint first, GLsizei count) = 0;
  virtual void VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) = 0;
  virtual void BlendFunc(GLenum sfactor, GLenum dfactor) = 0;
  virtual void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) = 0;
  virtual void Clear(GLbitfield mask) = 0;
  virtual void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = 0;
  virtual void Disable(GLenum cap) = 0;
  virtual void DisableClientState(GLenum cap) = 0; /* 1.1 */
  virtual void DrawBuffer(GLenum mode) = 0;
  virtual void EdgeFlag(GLboolean flag) = 0;
  virtual void Enable(GLenum cap) = 0;
  virtual void EnableClientState(GLenum cap) = 0; /* 1.1 */
  virtual void Flush(void) = 0;
  virtual void GetFloatv(GLenum pname, GLfloat *params) = 0;
  virtual void GetIntegerv(GLenum pname, GLint *params) = 0;
  virtual GLboolean IsEnabled(GLenum cap) = 0;
  virtual void LineStipple(GLint factor, GLushort pattern) = 0;
  virtual void LineWidth(GLfloat width) = 0;
  virtual void PointSize(GLfloat size) = 0;
  virtual void PolygonMode(GLenum face, GLenum mode) = 0;
  virtual void PolygonStipple(const GLubyte *mask) = 0;
  virtual void PopAttrib(void) = 0;
  virtual void PushAttrib(GLbitfield mask) = 0;
  virtual void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val) = 0;
  virtual void LoadIdentity(void) = 0;
  virtual void PopMatrix(void) = 0;
  virtual void PushMatrix(void) = 0;
  virtual void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) = 0;
  virtual void Scalef(GLfloat x, GLfloat y, GLfloat z) = 0;
  virtual void Translatef(GLfloat x, GLfloat y, GLfloat z) = 0;
  virtual void Viewport(GLint x, GLint y, GLsizei width, GLsizei height) = 0;
  virtual void CallList(GLuint list);
  virtual void EndList();
  virtual void NewList(GLuint list, GLenum mode);
  virtual void ShadeModel(GLenum mode) = 0;
  virtual void Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
      GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) = 0;
  virtual void DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
      const GLvoid *pixels) = 0;
  virtual void PixelStorei(GLenum pname, GLint param) = 0;
  virtual void PixelTransferf(GLenum pname, GLfloat param) = 0;
  virtual void PixelZoom(GLfloat xfactor, GLfloat yfactor) = 0;
  virtual void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      GLenum format, GLenum type, GLvoid *pixels) = 0;
  virtual void DepthMask(GLboolean flag) = 0;
  virtual void ClearStencil(GLint s) = 0;
  virtual void StencilFunc(GLenum func, GLint ref, GLuint mask) = 0;
  virtual void StencilOp(GLenum fail, GLenum zfail, GLenum zpass) = 0;
  virtual void BindTexture(GLenum target, GLuint texture) = 0;
  virtual void DeleteTextures(GLsizei n, const GLuint *textures) = 0;
  virtual void GenTextures(GLsizei n, GLuint *textures) = 0;
  virtual void TexEnvf(GLenum target, GLenum pname, GLfloat param) = 0;
  virtual void TexParameteri(GLenum target, GLenum pname, GLint param) = 0;
  virtual void TexImage2D(GLenum target, GLint level, GLint internalFormat,
      GLsizei width, GLsizei height, GLint border,
      GLenum format, GLenum type, const GLvoid *pixels) = 0;

  virtual bool supportsReadPixels() const = 0;

  void clear(const Colour& colour);
  void setColour(const Colour& c, bool alpha = true) Q_DECL_OVERRIDE;
  void setLineStyle(const Colour& c, float lw=1, bool alpha = true) Q_DECL_OVERRIDE;
  void setLineStyle(const Colour& c, float lw, const Linetype& lt, bool alpha = true) Q_DECL_OVERRIDE;

  void drawLine(float x1, float y1, float x2, float y2);
  void drawPolyline(const QPolygonF& points);
  /* draw a series of filled quads */
  void fillQuadStrip(const QPolygonF& points);
  using DiPainter::drawRect;
  void drawRect(float x1, float y1, float x2, float y2);
  using DiPainter::fillRect;
  void fillRect(float x1, float y1, float x2, float y2);
  void drawCircle(float centerx, float centery, float radius);
  void fillCircle(float centerx, float centery, float radius);

  void drawWindArrow(float u, float v, float x, float y,
      float arrowSize, bool withArrowHead);
};

#endif // DIGLPAINTER_H
