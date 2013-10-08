
#include "PolyContouring.h"

#include <vector>

#ifndef MILOGGER_CATEGORY
#define MILOGGER_CATEGORY "contouring"
#endif
#include <miLogger/miLogging.h>

namespace contouring {

void PolyContouring::emitLine(int, Polyline&, bool)
{
}

void PolyContouring::actionCloseLB(int li, PolylineExtender& p1, PolylineExtender& p2)
{
    assert(p1.isFront() or p1.isBack());
    assert(p2.isFront() or p2.isBack());

    const bool bothBorder = p1.isBorder() and p2.isBorder();
    if (not bothBorder and p1.sameLine(p2)) {
        // both have same line => must be inside, close and emit
        assert(not (p1.isBorder() or p2.isBorder()));
        emitLine(li, p1.points(), true);
    } else {
        if (not p1.isBack())
            p1.reverse();

        if (p2.isBack())
            p2.reverse();

        assert(p1.isBack() and p2.isFront());

        p1.points().move_back(p2.points());
        
        if (bothBorder) {
            // both border => join and emit
            emitLine(li, p1.points(), false);
        } else {
            // one border
            PolylineExtender* p2o = p2.otherEnd();
            if (p2o)
                p1.swap(*p2o);
        }
    }
    p1.stopLine();
    p2.stopLine();
}

void PolyContouring::actionJoinBRLT(PolylineExtender& pl, PolylineExtender& pb,
                                    const Point& right, const Point& top)
{
    if (pb)
        pb.addPoint(right);

    if (pl)
        pl.addPoint(top);

    pl.swap(pb);
}

void PolyContouring::actionJoinBT(PolylineExtender& pb, const Point& top)
{
    pb.addPoint(top);
}

void PolyContouring::actionJoinLR(PolylineExtender& pl, const Point& right)
{
    pl.addPoint(right);
}

void PolyContouring::actionOpenRT(PolylineExtender& pl, PolylineExtender& pb,
                                  const Point& right, const Point& top)
{
    pl.startLine(pb, right, top);
}

void PolyContouring::endLineAtBorder(PolylineExtender& p, int li)
{
    if (p and p.isBorder() and p.points().size() > 1) {
        emitLine(li, p.points(), false);
        p.points().clear();
    }
    p.stopLine();
}

bool PolyContouring::minmaxLevel(int levelIndex, int& l_min, int& l_max, bool& have_def)
{
    if (levelIndex == Field::UNDEFINED)
        return true;

    if (not have_def) {
        l_min = l_max = levelIndex;
    } else {
        l_min = std::min(levelIndex, l_min);
        l_max = std::max(levelIndex, l_max);
    }
    have_def = true;
    return false;
}

void PolyContouring::makeLines()
{
    const int NROW = mField->ny(), NCELLY = NROW-1, NCOL = mField->nx(), NCELLX = NCOL-1, NLEVEL = mField->nlevels();

    // list of levels for corner points on the bottom
    std::vector<int> levl_bot(NCOL);
    for (int ix=0; ix < NCOL; ++ix)
        levl_bot[ix] = mField->level_point(ix, 0);

    // list of polygons incoming from the left from outside the field
    typedef std::vector<PolylineExtender> poly_bot_t ;
    std::vector<poly_bot_t> poly_bot(NCELLX, poly_bot_t(NLEVEL));
    poly_bot_t poly_lft(NLEVEL);

    // polygons incoming from the bottom from outside the field
    for (int ix=0; ix < NCELLX; ++ix) {
        const int levl_bl = levl_bot[ix], levl_br = levl_bot[ix+1];

        int levl_min = 0, levl_max = 0;
        bool have_def = false;
        const bool undef_l = minmaxLevel(levl_bl, levl_min, levl_max, have_def);
        const bool undef_r = minmaxLevel(levl_br, levl_min, levl_max, have_def);
        if (not (undef_l or undef_r))
            for (int li=levl_min; li < NLEVEL and li < levl_max; li += 1)
                poly_bot[ix][li].startLine(mField->point(li, ix, 0, ix+1, 0));
    }

    // for each row of cells
    for (int iy=0; iy < NCELLY; ++iy) {
        // level at top left of cell
        int levl_tl = mField->level_point(0, iy+1);
        // level at bottom left of cell
        int levl_bl = levl_bot[0];

        // incoming lines at left of column
        {
            int levl_min = 0, levl_max = 0;
            bool have_def = false;
            const bool undef_b = minmaxLevel(levl_bl, levl_min, levl_max, have_def);
            const bool undef_t = minmaxLevel(levl_tl, levl_min, levl_max, have_def);
            if (not (undef_b or undef_t))
                for (int li=levl_min; li < NLEVEL and li < levl_max; li += 1)
                    poly_lft[li].startLine(mField->point(li, 0, iy, 0, iy+1));
        }

        // walk cells in row from left to right
        int levl_tr /* avoid warning from compiler */ = 0;
        for (int ix=0; ix < NCELLX; ++ix) {
            levl_tr = mField->level_point(ix+1, iy+1);
            const int levl_br = levl_bot[ix+1];

            int levl_min = 0, levl_max = 0;
            bool have_def = false;
            const bool undef_br = minmaxLevel(levl_br, levl_min, levl_max, have_def);
            const bool undef_tr = minmaxLevel(levl_tr, levl_min, levl_max, have_def);
            const bool undef_bl = minmaxLevel(levl_bl, levl_min, levl_max, have_def);
            const bool undef_tl = minmaxLevel(levl_tl, levl_min, levl_max, have_def);

            if (have_def) {
                for (int li=levl_min; li < NLEVEL and li<levl_max; ++li) {
                    PolylineExtender &pb = poly_bot[ix][li], &pl = poly_lft[li];

                    const bool c_top   = (levl_tl <= li) != (levl_tr <= li) // cross out on top
                        and not (undef_tl or undef_tr);
                    const bool c_right = (levl_tr <= li) != (levl_br <= li) // cross out on right
                        and not (undef_tr or undef_br);
                    const bool c_bottom = (pb); // cross in at bottom
                    const bool c_left   = (pl); // cross in on left

                    Point ptop, pright;
                    if (c_top)
                        ptop = mField->point(li, ix, iy+1, ix+1, iy+1);
                    if (c_right)
                        pright = mField->point(li, ix+1, iy, ix+1, iy+1);

                    if (c_bottom and c_left and c_top and c_right) {
                        // saddle point
                        const int levl_saddle = mField->level_center(ix, iy);
                        if ((levl_saddle <= li) != (levl_tl <= li)) {
                            actionCloseLB(li, pl, pb);
                            actionOpenRT(pl, pb, pright, ptop);
                        } else {
                            actionJoinBRLT(pl, pb, pright, ptop);
                        }
                    } else if (c_bottom and c_left /*and not (c_top or c_right)*/) {
                        actionCloseLB(li, pl, pb);
                    } else if ((c_bottom and c_right) or (c_left and c_top)) {
                        actionJoinBRLT(pl, pb, pright, ptop);
                    } else if (c_bottom and c_top) {
                        actionJoinBT(pb, ptop);
                    } else if (c_left and c_right) {
                        actionJoinLR(pl, pright);
                    } else if (c_top and c_right) {
                        actionOpenRT(pl, pb, pright, ptop);
                    } else {
                        if (c_left)
                            endLineAtBorder(pl, li);
                        if (c_right)
                            poly_lft[li].startLine(pright);
                        if (c_bottom)
                            endLineAtBorder(pb, li);
                        if (c_top)
                            poly_bot[ix][li].startLine(ptop);
                    }
                }
            }

            levl_bot[ix] = levl_tl;
            levl_tl = levl_tr;
            levl_bl = levl_br;
        }
        levl_bot[NCELLX] = levl_tr;

        // finished one row, polylines going out at the right must be emitted
        for (int li=0; li<NLEVEL; ++li)
            endLineAtBorder(poly_lft[li], li);
    }

    // finished all rows, polylines going out to the top must be emitted
    for (int ix=0; ix < NCELLX; ++ix) {
        for (int li=0; li<NLEVEL; ++li)
            endLineAtBorder(poly_bot[ix][li], li);
    }
}

} // namespace contouring
