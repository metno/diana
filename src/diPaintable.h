
#ifndef DIPAINTABLE_H
#define DIPAINTABLE_H 1

class DiCanvas;
class DiPainter;

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QWidget;

class DiPaintable {
public:
  static QWidget* createWidget(DiPaintable* p, QWidget* parent);

  virtual ~DiPaintable() { }

  virtual void setCanvas(DiCanvas* canvas) = 0;
  virtual void paint(DiPainter* painter) = 0;
  virtual void resize(int width, int height) = 0;

  virtual bool handleKeyEvents(QKeyEvent*) { return false; };
  virtual bool handleMouseEvents(QMouseEvent*) { return false; };
  virtual bool handleWheelEvents(QWheelEvent*) { return false; };
};

#endif // DIPAINTABLE_H
