
#ifndef DIANA_VCROSS_DIVCROSSPAINT_H
#define DIANA_VCROSS_DIVCROSSPAINT_H 1

class QPainter;

namespace vcross {

struct PaintArrow {
  virtual ~PaintArrow() { }

  virtual bool withArrowHead() const = 0;

  virtual float size() const = 0;

  /*! Paint an arrow with x-component u and y-component v at pixel position (px, py).
   * \param u u-component, painted horizontally
   * \param v v-component, painted vertically
   * \param px pixel x position
   * \param py pixel y position
   */
  virtual void paint(QPainter& painter, float u, float v, float px, float py) const = 0;
};

// ########################################################################

/*!
 * Helper class for painting wind arrows.
 */
struct PaintWindArrow : public PaintArrow {
  PaintWindArrow();

  virtual bool withArrowHead() const
    { return mWithArrowHead; }
  virtual float size() const
    { return mSize; }

  /*! Paint a wind arrow with x-component u and y-component v at pixel position (px, py).
   * \param u u-component, painted horizontally
   * \param v v-component, painted vertically
   * \param px pixel x position
   * \param py pixel y position
   */
  virtual void paint(QPainter& painter, float u, float v, float px, float py) const;

  bool mWithArrowHead;
  float mSize;
};

// ########################################################################

/*!
 * Helper class for painting vectors.
 */
struct PaintVector : public PaintArrow {

  /*! Prepare for painting vectors.
   * No other painting may occur while this object exists (as it calls geBegin).
   */
  PaintVector();

  /*! Finish painting vectors. */
  ~PaintVector();

  virtual bool withArrowHead() const
    { return true; }
  virtual float size() const
    { return mScale*4; }

  /*! Paint a wind arrow with u,v components at pixel position (px, py).
   * \param u u-component, painted horizontally
   * \param v v-component, painted vertically
   * \param px pixel x position
   * \param py pixel y position
   */
  virtual void paint(QPainter& painter, float u, float v, float px, float py) const;

  static void paintArrow(QPainter& painter, float px, float py, float ex, float ey);

  void setScaleXY(float sx, float sy);

  void setScale(float sxy);

  void setThickArrowScale(float tas)
    { mThickArrowScale = tas; }

  float mScale, mScaleX, mScaleY;
  float mThickArrowScale;
};

} // namespace vcross

#endif // DIANA_VCROSS_DIVCROSSPAINT_H
