#ifndef DIPAINTGLWIDGET_H
#define DIPAINTGLWIDGET_H 1

#include "diPaintGLPainter.h"
#include <QWidget>
#include <memory>

class DiPaintable;
class QImage;

class DiPaintGLWidget : public QWidget
{
  Q_OBJECT;

public:
  DiPaintGLWidget(DiPaintable* paintable, QWidget *parent,
      bool antialiasing = false);
  ~DiPaintGLWidget();

public Q_SLOTS:
  void updateGL();

protected:
  void paintEvent(QPaintEvent* event);
  void resizeEvent(QResizeEvent* event);
  void keyPressEvent(QKeyEvent *ke);
  void keyReleaseEvent(QKeyEvent *ke);
  void mousePressEvent(QMouseEvent* me);
  void mouseMoveEvent(QMouseEvent* me);
  void mouseReleaseEvent(QMouseEvent* me);
  void mouseDoubleClickEvent(QMouseEvent* me);
  void wheelEvent(QWheelEvent *we);

private:
  void paint(QPainter& painter);
  void dropBackgroundBuffer();

protected:
  std::unique_ptr<DiPaintGLCanvas> glcanvas;
  std::unique_ptr<DiPaintGLPainter> glpainter;
  DiPaintable* paintable;
  QImage* background_buffer;
  bool antialiasing;
};

#endif // DIPAINTGLWIDGET_H
