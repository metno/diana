#ifndef QTDIANADEVICEPAINTER_H
#define QTDIANADEVICEPAINTER_H

#include <QPainter>
#include <memory>

class DiPaintGLCanvas;
class DiPaintGLPainter;
class DiCanvas;
class GLwidget;

class QPaintDevice;

class DianaImageExporter {
public:
  DianaImageExporter(GLwidget* glw, QPaintDevice* pd, bool printing);
  ~DianaImageExporter();

  void paintOnDevice();

private:
  GLwidget* glw_;
  QPaintDevice* device;

  DiCanvas* oldCanvas;
  int oldWidth, oldHeight;

  std::auto_ptr<DiPaintGLCanvas> glcanvas;
  std::auto_ptr<DiPaintGLPainter> glpainter;
  QPainter painter;
};


#endif // QTDIANADEVICEPAINTER_H
