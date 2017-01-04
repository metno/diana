#include "qtDianaDevicePainter.h"

#include "diPaintGLPainter.h"
#include "qtGLwidget.h"

DianaImageExporter::DianaImageExporter(GLwidget* glw, QPaintDevice* pd, bool printing)
  : glw_(glw)
  , device(pd)
  , oldCanvas(glw_->canvas())
  , oldWidth(glw_->width())
  , oldHeight(glw_->height())
  , glcanvas(new DiPaintGLCanvas(pd))
  , glpainter(new DiPaintGLPainter(glcanvas.get()))
{
  glcanvas->parseFontSetup();
  glcanvas->setPrinting(printing);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);
  glw_->setCanvas(glcanvas.get());
  glpainter->Viewport(0, 0, device->width(), device->height());
  glw_->resize(device->width(), device->height());
}

void DianaImageExporter::paintOnDevice()
{
  painter.begin(device);
  glpainter->begin(&painter);
  glw_->paintUnderlay(glpainter.get());
  glw_->paintOverlay(glpainter.get());
  glpainter->end();
  painter.end();
}

DianaImageExporter::~DianaImageExporter()
{
  glw_->setCanvas(oldCanvas);
  glw_->resize(oldWidth, oldHeight);
}

