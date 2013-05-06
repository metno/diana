#ifndef PAINTGL_H
#define PAINTGL_H

#include <map>

#include <QColor>
#include <QHash>
#include <QList>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPicture>
#include <QPointF>
#include <QStack>
#include <QTransform>
#include <QVector>
#include <QWidget>

#include "GL/gl.h"

struct PaintAttributes {
    QRgb color;
    float width;
    QHash<GLenum,GLenum> polygonMode;
    QVector<qreal> dashes;
    qreal dashOffset;
    bool stipple;
};

struct StencilAttributes {
    GLint clear;
    GLenum func;
    GLint ref;
    GLuint mask;
    GLenum fail;
    GLenum zfail;
    GLenum zpass;
    QPainterPath path;

    bool clip;
    bool update;
    bool enabled;
};

struct RenderItem {
    GLenum mode;
    QPainter *painter;
    GLuint list;
};

class PaintGLContext {
public:
    PaintGLContext();
    virtual ~PaintGLContext();

    void makeCurrent();
    void begin(QPainter *painter);
    bool isPainting() const;
    void end();

    void renderPrimitive();
    void setViewportTransform();

    void setClipPath();
    void unsetClipPath();

    QPainter *painter;

    QStack<GLenum> stack;
    QStack<RenderItem> renderStack;
    QStack<QTransform> transformStack;

    bool blend;
    QPainter::CompositionMode blendMode;
    bool smooth;

    bool useTexture;
    GLuint currentTexture;

    QPointF rasterPos;
    QPointF pixelZoom;
    QHash<GLenum,GLint> pixelStore;
    QPointF bitmapMove;

    GLfloat pointSize;
    QColor clearColor;
    bool clear;
    QFont font;
    bool colorMask;

    GLenum mode;
    PaintAttributes attributes;
    StencilAttributes stencil;
    QPolygonF points;
    QList<bool> validPoints;
    QList<QRgb> colors;
    QTransform transform;

    QHash<GLuint,QPicture> lists;
    QHash<GLuint,QTransform> listTransforms;
    QHash<GLuint,QImage> textures;

    QRect viewport;
    QRectF window;

    bool printing;

private:
    void plotSubdivided(const QPointF quad[], const QRgb color[], int divisions = 0);
    void setPen();
    void setPolygonColor(const QRgb &color);
};

class PaintGL {
public:
    PaintGL();
    ~PaintGL();

    static PaintGL *instance() { return self; }
    PaintGLContext *currentContext;

private:
    static PaintGL *self;
};

//
// Printing options...
//

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

class GLPfile : public GLPcontext	//// GLPfile class
{
public:
  GLPfile(char *print_name,
          int  print_options = GLP_FIT_TO_PAGE | GLP_DRAW_BACKGROUND,
          int  print_size = 0,
          std::map<std::string,std::string> *mc = 0,
          bool makeepsf = false) {}
  ~GLPfile() {}
};

class glText {
public:
    enum FontScaling {
      S_FIXEDSIZE, S_VIEWPORTSCALED
    };
    enum {
      MAXFONTFACES = 4, MAXFONTS = 10, MAXFONTSIZES = 40
    };
    enum FontFace {
      F_NORMAL = 0, F_BOLD = 1, F_ITALIC = 2, F_BOLD_ITALIC = 3
    };

    glText() {}
    ~glText() {}

    // fill fontpack for testing
    virtual bool testDefineFonts(std::string path = "fonts");
    // define all fonts matching pattern with family, face and ps-equiv
    virtual bool defineFonts(const std::string pattern, const std::string family,
        const std::string psname = "");
    // define new font (family,name,face,size, ps-equiv, ps-xscale, ps-yscale)
    virtual bool defineFont(const std::string, const std::string, const glText::FontFace,
        const int, const std::string = "", const float = 1.0, const float = 1.0);
    // choose font, size and face
    virtual bool set(const std::string, const glText::FontFace, const float);
    virtual bool setFont(const std::string);
    virtual bool setFontFace(const glText::FontFace);
    virtual bool setFontSize(const float);
    // printing commands
    virtual bool drawChar(const int c, const float x, const float y,
        const float a = 0);
    virtual bool drawStr(const char* s, const float x, const float y,
        const float a = 0);
    // Metric commands
    virtual void adjustSize(const int sa)
    {
      size_add = sa;
    }
    virtual void setScalingType(const glText::FontScaling fs)
    {
      scaletype = fs;
    }
    // set viewport size in GL coordinates
    virtual void setGlSize(const float glw, const float glh)
    {
      glWidth = glw;
      glHeight = glh;
      pixWidth = glWidth / vpWidth;
      pixHeight = glHeight / vpHeight;
    }
    // set viewport size in physical coordinates (pixels)
    virtual void setVpSize(const float vpw, const float vph)
    {
      vpWidth = vpw;
      vpHeight = vph;
      pixWidth = glWidth / vpWidth;
      pixHeight = glHeight / vpHeight;
    }
    virtual void setPixSize(const float pw, const float ph)
    {
      pixWidth = pw;
      pixHeight = ph;
    }
    virtual bool getCharSize(const int c, float& w, float& h);
    virtual bool getMaxCharSize(float& w, float& h);
    virtual bool getStringSize(const char* s, float& w, float& h);
    // return info
    glText::FontScaling getFontScaletype()
    {
      return scaletype;
    }
    int getNumFonts()
    {
      return numFonts;
    }
    int getNumSizes()
    {
      return numSizes;
    }
    glText::FontFace getFontFace()
    {
      return Face;
    }
    virtual float getFontSize()
    {
      if (numSizes)
        return Sizes[SizeIndex];
      else
        return 0;
    }
    int getFontSizeIndex()
    {
      return SizeIndex;
    }
    std::string getFontName(const int index)
    {
      return (index >= 0 && index < numFonts) ? FamilyName[index] : std::string("");
    }
    // PostScript equivalent name
    virtual std::string getPsName() const
    {
      return "";
    }
    // for harcopy production (postscript)
    virtual void startHardcopy(GLPcontext*) {}
    virtual void endHardcopy() {}
    virtual void setWysiwyg(const bool) {}
    // requested-size/actual-size
    virtual float getSizeDiv() { return 0.0; }

private:
  std::string FamilyName[glText::MAXFONTS];
  int Sizes[glText::MAXFONTSIZES];
  int numFonts; // number of defined fonts
  int numSizes; // number of defined fontsizes
  QHash<QString,QString> fontMap;

  int FontIndex; // current font index
  glText::FontFace Face; // current font face
  int SizeIndex; // current fontsize index
  float reqSize; // last requested font size
  glText::FontScaling scaletype; // current scaling behaviour
  bool wysiwyg; // hardcopy size = screen size
  float vpWidth, vpHeight; // viewport size in pixels
  float glWidth, glHeight; // viewport size in gl-coord.
  float pixWidth, pixHeight; // size of pixel in gl-coord.
  bool initialised;
  int size_add; // user-controlled size modification
  bool hardcopy; // Hardcopy production
  GLPcontext* output; // OpenGL Hardcopy Module

  bool _addFamily(const std::string, int&) { return true; }
  bool _addSize(const int, int&) { return true; }
  bool _findSize(const int, int&, const bool = false) { return true; }
  bool _findFamily(const std::string, int&) { return true; }
};

class PaintGLWidget : public QWidget
{
    Q_OBJECT

public:
    PaintGLWidget(QWidget *parent, bool antialiasing = false);
    virtual ~PaintGLWidget();

    QImage grabFrameBuffer(bool withAlpha = false);
    bool isValid();
    void makeCurrent();
    void swapBuffers();
    virtual void updateGL();

public slots:
    /// Print the visible contents of the widget.
    void print(QPrinter* device);

protected:
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    void setAutoBufferSwap(bool enable);

    virtual void paint(QPainter *painter);

    PaintGLContext *glContext;
    bool initialized;
    bool antialiasing;
};

#endif
