
#ifndef diPolyContouring_hh
#define diPolyContouring_hh 1

#include <string>

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
  bool omit(contouring::level_t l) const { return l == OMIT_LEVEL; }
  enum { UNDEF_LEVEL = -10000000, OMIT_LEVEL = -10000001 };
};
typedef boost::shared_ptr<DianaLevels> DianaLevels_p;

DianaLevels_p dianaLevelsForPlotOptions(const PlotOptions& mPoptions, float fieldUndef);

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

inline int find_index(bool repeat, int available, int i)
{ if (repeat) return i % available; else return std::min(i, available-1); }

bool poly_contour(int nx, int ny, int ix0, int iy0, int ix1, int iy1,
    const float z[], const float xz[], const float yz[],
    FontManager* fp, const PlotOptions& poptions, float fieldUndef);

#endif // diPolyContouring_hh
