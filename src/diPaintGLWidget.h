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

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent *ke) override;
  void keyReleaseEvent(QKeyEvent *ke) override;
  void mousePressEvent(QMouseEvent* me) override;
  void mouseMoveEvent(QMouseEvent* me) override;
  void mouseReleaseEvent(QMouseEvent* me) override;
  void mouseDoubleClickEvent(QMouseEvent* me) override;
  void wheelEvent(QWheelEvent *we) override;

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
