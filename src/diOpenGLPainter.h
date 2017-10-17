
#ifndef DIOPENGLPAINTER_H
#define DIOPENGLPAINTER_H 1

#include "diGLPainter.h"

#include <memory>

class FontManager;

class QGLWidget;

class DiOpenGLCanvas : public DiGLCanvas {
public:
  DiOpenGLCanvas(QGLWidget* widget);
  ~DiOpenGLCanvas();

  void parseFontSetup(const std::vector<std::string>& sect_fonts) Q_DECL_OVERRIDE;
  void defineFont(const std::string& fontfam, const std::string& fontfilename,
      const std::string& face, bool use_bitmap) Q_DECL_OVERRIDE;

  void setVpGlSize(int vpw, int vph, float glw, float glh) override;
  bool setFont(const std::string& font) override;
  bool setFont(const std::string& font, float size, FontFace face) override;
  bool setFontSize(float size) override;
  bool getTextRect(const QString& text, float& x, float& y, float& w, float& h) override;

  inline FontManager* fp()
    { if (!mFP.get()) initializeFP(); return mFP.get(); }

  void DeleteLists(GLuint list, GLsizei range) Q_DECL_OVERRIDE;
  GLuint GenLists(GLsizei range) Q_DECL_OVERRIDE;
  GLboolean IsList(GLuint list) Q_DECL_OVERRIDE;
  bool supportsDrawLists() const Q_DECL_OVERRIDE;

  QImage convertToGLFormat(const QImage& i) Q_DECL_OVERRIDE;

  GLuint bindTexture(const QImage& image);
  void deleteTexture(GLuint texid);

private:
  void initializeFP();

private:
  std::unique_ptr<FontManager> mFP;
  QGLWidget* mWidget;
};

class DiOpenGLPainter : public DiGLPainter {
public:
  DiOpenGLPainter(DiOpenGLCanvas* canvas);

  void Begin(GLenum mode) override;
  void Color3d(GLdouble red, GLdouble green, GLdouble blue) override;
  void Color3f(GLfloat red, GLfloat green, GLfloat blue) override;
  void Color3fv(const GLfloat *v) override;
  void Color3ub(GLubyte red, GLubyte green, GLubyte blue) override;
  void Color3ubv(const GLubyte *v) override;
  void Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) override;
  void Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) override;
  void Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) override;
  void Color4fv(const GLfloat *v) override;
  void Color4ubv(const GLubyte *v) override;
  void End() override;
  void Indexi(GLint c) override;
  void RasterPos2f(GLfloat x, GLfloat y) override;
  void Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) override;
  void TexCoord2f(GLfloat s, GLfloat t) override;
  void Vertex2dv(const GLdouble *v) override;
  void Vertex2f(GLfloat x, GLfloat y) override;
  void Vertex2i(GLint x, GLint y) override;
  void Vertex3f(GLfloat x, GLfloat y, GLfloat z) override;
  void Vertex3i(GLint x, GLint y, GLint z) override;
  void DrawArrays(GLenum mode, GLint first, GLsizei count) override;
  void VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) override;
  void BlendFunc(GLenum sfactor, GLenum dfactor) override;
  void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) override;
  void Clear(GLbitfield mask) override;
  void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) override;
  void Disable(GLenum cap) override;
  void DisableClientState(GLenum cap) override; /* 1.1 */
  void DrawBuffer(GLenum mode) override;
  void EdgeFlag(GLboolean flag) override;
  void Enable(GLenum cap) override;
  void EnableClientState(GLenum cap) override; /* 1.1 */
  void Flush() override;
  void GetFloatv(GLenum pname, GLfloat *params) override;
  void GetIntegerv(GLenum pname, GLint *params) override;
  GLboolean IsEnabled(GLenum cap) override;
  void LineStipple(GLint factor, GLushort pattern) override;
  void LineWidth(GLfloat width) override;
  void PointSize(GLfloat size) override;
  void PolygonMode(GLenum face, GLenum mode) override;
  void PolygonStipple(const GLubyte *mask) override;
  void PopAttrib() override;
  void PushAttrib(GLbitfield mask) override;
  void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val) override;
  void LoadIdentity() override;
  void PopMatrix() override;
  void PushMatrix() override;
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) override;
  void Scalef(GLfloat x, GLfloat y, GLfloat z) override;
  void Translatef(GLfloat x, GLfloat y, GLfloat z) override;
  void Viewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
  void CallList(GLuint list) override;
  void EndList() override;
  void NewList(GLuint list, GLenum mode) override;
  void ShadeModel(GLenum mode) override;
  void Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
      GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) override;
  void DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
      const GLvoid *pixels) override;
  void PixelStorei(GLenum pname, GLint param) override;
  void PixelTransferf(GLenum pname, GLfloat param) override;
  void PixelZoom(GLfloat xfactor, GLfloat yfactor) override;
  void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      GLenum format, GLenum type, GLvoid *pixels) override;
  void DepthMask(GLboolean flag) override;
  void ClearStencil(GLint s) override;
  void StencilFunc(GLenum func, GLint ref, GLuint mask) override;
  void StencilOp(GLenum fail, GLenum zfail, GLenum zpass) override;
  void BindTexture(GLenum target, GLuint texture) override;
  void DeleteTextures(GLsizei n, const GLuint *textures) override;
  void GenTextures(GLsizei n, GLuint *textures) override;
  void TexEnvf(GLenum target, GLenum pname, GLfloat param) override;
  void TexParameteri(GLenum target, GLenum pname, GLint param) override;
  void TexImage2D(GLenum target, GLint level, GLint internalFormat,
      GLsizei width, GLsizei height, GLint border,
      GLenum format, GLenum type, const GLvoid *pixels) override;
  bool supportsReadPixels() const override
    { return true; }

  // ========================================

  bool drawText(const QString& text, const QPointF& xy, float angle) override;
  void drawPolygon(const QPolygonF& points) override;
  void drawPolygons(const QList<QPolygonF>& polygons) override;

  void drawScreenImage(const QPointF&, const QImage&) override;
};

#endif // DIOPENGLPAINTER_H
