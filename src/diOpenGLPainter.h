
#ifndef DIOPENGLPAINTER_H
#define DIOPENGLPAINTER_H 1

#include "diGLPainter.h"

#include <memory>

class FontManager;

class QGLWidget;

class DiOpenGLCanvas : public DiGLCanvas {
public:
  DiOpenGLCanvas(QGLWidget* widget);

  void setVpGlSize(float vpw, float vph, float glw, float glh);
  bool setFont(const std::string& font);
  bool setFont(const std::string& font, float size, FontFace face);
  bool setFontSize(float size);
  bool getCharSize(char ch, float& w, float& h);
  bool getTextSize(const std::string& text, float& w, float& h);

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
  std::auto_ptr<FontManager> mFP;
  QGLWidget* mWidget;
};

class DiOpenGLPainter : public DiGLPainter {
public:
  DiOpenGLPainter(DiOpenGLCanvas* canvas);

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
  void Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val);
  void LoadIdentity(void);
  void PopMatrix(void);
  void PushMatrix(void);
  void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
  void Scalef(GLfloat x, GLfloat y, GLfloat z);
  void Translatef(GLfloat x, GLfloat y, GLfloat z);
  void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
  void CallList(GLuint list);
  void EndList(void);
  void NewList(GLuint list, GLenum mode);
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
    { return true; }

  // ========================================

  bool drawText(const std::string& text, float x, float y, float angle);
  void drawPolygon(const QPolygonF& points);
  void drawPolygons(const QList<QPolygonF>& polygons) Q_DECL_OVERRIDE;

  void drawReprojectedImage(const QImage& image, const float* mapPositionsXY, bool smooth);
};

#endif // DIOPENGLPAINTER_H
