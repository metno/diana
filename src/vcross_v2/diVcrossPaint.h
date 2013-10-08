
#ifndef DIANA_VCROSS_DIVCROSSPAINT_H
#define DIANA_VCROSS_DIVCROSSPAINT_H 1

/*!
 * Helper class for painting wind arrows.
 */
struct PaintWindArrow {
  bool mWithArrowHead;

  float mSize;

  PaintWindArrow();

  /*! Paint a wind arrow with x-component u and y-component v at pixel position (px, py).
   * \param u u-component, painted horizontally
   * \param v v-component, painted vertically
   * \param px pixel x position
   * \param py pixel y position
   */
  void paint(float u, float v, float px, float py) const;
};

// ########################################################################

/*!
 * Helper class for painting vectors.
 */
struct PaintVector {
  bool mWithArrowHead;
  float mScaleX, mScaleY;

  /*! Prepare for painting vectors.
   * No other painting may occur while this object exists (as it calls geBegin).
   */
  PaintVector();

  /*! Finish painting vectors. */
  ~PaintVector();

  /*! Paint a wind arrow with u,v components at pixel position (px, py).
   * \param u u-component, painted horizontally
   * \param v v-component, painted vertically
   * \param px pixel x position
   * \param py pixel y position
   */
  void paint(float u, float v, float px, float py) const;
};

#endif // DIANA_VCROSS_DIVCROSSPAINT_H
