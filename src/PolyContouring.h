
#ifndef POLY_CONTOURING_HH
#define POLY_CONTOURING_HH 1

#include "reversible_list.hh"

#include <cassert>
#include <utility>
#include <vector>

namespace contouring {

struct Point {
    float x, y;
    Point() : x(0), y(0) { }
    Point(float xx, float yy)
        : x(xx), y(yy) { }
};

typedef reversible_list<Point> Polyline;

class PolylineExtender;

class TaggedPolyline {
public:
    TaggedPolyline()
        : mExtenderFront(0), mExtenderBack(0) { }
    TaggedPolyline(PolylineExtender* f)
        : mExtenderFront(f), mExtenderBack(0) { }

    void attach(PolylineExtender* end)
        { if (mExtenderFront==0) mExtenderFront = end; else { assert(not mExtenderBack); mExtenderBack = end; } }
    bool detach(PolylineExtender* ex)
        { if (isFront(ex)) mExtenderFront = 0; else mExtenderBack = 0; return (mExtenderFront == 0 and mExtenderBack == 0); }

    bool isFront(const PolylineExtender* ex) const
        { return ex == mExtenderFront; }
    bool isBack(const PolylineExtender* ex) const
        { return ex == mExtenderBack; }

    PolylineExtender* otherEnd(const PolylineExtender* end) const
        { if (isFront(end)) return mExtenderBack; else return mExtenderFront; }
    void setOtherEnd(PolylineExtender* end, PolylineExtender* oe)
        { if (isFront(end)) mExtenderBack = oe; else mExtenderFront = oe; }
    void setThisEnd(PolylineExtender* end, PolylineExtender* te)
        { if (isFront(end)) mExtenderFront = te; else mExtenderBack = te; }

    void reverse()
        { mPoints.reverse(); std::swap(mExtenderFront, mExtenderBack); }

    bool isBorder() const
        { return mExtenderBack == 0 or mExtenderFront == 0; }

    void addPoint(PolylineExtender* end, const Point& p)
        { if (end == mExtenderFront) mPoints.push_front(p); else mPoints.push_back(p); }
    void addPoint(const Point& p)
        { mPoints.push_back(p); }

    Polyline& points()
        { return mPoints; }

private:
    PolylineExtender *mExtenderFront, *mExtenderBack;
    Polyline mPoints;
};

// ########################################################################

class PolylineExtender {
public:
    PolylineExtender()
        : mPolyline(0) { }
    ~PolylineExtender()
        { stopLine(); }

    void swap(PolylineExtender& o)
        { if (mPolyline) mPolyline->setThisEnd(this, &o); if (o.mPolyline) o.mPolyline->setThisEnd(&o, this); std::swap(mPolyline, o.mPolyline); }

    operator bool() const
        { return mPolyline != 0; }

    void startLine(const Point& p0)
        { assert(not mPolyline); mPolyline = new TaggedPolyline(this); mPolyline->addPoint(p0); }

    void startLine(PolylineExtender& oe, const Point& p0, const Point& p1)
        { startLine(p0); mPolyline->addPoint(p1);
            assert(not oe.mPolyline); oe.mPolyline = mPolyline; mPolyline->setOtherEnd(this, &oe); }

    void stopLine()
        { if (mPolyline and mPolyline->detach(this)) delete mPolyline; mPolyline = 0; }

    PolylineExtender* otherEnd() const
        { return mPolyline->otherEnd(this); }

    bool isFront() const
        { return mPolyline->isFront(this); }
    bool isBack() const
        { return mPolyline->isBack(this); }
    void reverse()
        { mPolyline->reverse(); }

    bool isBorder() const
        { return mPolyline->isBorder(); }

    void addPoint(const Point& p)
        { mPolyline->addPoint(this, p); }
    Polyline& points()
        { return mPolyline->points(); }

    bool sameLine(PolylineExtender& o)
        { return mPolyline == o.mPolyline; }

private:
    TaggedPolyline* mPolyline;
};

// ########################################################################

class Field {
public:
    virtual ~Field() { }

    enum { UNDEFINED = -32767 };

    virtual int nx() const = 0;
    virtual int ny() const = 0;
    virtual int nlevels() const = 0;
    virtual int level_point(int ix, int iy) const = 0;
    virtual int level_center(int cx, int cy) const = 0;
    virtual Point point(int levelIndex, int x0, int y0, int x1, int y1) const = 0;
};

// ########################################################################

class PolyContouring {
public:
    PolyContouring(Field* field)
        : mField(field) { }
    virtual ~PolyContouring() { }

    void makeLines();

protected:
    virtual void emitLine(int level, Polyline& points, bool close);

private:
    void actionCloseLB(int li, PolylineExtender& pl, PolylineExtender& pb);
    void actionJoinBRLT(PolylineExtender& pl, PolylineExtender& pb,
                        const Point& right, const Point& top);
    void actionJoinBT(PolylineExtender& pb, const Point& top);
    void actionJoinLR(PolylineExtender& pl, const Point& right);
    void actionOpenRT(PolylineExtender& pl, PolylineExtender& pb,
                      const Point& right, const Point& top);


    void endLineAtBorder(PolylineExtender& p, int li);
    bool minmaxLevel(int levelIndex, int& l_mini, int& l_maxi, bool& have_def);

protected:
    Field* mField;
};

} // namespace contouring

#endif // POLY_CONTOURING_HH
