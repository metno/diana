
#ifndef VCROSS_DIVCROSSAXIS_HH
#define VCROSS_DIVCROSSAXIS_HH 1

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace vcross {
namespace detail {

struct Axis {
  typedef std::vector<std::string> string_v;
  typedef std::vector<string_v> labels_t;
  enum Type { LINEAR, EXNER };
  enum Quantity { TIME, DISTANCE, HEIGHT, PRESSURE };
  
  void setDataRange(float mi, float ma)
    { dataMin = mi; dataMax = ma; }
  void setValueRange(float mi, float ma)
    { valueMin = mi; valueMax = ma; calculateScale(); }
  void setPaintRange(float mi, float ma)
    { paintMin = mi; paintMax = ma; calculateScale(); }

  float getDataMin() const
    { return dataMin; }
  float getDataMax() const
    { return dataMax; }
  float getValueMin() const
    { return valueMin; }
  float getValueMax() const
    { return valueMax; }
  float getPaintMin() const
    { return paintMin; }
  float getPaintMax() const
    { return paintMax; }

  bool zoomIn(float paint0, float paint1);
  bool zoomOut();
  bool pan(float delta);

  float value2paint(float v, bool check=true) const;
  float paint2value(float p, bool check=true) const;
  bool legalPaint(float p) const;
  bool legalValue(float v) const;
  bool legalData(float d) const;

  bool horizontal;
  Type type;
  Quantity quantity;
  std::string label;

  Axis(bool h)
    : horizontal(h), type(LINEAR), quantity(h ? DISTANCE : PRESSURE), valueMin(0), valueMax(1), paintMin(0), paintMax(1), scale(1) { }

private:
  void calculateScale();

  float function(float x) const;
  float functionInverse(float x) const;

private:
  float dataMin, dataMax;
  float valueMin, valueMax;
  float paintMin, paintMax;
  float scale;
};

typedef boost::shared_ptr<Axis> AxisPtr;
typedef boost::shared_ptr<const Axis> AxisCPtr;

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSAXIS_HH
