
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
  ~DiOpenGLWidget();

  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

  void keyPressEvent(QKeyEvent *ke) override;
  void keyReleaseEvent(QKeyEvent *ke) override;
  void mousePressEvent(QMouseEvent* me) override;
  void mouseMoveEvent(QMouseEvent* me) override;
  void mouseReleaseEvent(QMouseEvent* me) override;
  void mouseDoubleClickEvent(QMouseEvent* me) override;
  void wheelEvent(QWheelEvent *we) override;

private:;
  void paintUnderlay();
  void paintOverlay();
  void dropBackgroundBuffer();

private:
  std::unique_ptr<DiOpenGLCanvas> glcanvas;
  std::unique_ptr<DiOpenGLPainter> glpainter;
  DiPaintable* paintable;
  DiGLPainter::GLuint *buffer_data;
};

#endif // DIOPENGLWIDGET_H
