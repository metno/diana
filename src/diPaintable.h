
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

  DiPaintable();
  virtual ~DiPaintable() { }

  virtual void setCanvas(DiCanvas* canvas);
  DiCanvas* canvas() const
    { return mCanvas; }

  virtual void resize(int width, int height) = 0;
  virtual void paintUnderlay(DiPainter* painter) = 0;
  virtual void paintOverlay(DiPainter* painter) = 0;
  void requestBackgroundBufferUpdate()
    { update_background_buffer = true; }

  virtual bool handleKeyEvents(QKeyEvent*) { return false; }
  virtual bool handleMouseEvents(QMouseEvent*) { return false; }
  virtual bool handleWheelEvents(QWheelEvent*) { return false; }

public:
  bool enable_background_buffer;
  bool update_background_buffer;

private:
  DiCanvas* mCanvas;
};

#endif // DIPAINTABLE_H
