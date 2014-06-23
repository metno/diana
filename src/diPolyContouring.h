
#ifndef diPolyContouring_hh
#define diPolyContouring_hh 1

#include <string>
#include <vector>

#include "poly_contouring.hh"

class Area;
class FontManager;
class GLPfile;
class PlotOptions;

class DianaLevels {
public:
  virtual ~DianaLevels() { }
  virtual contouring::level_t level_for_value(float value) const = 0;
  virtual float value_for_level(contouring::level_t l) const = 0;
  enum { UNDEF_LEVEL = -10000000 };
};
typedef boost::shared_ptr<DianaLevels> DianaLevels_p;

DianaLevels_p dianaLevelsForPlotOptions(const PlotOptions& mPoptions, float fieldUndef);

//------------------------------------------------------------------------

class DianaLevelList : public DianaLevels {
public:
  DianaLevelList(const std::vector<float>& levels);
  DianaLevelList(float lstart, float lstop, float lstep);
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const;
  size_t nlevels() const
    { return mLevels.size(); }
protected:
  DianaLevelList();
protected:
  std::vector<float> mLevels;
};

// ------------------------------------------------------------------------

class DianaLevelList10 : public DianaLevelList {
public:
  DianaLevelList10(const std::vector<float>& levels);
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const;
  enum { BASE = 10 };
};

//------------------------------------------------------------------------

class DianaLevelStep : public DianaLevels {
public:
  DianaLevelStep(float step, float off)
    : mStep(step), mOff(off), mMin(1), mMax(0) { }
  void set_limits(float mini, float maxi)
    { mMin = mini; mMax = maxi; }
  virtual contouring::level_t level_for_value(float value) const;
  virtual float value_for_level(contouring::level_t l) const;
protected:
  float mStep, mOff, mMin, mMax;
};

//------------------------------------------------------------------------

class DianaLevelStepOmit : public DianaLevelStep {
public:
  DianaLevelStepOmit(float step, float off)
    : DianaLevelStep(step, off) { }
  contouring::level_t level_for_value(float value) const;
};

// ########################################################################

class DianaPositions {
public:
  virtual ~DianaPositions() { }
  virtual contouring::point_t position(size_t ix, size_t iy) const = 0;
};

typedef boost::shared_ptr<DianaPositions> DianaPositions_p;

// ########################################################################

class DianaFieldBase : public contouring::field_t {
public:
  DianaFieldBase(const DianaLevels& levels, const DianaPositions& positions)
    : mLevels(levels), mPositions(positions) { }
  
  virtual contouring::level_t grid_level(size_t ix, size_t iy) const
    { return mLevels.level_for_value(value(ix, iy)); }
  virtual contouring::point_t line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const;
  virtual contouring::point_t grid_point(size_t x, size_t y) const
    { return position(x, y); }

  contouring::level_t undefined_level() const
    { return DianaLevels::UNDEF_LEVEL; }
  
protected:
  virtual float value(size_t ix, size_t iy) const = 0;

  contouring::point_t position(size_t ix, size_t iy) const
    { return mPositions.position(ix, iy); }

  const DianaPositions& positions() const
    { return mPositions; }

private:
  const DianaLevels& mLevels;
  const DianaPositions& mPositions;
};

// ########################################################################

int find_index(bool repeat, int available, int i);

bool poly_contour(int nx, int ny, int ix0, int iy0, int ix1, int iy1,
    const float z[], const float xz[], const float yz[],
    FontManager* fp, const PlotOptions& poptions, float fieldUndef);

#endif // diPolyContouring_hh
