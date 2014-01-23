/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
  Note: This is only a subset of the original gl.h file.
 */

#ifndef __gl_h__
#define __gl_h__

#ifdef __cplusplus
extern "C" {
#endif

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

/* Errors */
#define GL_NO_ERROR             0x0
#define GL_INVALID_ENUM         0x0500
#define GL_INVALID_VALUE        0x0501
#define GL_INVALID_OPERATION    0x0502
#define GL_STACK_OVERFLOW       0x0503
#define GL_STACK_UNDERFLOW      0x0504
#define GL_OUT_OF_MEMORY        0x0505

/* Attribute bits */
#define GL_LINE_BIT             0x00000004
#define GL_POLYGON_BIT          0x00000008

/* Buffers, Pixel Drawing/Reading */
#define GL_ALPHA                0x1906
#define GL_COLOR                0x1800
#define GL_RGB                  0x1907
#define GL_RGBA                 0x1908

/* Depth buffer */
#define GL_ALWAYS               0x0207
#define GL_DEPTH_TEST           0x0B71
#define GL_EQUAL                0x0202
#define GL_NOTEQUAL             0x0205

/* Polygons */
#define GL_BACK                 0x0405
#define GL_LINE                 0x1B01
#define GL_FILL                 0x1B02
#define GL_POLYGON_STIPPLE      0x0B42

/* Blending */
#define GL_BLEND                0x0BE2
#define GL_ONE_MINUS_SRC_ALPHA  0x0303
#define GL_SRC_ALPHA            0x0302

/* Display Lists */
#define GL_COMPILE              0x1300
#define GL_COMPILE_AND_EXECUTE  0x1301

/* Gets */
#define GL_CURRENT_COLOR        0x0B00
#define GL_MODELVIEW_MATRIX     0x0BA6
#define GL_VIEWPORT             0x0BA2

/* Alpha testing */
#define GL_ALPHA_TEST           0x0BC0

/* Vertex Arrays */
#define GL_COLOR_ARRAY          0x8076
#define GL_EDGE_FLAG_ARRAY      0x8079
#define GL_INDEX_ARRAY          0x8077
#define GL_NORMAL_ARRAY         0x8075
#define GL_TEXTURE_COORD_ARRAY  0x8078
#define GL_VERTEX_ARRAY         0x8074

/* glPush/PopAttrib bits */
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_DEPTH_BUFFER_BIT     0x00000100
#define GL_STENCIL_BUFFER_BIT   0x00000400

/* Data types */
#define GL_DOUBLE               0x140A
#define GL_FLOAT                0x1406
#define GL_INT                  0x1404
#define GL_SHORT                0x1402
#define GL_UNSIGNED_BYTE        0x1401

/* Boolean values */
#define GL_FALSE                0x0
#define GL_TRUE                 0x1

/* Lighting */
#define GL_BACK                 0x0405
#define GL_FLAT                 0x1D00
#define GL_FRONT                0x0404
#define GL_FRONT_AND_BACK       0x0408
#define GL_LIGHTING             0x0B50
#define GL_SMOOTH               0x1D01

/* Fog */
#define GL_FOG                  0x0B60

/* Stencil */
#define GL_KEEP                 0x1E00
#define GL_REPLACE              0x1E01
#define GL_STENCIL_TEST         0x0B90

/* Primitives */
#define GL_LINES                0x0001
#define GL_LINE_LOOP            0x0002
#define GL_LINE_STRIP           0x0003
#define GL_POINTS               0x0000
#define GL_POLYGON              0x0009
#define GL_QUADS                0x0007
#define GL_QUAD_STRIP           0x0008
#define GL_TRIANGLE_FAN         0x0006
#define GL_TRIANGLE_STRIP       0x0005
#define GL_TRIANGLES            0x0004

/* Lines */
#define GL_LINE_STIPPLE         0x0B24
#define GL_LINE_WIDTH           0x0B21

/* Implementation limits */
#define GL_MAX_VIEWPORT_DIMS    0x0D3A

/* Texture mapping */
#define GL_NEAREST              0x2600
#define GL_REPEAT               0x2901
#define GL_TEXTURE_2D           0x0DE1
#define GL_TEXTURE_ENV          0x2300
#define GL_TEXTURE_ENV_MODE     0x2200
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803

/* Pixel Mode / Transfer */
#define GL_PACK_ALIGNMENT       0x0D05
#define GL_PACK_ROW_LENGTH      0x0D02
#define GL_PACK_SKIP_PIXELS     0x0D04
#define GL_PACK_SKIP_ROWS       0x0D03
#define GL_UNPACK_ALIGNMENT     0x0CF5
#define GL_UNPACK_ROW_LENGTH    0x0CF2
#define GL_UNPACK_SKIP_PIXELS   0x0CF4
#define GL_UNPACK_SKIP_ROWS     0x0CF3

/* Points */
#define GL_POINT_SMOOTH         0x0B10

/* Scissor box */
#define GL_SCISSOR_TEST         0x0C11

/* Multisampling */
#define GL_MULTISAMPLE          0x809D

/*
 * GL_ARB_imaging
 */

#define GL_TABLE_TOO_LARGE      0x8031

/*
 * Drawing Functions
 */

void glBegin(GLenum mode);
void glColor3d(GLdouble red, GLdouble green, GLdouble blue);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor3fv(const GLfloat *v);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor3ubv(const GLubyte *v);
void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColor4fv(const GLfloat *v);
void glColor4ubv(const GLubyte *v);
void glEnd();
void glIndexi(GLint c);
void glRasterPos2f(GLfloat x, GLfloat y);
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void glTexCoord2f(GLfloat s, GLfloat t);
void glVertex2dv(const GLdouble *v);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex2i(GLint x, GLint y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3i(GLint x, GLint y, GLint z);

/*
 * Vertex Arrays  (1.1)
 */

void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);

/*
 * Miscellaneous
 */

void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClear(GLbitfield mask);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glDisable(GLenum cap);
void glDisableClientState(GLenum cap);  /* 1.1 */
void glDrawBuffer(GLenum mode);
void glEdgeFlag(GLboolean flag);
void glEnable(GLenum cap);
void glEnableClientState(GLenum cap);  /* 1.1 */
void glFlush(void);
void glGetFloatv(GLenum pname, GLfloat *params);
void glGetIntegerv(GLenum pname, GLint *params);
GLboolean glIsEnabled(GLenum cap);
void glLineStipple(GLint factor, GLushort pattern);
void glLineWidth(GLfloat width);
void glPointSize(GLfloat size);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonStipple(const GLubyte *mask);
void glPopAttrib(void);
void glPushAttrib(GLbitfield mask);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);

/*
 * Transformation
 */

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val);
void glLoadIdentity(void);
void glPopMatrix(void);
void glPushMatrix(void);

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

/*
 * Display Lists
 */

void glCallList(GLuint list);
void glDeleteLists(GLuint list, GLsizei range);
void glEndList(void);
GLuint glGenLists(GLsizei range);
GLboolean glIsList(GLuint list);
void glNewList(GLuint list, GLenum mode);

/*
 * Lighting
 */

void glShadeModel(GLenum mode);

/*
 * Raster functions
 */

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
              GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
                  const GLvoid *pixels);
void glPixelStorei(GLenum pname, GLint param);
void glPixelZoom(GLfloat xfactor, GLfloat yfactor);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels);

/*
 * Depth Buffer
 */

void glDepthMask(GLboolean flag);

/*
 * Stenciling
 */

void glClearStencil(GLint s);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);

/*
 * Texture mapping
 */

void glBindTexture(GLenum target, GLuint texture);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glGenTextures(GLsizei n, GLuint *textures);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels);

#ifdef __cplusplus
}
#endif

#endif
