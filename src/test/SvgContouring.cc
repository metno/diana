
#include "PolyContouring.h"

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class SVG {
public:
  SVG(const std::string& filename);
  ~SVG();

  void polyline(const contouring::Polyline& points, const std::string& stroke = "black");
  void comment(const std::string& text);
  void rect_corners(float x0, float y0, float x1, float y1);

private:
  std::ofstream svg;
};


SVG::SVG(const std::string& filename)
  : svg(filename.c_str())
{
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
}

SVG::~SVG()
{
  svg << "</svg>\n";
}

void SVG::polyline(const contouring::Polyline& points, const std::string& stroke)
{
  svg << '<';
  if (points.size() >= 3
      and std::fabs(points.front().x - points.back().x) < 1e-3
      and std::fabs(points.front().y - points.back().y) < 1e-3)
  {
    svg << "polygon";
  } else {
    svg << "polyline";
  }
  svg << " style=\"fill:none;stroke:" << stroke << ";stroke-width:1;stroke-linejoin:bevel\" points=";
  char sep = '"';
  BOOST_FOREACH(const contouring::Point& p, points) {
    svg << sep << (boost::format("%.1f,%.1f") % p.x % p.y).str();
    sep = ' ';
  }
  svg << "\" />\n";
}

void SVG::comment(const std::string& text)
{
  svg << "<!-- " << text << " -->\n";
}

void SVG::rect_corners(float x0, float y0, float x1, float y1)
{
  svg << (boost::format("<rect x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\" fill=\"white\" stroke=\"none\"/>\n")
      % x0 % y0 % (x1-x0) % (y1-y0)).str();
}

// ========================================================================

class ArrayField : public contouring::Field {
public:
  ArrayField(int nx, int ny, float* data)
    : mNX(nx), mNY(ny), mData(data), mScale(1) { }
    
  ArrayField(const std::string& filename);

  ~ArrayField()
    { delete[] mData; }

  virtual int nx() const
    { return mNX; }

  virtual int ny() const
    { return mNY; }

  virtual int nlevels() const
    { return mLevels.size(); }

  virtual contouring::Point point(int levelIndex, int x0, int y0, int x1, int y1) const;
  virtual int level_point(int ix, int iy) const;
  virtual int level_center(int cx, int cy) const;

  const std::vector<float>& levels() const
    { return mLevels; }

  std::vector<float>& levels()
    { return mLevels; }

  float scale() const
    { return mScale; }

private:
  float value(int ix, int iy) const;
  float& lvalue(int ix, int iy);
  contouring::Point position(int ix, int iy) const;

private:
  std::vector<float> mLevels;

  int mNX, mNY;
  float* mData;
  float mScale;
};

const float UNDEF_VALUE = 1e30;

inline bool isUndefined(float v)
{
  return v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

ArrayField::ArrayField(const std::string& filename)
  : mScale(10)
{
  std::ifstream file(filename.c_str());
  std::string line;
  int UP = 1;
  while (std::getline(file, line)) {
    if (line.empty() or line.at(0) == '#')
      continue;

    if (line.at(0) >= 'a' and line.at(0) <= 'z') {
      std::istringstream iline(line);
      std::string key;
      iline >> key;
      if (key == "level-range") {
        float start, stop, step;
        iline >> start >> stop >> step;
        for (float l=start; (step > 0 and l<stop) or (step < 0 and l>stop); l += step)
          mLevels.push_back(l);
      } else if (key == "level-list") {
        float level;
        do {
          iline >> level;
          if (iline)
            mLevels.push_back(level);
        } while (iline);
      } else if (key == "nx") {
        iline >> mNX;
      } else if (key == "ny") {
        iline >> mNY;
      } else if (key == "scale_up") {
        iline >> UP;
      } else if (key == "data") {
        break;
      }
    }
  }

  mData = new float[mNX*mNY];
  for(int iy=0; iy<mNY; iy++)
    for(int ix=0; ix<mNX; ix++)
      file >> lvalue(ix, iy);

  if (UP > 1) {
    const int nx = (mNX-1)*UP, ny = (mNY-1)*UP;
    float* data = new float[nx*ny];
    for(int iy=0; iy<mNY-1; iy++) {
      for(int ix=0; ix<mNX-1; ix++) {
        const float vbl = value(ix,   iy);
        const float vtl = value(ix,   iy+1);
        const float vbr = value(ix+1, iy);
        const float vtr = value(ix+1, iy+1);
        for (int dy=0; dy<UP; ++dy) {
          for (int dx=0; dx<UP; ++dx) {
            const float rx = dx/float(UP), ry = dy/float(UP);
            const float v = (1-rx)*(1-ry)*vbl + (1-rx)*ry*vtl + rx*(1-ry)*vbr + rx*ry*vtr;
            data[(ix*UP + dx)*ny + (iy*UP + dy)] = v;
          }
        }
      }
    }
    delete[] mData;
    mData = data;
    mNX = nx;
    mNY = ny;
    mScale /= UP;
  }
}

float ArrayField::value(int ix, int iy) const
{
  return mData[ix*mNY + iy];
}

float& ArrayField::lvalue(int ix, int iy)
{
  return mData[ix*mNY + iy];
}

contouring::Point ArrayField::position(int ix, int iy) const
{
  return contouring::Point(mScale*ix, mScale*iy);
}

contouring::Point ArrayField::point(int levelIndex, int x0, int y0, int x1, int y1) const
{
  const float v0 = value(x0, y0);
  const float v1 = value(x1, y1);
  const float c = (mLevels[levelIndex]-v0)/(v1-v0);

  const contouring::Point p0 = position(x0, y0);
  const contouring::Point p1 = position(x1, y1);
  const float x = (1-c)*p0.x + c*p1.x;
  const float y = (1-c)*p0.y + c*p1.y;
  return contouring::Point(x, y);
}

int ArrayField::level_point(int ix, int iy) const
{
  const float v = value(ix, iy);
  if (isUndefined(v))
    return UNDEFINED;
  return std::lower_bound(mLevels.begin(), mLevels.end(), value(ix, iy)) - mLevels.begin();
}

int ArrayField::level_center(int cx, int cy) const
{
  const float v_00 = value(cx, cy), v_10 = value(cx+1, cy), v_01 = value(cx, cy+1), v_11 = value(cx+1, cy+1);
  if (isUndefined(v_00) or isUndefined(v_01) or isUndefined(v_10) or isUndefined(v_11))
    return UNDEFINED;
    
  const float avg = 0.25*(v_00 + v_01 + v_10 + v_11);
  return std::lower_bound(mLevels.begin(), mLevels.end(), avg) - mLevels.begin();
}

// ========================================================================

class LevelLineContouring : public contouring::PolyContouring {
public:
  LevelLineContouring(contouring::Field* field)
    : PolyContouring(field), mLines(field->nlevels()) { }

  virtual void emitLine(int li, contouring::Polyline& points, bool close);

  const std::vector<contouring::Polyline>& lines4level(int li) const
    { return mLines[li]; }

private:
  std::vector< std::vector<contouring::Polyline> > mLines;
};

void LevelLineContouring::emitLine(int li, contouring::Polyline& points, bool close)
{
  mLines[li].push_back(contouring::Polyline());
  contouring::Polyline& p = mLines[li].back();
  p.swap(points);
  if (close)
    p.push_back(p.front());
}

// ########################################################################

const std::string l2color(const std::vector<float>& levels, float lvl)
{
  float fraction;
  if (levels.size() >= 2 and abs(levels.back() - levels.front()) >= 1e-3)
    fraction = (lvl - levels[0])/(levels.back() - levels.front());
  else
    fraction = 0.5;
  return (boost::format("#%1$02x00%2$02x") % int(255*fraction) % (0x80-int(0x80*fraction))).str();
}

int main(int argc, char* argv[])
{
  if (argc != 3)
    return 1;

  const std::string infile = argv[1], outfile = argv[2];

  boost::shared_ptr<ArrayField> field = boost::make_shared<ArrayField>(infile);

  boost::shared_ptr<SVG> svg = boost::make_shared<SVG>(outfile);
  svg->rect_corners(0, 0, field->scale()*(field->nx()-1), field->scale()*(field->ny()-1));

  LevelLineContouring llc(field.get());
  llc.makeLines();

#if 1
  for (unsigned int li=0; li<field->levels().size(); ++li) {
    const float level = field->levels()[li];
    const std::string stroke = l2color(field->levels(), level);
    svg->comment((boost::format("lines for level %1$5.1f") % level).str());
    BOOST_FOREACH(const contouring::Polyline& p, llc.lines4level(li)) {
      svg->polyline(p, stroke);
    }
  }
#endif

  return 0;
}
