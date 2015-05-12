
#ifndef DIOPENGLWIDGET_H
#define DIOPENGLWIDGET_H 1

#include "diOpenGLPainter.h"
#include <QGLWidget>
#include <memory>

class DiPaintable;

class DiOpenGLWidget : public QGLWidget
{
public:
  DiOpenGLWidget(DiPaintable* p, QWidget* parent=0);

  void initializeGL();
  void paintGL();
  void resizeGL(int w, int h);

  void keyPressEvent(QKeyEvent *ke);
  void keyReleaseEvent(QKeyEvent *ke);
  void mousePressEvent(QMouseEvent* me);
  void mouseMoveEvent(QMouseEvent* me);
  void mouseReleaseEvent(QMouseEvent* me);
  void mouseDoubleClickEvent(QMouseEvent* me);
  void wheelEvent(QWheelEvent *we);

private:
  std::auto_ptr<DiOpenGLCanvas> glcanvas;
  std::auto_ptr<DiOpenGLPainter> glpainter;
  DiPaintable* paintable;
};

#endif // DIOPENGLWIDGET_H
