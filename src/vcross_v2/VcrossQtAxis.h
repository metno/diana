
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
  enum Quantity { TIME, DISTANCE, ALTITUDE, PRESSURE };
  
  Axis(bool h);

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

  void setLabel(const std::string& l)
    { mLabel = l; }
  const std::string& label() const
    { return mLabel; }

  bool setType(const std::string& t);
  Type type() const
    { return mType; }
  bool setQuantity(const std::string& q);
  Quantity quantity() const
    { return mQuantity; }

  bool increasing() const;

private:
  float fDataMin() const
    { return function(dataMin); }
  float fDataMax() const
    { return function(dataMax); }
  float fValueMin() const
    { return function(valueMin); }
  float fValueMax() const
    { return function(valueMax); }
  void calculateScale();

  float function(float x) const;
  float functionInverse(float x) const;

private:
  bool horizontal;
  Type mType;
  Quantity mQuantity;
  std::string mLabel;

  float dataMin, dataMax;
  float valueMin, valueMax;
  float paintMin, paintMax;
  float mScale;
};

typedef boost::shared_ptr<Axis> AxisPtr;
typedef boost::shared_ptr<const Axis> AxisCPtr;

} // namespace detail
} // namespace vcross

#endif // VCROSS_DIVCROSSAXIS_HH
