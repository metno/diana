/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2014-2023 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "poly_contouring.h"

#include <cassert>
#include <list>
#include <vector>

#include <iostream>
#include <sstream>

//#define LENGTH_STATS 1

//#define CONTOURING_ENABLE_DEBUG 1

#ifdef USE_ALLOCATOR
#include <boost/pool/object_pool.hpp>
#endif

#ifdef CONTOURING_ENABLE_DEBUG
#include <iostream>
extern size_t debug_level;
void debug_indent(char c);
#define POCO_IFDEBUG(x) do { x; } while(false)
#define P(x) std::cout << x
#define POCO_DEBUG(x) do {                      \
        debug_indent('_');                      \
        x ; P(std::endl);                       \
    } while(false)
#define LV(x) #x << "='" << x << "' "
#define DUMP_L(t,x)  P("[" t); if(x) { P("l=" << x->get_level() << ",id=" << x->m_line_id); } else { P("%"); } \
    /* << " f="); if(x->front()) P(x->front()->m_end_id); else P('%'); */ \
    /* P(" b="); if (x->back()) P(x->back()->m_end_id); else P('%'); */ P(']')
#define DUMP_LE(t,x) P("{" t "@e@"); if (x) { P(x->m_end_id); DUMP_L("", x->get_line()); } else P("%"); P('}')
#define DUMP_TR(t,x) P("<" t "@t@" << x->m_triplet_id << '('); DUMP_LE("o", (x)->over()); P(' '); \
    DUMP_LE("c", (x)->ctour()); P(' '); DUMP_LE("u", (x)->under()); P('>')
#define SEGFAULT *((char*)0) = 7
#else
#define POCO_IFDEBUG(x) do { } while(false)
#define P(x)
#define POCO_DEBUG(x) do { } while(false)
#define LV(x)
#define DUMP_L(t,x)
#define DUMP_LE(t,x)
#define DUMP_TR(t,x)
#define SEGFAULT while(false) { }
#endif

#ifdef CONTOURING_ENABLE_DEBUG
class debug_enter {
public:
    debug_enter(const char* n) : name(n) { debug_level += 1; debug_indent('+'); P("enter " << name << std::endl); }
    ~debug_enter() { debug_indent('-'); P("leave " << name << std::endl); debug_level -= 1; }
private:
    const char* name;
};
#define POCO_DEBUG_SCOPE() debug_enter debug_enter_scope(__FUNCTION__)
#define FAIL(x) do { POCO_DEBUG(P(x)); SEGFAULT; } while (false)
#else
#define POCO_DEBUG_SCOPE()
#define FAIL(x) do { } while (false)
#endif


#ifdef CONTOURING_ENABLE_DEBUG
size_t debug_level = 0;
void debug_indent(char c)
{
    for (size_t i=0; i<debug_level; ++i)
        std::cout << c << c;
    std::cout << ' ';
}
#endif // CONTOURING_ENABLE_DEBUG

#ifdef LENGTH_STATS
#include <fstream>
#include <cstring>
static const size_t MAX_JS = 2000;
static size_t join_size[MAX_JS+1];
static void clear_join_size()
{
    memset(join_size, 0, sizeof(join_size));
    std::ifstream jsin("/tmp/join_sizes.dat");
    while (jsin) {
        size_t i = 0, n = 0;
        jsin >> i >> n;
        if (not jsin)
            break;
        if (i < MAX_JS)
            join_size[i] = n;
    }

}
static void inc_join_size(size_t js)
{
    if (js < MAX_JS)
        join_size[js] += 1;
    else
        join_size[MAX_JS] += 1;
}
static void dump_join_size()
{
    std::ofstream jsout("/tmp/join_sizes.dat");
    //jsout << "~~~~~~~~~~~~~~~~~~~~" << std::endl;
    for (size_t i=0; i<MAX_JS+1; ++i)
        jsout << i << '\t' << join_size[i] << std::endl;
    //jsout << "~~~~~~~~~~~~~~~~~~~~" << std::endl;
}
#endif

namespace contouring {

std::string too_many_levels::fmt_many_levels(level_t ix, level_t iy, level_t lbl, level_t lbr, level_t ltr, level_t ltl)
{
  std::ostringstream out;
  out << "too many levels @" << ix << ':' << iy << " bl=" << lbl
      << " br=" << lbr << " tr=" << ltr << " tl=" << ltl;
  return out.str();
}

namespace detail {

const int MAX_LEVELS = 1000;

class line_end;
typedef line_end* line_end_x;
typedef const line_end* line_end_xc;

class line {
public:
    line(const point_t& p, level_t level)
        : m_front(0), m_back(0), m_level(level), m_touched_border_count(0)
        { m_points.push_back(p);
            POCO_IFDEBUG(m_line_id = ++s_line_id); POCO_DEBUG(P("*line"); DUMP_L("",this)); }

    ~line()
        { POCO_DEBUG(P("~line"); DUMP_L("",this)); }

    void attach(line_end_x e)
        { if (not m_front) m_front = e; else if (not m_back) m_back = e; else SEGFAULT; }

    bool detach(line_end_x e)
        { this_end(e) = 0; return not (m_front or m_back); }

    bool is_front(line_end_xc e) const
        { return e == m_front; }

    bool is_back(line_end_xc e) const
        { return e == m_back; }

    line_end_x front() const
        { return m_front; }

    line_end_x back() const
        { return m_back; }

    line_end_x other_end(line_end_xc end) const
        { if (is_front(end)) return m_back; else if (is_back(end)) return m_front; SEGFAULT; return 0; }

    void replace_end(line_end_x end, line_end_x te)
        { this_end(end) = te; }

    void reverse()
        { m_points.reverse(); std::swap(m_front, m_back); }

    void add(const point_t& p, bool front)
        { m_points.push_front_or_back(front, p); }

    const point_t& get_point_at_end(line_end_x e)
        { return m_points.front_or_back(is_front(e)); }

    void drop_last_point(const line_end_x e)
        { m_points.pop_front_or_back(is_front(e)); }

    void add_point(const point_t& p)
        { m_points.push_back(p); }

    points_t& get_points()
        { return m_points; }

    level_t get_level() const
        { return m_level; }

    void join(line* other)
        { m_points.move_back(other->m_points);
            m_touched_border_count += other->m_touched_border_count; }

    void touch_border()
        { m_touched_border_count += 1; }

    bool is_border() const
        { return m_touched_border_count >= 2; }

private:
    line_end_x& this_end(line_end_x end)
        { if (is_front(end)) return m_front; else if (is_back(end)) return m_back; else SEGFAULT; return m_back; }

private:
    line_end_x m_front, m_back;
    points_t m_points;
    level_t m_level;
    size_t m_touched_border_count;
#ifdef CONTOURING_ENABLE_DEBUG
public:
    size_t m_line_id;
    static size_t s_line_id;
#endif
};

typedef line* line_x;

// ########################################################################

class line_end {
public:
    line_end()
        : m_line(0)
        { POCO_IFDEBUG(m_end_id = ++s_end_id); POCO_DEBUG(P("*line_end"); DUMP_LE("",this)); }

    line_end(line_x l)
        : m_line(l)
        { m_line->attach(this);
            POCO_IFDEBUG(m_end_id = ++s_end_id); POCO_DEBUG(P("*line_end"); DUMP_LE("",this)); }

    line_end(line_end_x le)
        : m_line(le->get_line())
        { m_line->attach(this);
            POCO_IFDEBUG(m_end_id = ++s_end_id); POCO_DEBUG(P("*line_end"); DUMP_LE("",this)); }

    ~line_end()
        { POCO_DEBUG(P("~line_end"); DUMP_LE("",this)); }

    void swap(line_end& o)
        { if (m_line) m_line->replace_end(this, &o); if (o.m_line) o.m_line->replace_end(&o, this); std::swap(m_line, o.m_line); }

    operator bool() const
        { return m_line != 0; }

    line_x stop_line()
        { if (m_line and m_line->detach(this)) return m_line; else return 0; }

    line_end_x other_end() const
        { return m_line->other_end(this); }

    bool is_front() const
        { return m_line->is_front(this); }

    bool is_back() const
        { return m_line->is_back(this); }

    void reverse()
        { m_line->reverse(); }

    void add(const point_t& p)
        { m_line->add(p, is_front()); }

    const point_t& get_point_at_end()
        { return m_line->get_point_at_end(this); }

    void drop_last_point()
        { m_line->drop_last_point(this); }

    line_x& get_line()
        { return m_line; }

    points_t& get_points()
        { return m_line->get_points(); }

    level_t get_level() const
        { return m_line->get_level(); }

    bool same_line(line_end& o)
        { return m_line == o.m_line; }

    void touch_border()
        { m_line->touch_border(); }

    bool is_border() const
        { return m_line->is_border(); }

private:
    line_x m_line;
#ifdef CONTOURING_ENABLE_DEBUG
public:
    size_t m_end_id;
    static size_t s_end_id;
#endif
};

// ########################################################################

class line_triplet;
typedef line_triplet* line_triplet_x;

// line_end objects for lines "under", on ("ctour") and "over" the contour line (polylines)
class line_triplet
{
public:
    line_triplet(line_end_x u, line_end_x c, line_end_x o)
        : m_over(o), m_ctour(c), m_under(u), m_peer(0)
        { POCO_IFDEBUG(m_triplet_id = ++s_triplet_id); POCO_DEBUG(P("*line_triplet"); DUMP_TR("",this)); }

    ~line_triplet()
        { if(m_peer) m_peer->m_peer = 0; POCO_DEBUG(P("~line_triplet " << m_triplet_id)); }

    void add(const point_t& point)
        { m_over->add(point); m_ctour->add(point); m_under->add(point); }

    level_t level_over() const
        { return m_over->get_level(); }

    level_t level_ctour() const
        { return m_ctour->get_level(); }

    level_t level_under() const
        { return m_under->get_level(); }

    line_end_x over() const
        { return m_over; }

    line_end_x ctour() const
        { return m_ctour; }

    line_end_x under() const
        { return m_under; }

    void set_over(line_end_x b)
        { m_over = b; }

    void set_ctour(line_end_x m)
        { m_ctour = m; }

    void set_under(line_end_x t)
        { m_under = t; }

    line_triplet_x peer() const
        { return m_peer; }

    void set_peer(line_triplet_x peer)
        { m_peer = peer; }

    void peer_up(line_triplet_x peer)
        { m_peer = peer; peer->m_peer = this; }

private:
    line_end_x m_over, m_ctour, m_under;
    line_triplet_x m_peer;
#ifdef CONTOURING_ENABLE_DEBUG
public:
    size_t m_triplet_id;
    static size_t s_triplet_id;
#endif
};

class point_generator {
public:
    point_generator(const field_t& field, size_t ix, size_t iy, bool hor_ver, level_t l0, level_t l1);

    point_t point(level_t l) const
        { return m_field.line_point(l, m_x0, m_y0, m_x1, m_y1); }

    point_t point() const
        { return point(level()); }
    level_t level() const
        { return m_current; }

    point_t rpoint() const
        { return point(rlevel()); }
    level_t rlevel() const
        { return m_end-m_delta; }

    void next()
        { m_current += m_delta; }

    void rnext()
        { m_end -= m_delta; }

    bool done()
        { return (m_current == m_end); }

    int increasing() const
        { return m_delta == 1 ? 1 : 0; }

private:
    const field_t& m_field;
    size_t m_x0, m_y0, m_x1, m_y1;
    level_t m_current, m_end;
    int m_delta;
};

typedef std::list<line_triplet_x> triplet_l;
typedef triplet_l::iterator triplet_li;
typedef std::pair<line_triplet_x, line_triplet_x> triplet_pair;
typedef std::pair<line_end_x,line_end_x> connect_up_t;

// ########################################################################

class runner {
public:
    runner(const field_t& field, lines_t& lout);
    void run();

private:
    void prepare_left_border();
    void prepare_column_bottom(size_t ix);
    void handle_inner_cell(size_t ix, size_t iy);
    void finish_column_top(size_t ix);
    void finish_right_border();

    void handle_undef_inner(size_t ix, size_t iy, bool undef_bl, bool undef_tl, bool undef_br, bool undef_tr,
                            level_t level_bl, level_t level_tl, level_t level_tr);
    void handle_def_inner(size_t ix, size_t iy, level_t level_bl, level_t level_tl, level_t level_tr);

    triplet_pair open_single(const point_t& p, level_t level_bot, level_t level_mid, level_t level_top);
    triplet_pair open_inside(const point_t& ptop, const point_t& pright, level_t level_bot, level_t level_mid, level_t level_top);

    line_end_x close_right(line_triplet_x bmt, line_end_x le_t);
    line_end_x close_top(line_end_x le_t, line_triplet_x bmt);
    void close_inside(line_triplet_x bleft, line_triplet_x bbottom);

    line_end_x le_connect_up(line_end_x le);
    void connect_up(line_triplet_x above);

    bool line_join(line_end_x a, line_end_x b, bool polygon);
    line_triplet_x open_left(const point_t& mid_point, line_end_x u, level_t level_c, level_t level_o);
    line_triplet_x open_bottom(const point_t& mid_point, level_t level_u, level_t level_c, line_end_x o);
    int count_corner(level_t level_corner, level_t level_1, level_t level_2);
    int count_cross(level_t level_1a, level_t level_1b, level_t level_2a, level_t level_2b);

    static triplet_li advanced(triplet_li it, int n)
        { std::advance(it, n); return it; }
    triplet_li erase(triplet_li a, triplet_li b);

    bool is_undefined(level_t l) const
        { return l == m_undefined_level; }

    line_triplet_x new_line_triplet(line_end_x u, line_end_x c, line_end_x o);

    void add_contour_line(level_t level, const points_t& points, bool closed);
    void add_contour_polygon(level_t level, const points_t& points);

#ifdef USE_ALLOCATOR
    line_x new_line(const point_t& p, level_t level)
        { return pool_line.construct(p, level); }

    line_end_x new_line_end(line_end_x le)
        { return pool_line_end.construct(le); }

    line_end_x new_line_end(const point_t& p, level_t level)
        { return pool_line_end.construct(new_line(p, level)); }

    void delete_line(line_x l)
        { pool_line.destroy(l); }

    void delete_line_end(line_end_x le);

    void delete_line_triplet(line_triplet_x t)
        { pool_line_triplet.destroy(t); }
#else // !USE_ALLOCATOR
    line_x new_line(const point_t& p, level_t level)
        { return new line(p, level); }

    line_end_x new_line_end(line_end_x le)
        { return new line_end(le); }

    line_end_x new_line_end(const point_t& p, level_t level)
        { return new line_end(new_line(p, level)); }

    void delete_line(line_x l)
        { delete l; }

    void delete_line_end(line_end_x le);

    void delete_line_triplet(line_triplet_x t)
        { delete t; }
#endif// !USE_ALLOCATOR

private:
    const field_t& m_field;
    lines_t& m_lines;
    size_t m_nx;
    size_t m_ny;

    line_end_x m_le_border_top;
    line_end_x m_le_border_low;

    level_t m_level_br;

    triplet_l m_triplets;
    triplet_li m_tix;

    connect_up_t m_connect_up;

    std::vector<level_t> m_levels;

    const level_t m_undefined_level;

#ifdef USE_ALLOCATOR
    boost::object_pool<line> pool_line;
    boost::object_pool<line_end> pool_line_end;
    boost::object_pool<line_triplet> pool_line_triplet;
#endif
};

line_triplet_x runner::new_line_triplet(line_end_x u, line_end_x c, line_end_x o)
{
    POCO_DEBUG_SCOPE();
    POCO_DEBUG(DUMP_LE("u", u); DUMP_LE("c", c); DUMP_LE("o", o));
#if 0 && defined(CONTOURING_ENABLE_DEBUG)
    if (c->get_level() != u->get_level() and c->get_level() != o->get_level())
        FAIL("BMT BAD LEVEL COMBI");
#endif

#ifdef USE_ALLOCATOR
    return pool_line_triplet.construct(u, c, o);
#else
    return new line_triplet(u, c, o);
#endif
}

// the contour algorithm produces bad polygons/polylines when handling isolated points surrounded by undefined values;
// it is not clear how contour lines should be defined in this case, so we just drop these polygons and polylines

void runner::add_contour_line(level_t level, const points_t& points, bool closed)
{
    if ((closed && points.size() >= 3) || (!closed && points.size() >= 2))
        m_lines.add_contour_line(level, points, closed);
}

void runner::add_contour_polygon(level_t level, const points_t& points)
{
    if (points.size() >= 3)
        m_lines.add_contour_polygon(level, points);
}

#ifdef CONTOURING_ENABLE_DEBUG
size_t line_end::s_end_id = 0;
size_t line::s_line_id = 0;
size_t line_triplet::s_triplet_id = 0;
#endif // CONTOURING_ENABLE_DEBUG

triplet_li runner::erase(triplet_li a, triplet_li b)
{
    return m_triplets.erase(a, b);
}

bool runner::line_join(line_end_x a, line_end_x b, bool polygon)
{
    POCO_DEBUG_SCOPE();
    POCO_DEBUG(DUMP_LE("a",a); P(" with "); DUMP_LE("b",b));
#ifdef LENGTH_STATS
    inc_join_size(a->get_points().size());
    inc_join_size(b->get_points().size());
#endif

    // join lines; return points if line is closed or touching border twice; return None if line not finished yet
    line* la = a->get_line();
    if (a->same_line(*b)) {
        POCO_DEBUG(P("same line"));
        assert(a->other_end() == b);
        assert(b->other_end() == a);
        if (polygon)
            add_contour_polygon(a->get_level(), la->get_points());
        else
            add_contour_line(a->get_level(), la->get_points(), true);
        delete_line_end(a);
        delete_line_end(b);
        return true;
    }

    // after this, there exists one line with two ends, line_end_a.other_end() and line_end_b.other_end()

#ifdef CONTOURING_ENABLE_DEBUG
    if (a->get_level() != b->get_level())
        FAIL("LEVEL MISMATCH " << LV(a->get_level()) << LV(b->get_level()));
#endif

    // connect points in correct order
    if (a->is_front())
        a->reverse();
    if (not b->is_front())
        b->reverse();

    la->join(b->get_line());

    const bool border = la->is_border();
    if (border) {
        // line touching border twice, ready to emit
        POCO_DEBUG(P("border"));
        assert(not polygon);
        assert(not a->other_end());
        assert(not b->other_end());
        add_contour_line(a->get_level(), la->get_points(), false);
    } else {
        // re-connect line ends
        line_end_x other_end_b = b->other_end();
        POCO_DEBUG(P("re-connect"); DUMP_LE("ob", other_end_b));
        if (other_end_b)
            a->swap(*other_end_b);
        POCO_DEBUG(P(" --> after reconnect "); DUMP_LE("a",a); DUMP_LE("b",b));
    }
    delete_line_end(a);
    delete_line_end(b);
    return border;
}

line_triplet_x runner::open_left(const point_t& p, line_end_x u, level_t lc, level_t lo)
{
    POCO_DEBUG_SCOPE();

    u->add(p);

    line_end_x c = new_line_end(p, lc);
    c->touch_border();

    line_end_x o = new_line_end(p, lo);

    return new_line_triplet(u, c, o);
}

line_triplet_x runner::open_bottom(const point_t& p, level_t lu, level_t lc, line_end_x o)
{
    POCO_DEBUG_SCOPE();

    line_end_x u = new_line_end(p, lu);

    line_end_x c = new_line_end(p, lc);
    c->touch_border();

    o->add(p);

    return new_line_triplet(u, c, o);
}

triplet_pair runner::open_single(const point_t& p, level_t lo, level_t lc, level_t li)
{
    POCO_DEBUG_SCOPE();
    POCO_DEBUG(P(LV(lo) << LV(lc) << LV(li)));
    line_end_x o1 = new_line_end(p, lo);
    line_end_x o2 = new_line_end(o1);

    line_end_x c1 = new_line_end(p, lc);
    line_end_x c2 = new_line_end(c1);

    line_end_x i1 = new_line_end(p, li);
    line_end_x i2 = new_line_end(i1);

    line_triplet_x btop   = new_line_triplet(o1, c1, i1);
    line_triplet_x bright = new_line_triplet(i2, c2, o2);
    btop->peer_up(bright);
    return triplet_pair(btop, bright);
}

triplet_pair runner::open_inside(const point_t& ptop, const point_t& pright, level_t lo, level_t lc, level_t li)
{
    POCO_DEBUG_SCOPE();
    triplet_pair btop_right = open_single(pright, lo, lc, li);
    btop_right.second->add(ptop);
    return btop_right;
}

line_end_x runner::close_right(line_triplet_x bmt, line_end_x le_t)
{
    POCO_DEBUG_SCOPE();
    // emits ctour of bmt and joins/closes over of bmt.under to le_t
    POCO_DEBUG(DUMP_TR("triplet=",bmt));
    POCO_DEBUG(DUMP_LE("line=", le_t));

    bmt->ctour()->touch_border();
    if (bmt->ctour()->is_border())
        add_contour_line(bmt->level_ctour(), bmt->ctour()->get_points(), false);
    delete_line_end(bmt->ctour());

    line_end_x le_i = bmt->under();
    line_end_x le_o = bmt->over();
    line_join(le_i, le_t, true);

    delete_line_triplet(bmt);

    return le_o;
}

line_end_x runner::close_top(line_end_x le_t, line_triplet_x bmt)
{
    POCO_DEBUG_SCOPE();
    POCO_DEBUG(DUMP_LE("line=", le_t));
    POCO_DEBUG(DUMP_TR("triplet=",bmt));

    bmt->ctour()->touch_border();
    if (bmt->ctour()->is_border())
        add_contour_line(bmt->level_ctour(), bmt->ctour()->get_points(), false);
    delete_line_end(bmt->ctour());

    line_end_x le_u = bmt->under();
    line_end_x le_o = bmt->over();
    line_join(le_o, le_t, true);

    delete_line_triplet(bmt);

    return le_u;
}

void runner::close_inside(line_triplet_x bleft, line_triplet_x bbottom)
{
    POCO_DEBUG_SCOPE();
    POCO_DEBUG(DUMP_TR("  bleft",bleft));
    POCO_DEBUG(DUMP_TR("bbottom",bbottom));

    const bool is_island = bbottom->under()->same_line(*bleft->over());
    line_join(bbottom->ctour(), bleft->ctour(), false);

    line_triplet_x p_left = bleft->peer(), p_bottom = bbottom->peer();

    if (p_bottom)
        p_bottom->set_peer(p_left);
    if (p_left)
        p_left->set_peer(p_bottom);

    if (is_island) {
        POCO_DEBUG(P(" .. connect_up "); DUMP_LE("cu",bleft->over()); DUMP_LE("cuo",bbottom->under()));
        m_connect_up = connect_up_t(bleft->over(), bbottom->under());
    } else {
        line_join(bleft->over(), bbottom->under(), true);
    }
    line_join(bleft->under(), bbottom->over(), true);

    delete_line_triplet(bbottom);
    delete_line_triplet(bleft);
}

line_end_x runner::le_connect_up(line_end_x le)
{
    POCO_DEBUG_SCOPE();
    line_end_x cu = m_connect_up.first, cuo = m_connect_up.second;
    line_end_x oe_cu = cu->other_end();
    const point_t cu_last = cu->get_point_at_end();
    const point_t le_last = le->get_point_at_end();
    const bool same = line_join(le, cu, true);
    POCO_DEBUG(P("dump le_connect_up"); DUMP_LE("le",le); P(','); DUMP_LE("cu",cu); P(','); DUMP_LE("cuo",cuo); P(','); DUMP_LE("oe_cu",oe_cu); P(LV(same)));
    if (!same) {
        oe_cu->add(cu_last);
        oe_cu->add(le_last);
    }
    return cuo;
}

void runner::connect_up(line_triplet_x above)
{
    if (m_connect_up.first) {
        // for the lower line, ie connect_up, always use the original
        // "over" line; this must be the "over" line for an island
        above->set_under(le_connect_up(above->under()));
    }
    m_connect_up = connect_up_t(0,0);
}

int runner::count_corner(level_t level_corner, level_t level_1, level_t level_2)
{
    const int diff_1 = level_corner - level_1;
    const int diff_2 = level_corner - level_2;
    if (diff_1 < 0 and diff_2 < 0)
        return -std::max(diff_1, diff_2); // -negative
    else if (diff_1 > 0 and diff_2 > 0)
        return +std::min(diff_1, diff_2); // +positive
    else
        return 0;
}

int runner::count_cross(level_t level_1a, level_t level_1b, level_t level_2a, level_t level_2b)
{
    level_t max_a = std::max(level_1a, level_2a);
    level_t min_b = std::min(level_1b, level_2b);
    if (max_a < min_b)
        return min_b - max_a;

    level_t  min_a = std::min(level_1a, level_2a);
    level_t  max_b = std::max(level_1b, level_2b);
    if (min_a > max_b)
        return min_a - max_b;

    return 0;
}

point_generator::point_generator(const field_t& field, size_t ix, size_t iy, bool hor_ver, level_t l0, level_t l1)
    : m_field(field), m_x0(ix), m_y0(iy), m_x1(hor_ver ? ix+1 : ix), m_y1(hor_ver ? iy : iy+1)
{
    if (l0 < l1) {
        m_current = l0;
        m_end = l1;
        m_delta = +1;
    } else {
        m_current = l0-1;
        m_end = l1-1;
        m_delta = -1;
    }
}

void runner::handle_undef_inner(size_t ix, size_t iy, bool undef_bl, bool undef_tl, bool undef_br, bool undef_tr,
                                level_t level_bl, level_t level_tl, level_t level_tr)
{
    POCO_DEBUG_SCOPE();
    // tail and head (t-h) are always together, located at lix+n_bottom (n_bottom is 0 if no defined bottom)
    // each continuous sequence of undefined values along bmts has its own t-h pair (see split_undef)
    // new_right insertet before t-h, adding to tail
    // new_top after t-h, adding to head
    // close bottom/left with head
    // if defined top, set lix after t-h, otherwise before t-h (so that next cell finds it)

    // new t-h pair if tr undefined, other corners defined
    // close t-h if bl undefined, others defined
    // join t-h-t-h doublet iff bl defined, br and tl undefined

    const bool defined_top   = not (undef_tr or undef_tl);
    const bool defined_bot   = not (undef_br or undef_bl);
    const bool defined_left  = not (undef_bl or undef_tl);
    const bool defined_right = not (undef_br or undef_tr);

    const bool new_undefined = undef_tr and not (undef_bl or undef_br or undef_tl);
    const bool closed_undef  = undef_bl and not (undef_br or undef_tl or undef_tr);
    const bool join_undef    = (not undef_bl) and undef_br and undef_tl;  // may occur with split_undef
    const bool split_undef    = undef_br and undef_tl and (not undef_tr); // may occur with join_undef

    const size_t n_bottom = defined_bot  ? abs(m_level_br - level_bl) : 0;
    const size_t n_left   = defined_left ? abs(  level_bl - level_tl) : 0;

    triplet_pair undef; // tail = first, head = second
    triplet_li it_after_bottom = advanced(m_tix, n_bottom), it_after_undef = it_after_bottom;
    POCO_DEBUG(P(LV(&(*m_tix)) << LV(&(*it_after_bottom)) << LV(&(*it_after_undef))));
    if (new_undefined) {
        POCO_DEBUG(P("new_undefined"));
        // new undefined area, create undef bmt (will be along br-bl-tl)
        const point_t p_corner_bl = m_field.grid_point(ix, iy); // bottom-left corner of undefined cell
        undef = open_single(p_corner_bl, level_bl, level_tr, level_tr);

        it_after_bottom = m_triplets.insert(it_after_undef, undef.first);
        if (n_bottom == 0)
            m_tix = it_after_bottom;
        POCO_DEBUG(P(LV(&(*m_tix)) << LV(&(*it_after_bottom)) << LV(&(*it_after_undef))));
        m_triplets.insert(it_after_undef, undef.second);
    } else {
        POCO_DEBUG(P("old undefined"));
        // already undefined, fetch bmts
        undef.first  = *it_after_undef++;
        undef.second = *it_after_undef++;
    }
    POCO_DEBUG(DUMP_LE("bt",m_le_border_top));
    // now, it_after_bottom points after bottom plus twi undef

    if (defined_bot) {
        POCO_DEBUG(P("defined_bot"));
        connect_up(undef.first);
        if (n_bottom > 0) {
            triplet_li lix0 = it_after_bottom;
            for (size_t i=0; i<n_bottom; ++i)
                undef.first->set_under(close_top(undef.first->under(), *(--lix0)));
            m_tix = erase(m_tix, it_after_bottom);
        }
        undef.first->add(m_field.grid_point(ix+1, iy)); // bottom-right corner of undefined cell
    }

    if (defined_left) {
        POCO_DEBUG(P("defined_left"));
        if (n_left > 0) {
            // close all incoming on left, bottom-top
            triplet_li it_top = it_after_undef;
            POCO_DEBUG(P(LV(&(*m_tix)) << LV(&(*it_after_bottom)) << LV(&(*it_after_undef)) << LV(&(*it_top))));
            for (size_t i=0; i<n_left; ++i, ++it_top) {
                POCO_DEBUG(P(LV(i)); DUMP_LE("bt before",m_le_border_top));
                undef.second->set_over(close_right(*it_top, undef.second->over()));
                POCO_DEBUG(P(LV(i)); DUMP_LE("bt after",m_le_border_top));
            }
            it_after_undef = erase(it_after_undef, it_top);
            POCO_DEBUG(DUMP_LE("bt",m_le_border_top));
        }
        undef.second->add(m_field.grid_point(ix, iy+1));
    }
    POCO_DEBUG(DUMP_LE("bt",m_le_border_top));

    if (defined_right) {
        POCO_DEBUG(P("defined_right"));
        // open outgoing on right
        point_generator right(m_field, ix+1, iy, false/*vertical*/, m_level_br, level_tr);
        for (; not right.done(); right.next()) {
            const level_t lt = right.level() + right.increasing();
            line_triplet_x nbmt = open_left(right.point(), undef.first->under(), right.level(), lt);
            m_triplets.insert(m_tix, nbmt);
            undef.first->set_under(new_line_end(nbmt->over()));
        }
        undef.first->add(m_field.grid_point(ix+1, iy+1)); // top-right corner of undefined cell
    }
    // now, m_tix is "advanced" by new_right_pl.size()
        POCO_DEBUG(DUMP_LE("bt",m_le_border_top));

    if (defined_top) {
        POCO_DEBUG(P("defined_top"));
        // open outgoing on top
        point_generator top(m_field, ix, iy+1, true/*horizontal*/, level_tl, level_tr);
        for (; not top.done(); top.next()) {
            const level_t lt = top.level() + ((level_tr < level_tl) ? 0 : 1);
            line_triplet_x nbmt = open_bottom(top.point(), lt, top.level(), undef.second->over());
            undef.second->set_over(new_line_end(nbmt->under()));
            it_after_undef = m_triplets.insert(it_after_undef, nbmt);
        }
        if (not defined_right)
            undef.second->add(m_field.grid_point(ix+1, iy+1)); // top-right corner of undefined cell
    }

    if (closed_undef) {
        assert(!join_undef && !split_undef);
        POCO_DEBUG(P("closed_undef"));
        // close undef bmt, possibly set connect_up
        close_inside(undef.second, undef.first);
        m_tix = erase(m_tix, it_after_undef);
    }
    if (join_undef) {
        POCO_DEBUG(P("join_undef"));
        // join with next undefined area
        close_inside(*it_after_undef, undef.second);
        triplet_li it1 = advanced(m_tix, 1);
        triplet_li it3 = advanced(it1, 2);
        erase(it1, it3);
    }
    if (split_undef) {
        POCO_DEBUG(P("split_undef"));
        // create new undef head/tail undefined area
        const point_t p_corner_tl = m_field.grid_point(ix+1, iy+1); // top-left corner of undefined cell;
        triplet_pair undef;
        //if (undef_bl)
        //    undef = open_single(p_corner_tl, level_tl, level_tl, level_tr);
        //else
        //    undef = open_single(p_corner_tl, level_tr, level_tl, level_tl);
        level_t lo = level_tr, lc = level_tl, li = level_tl;
#if 0
        line_triplet* uout = undef.first;
        if (not uout)
            uout = undef.second;
        if (uout) {
            lo = uout->level_over();
            lc = uout->level_ctour();
            li = uout->level_under();
        }
#else
        //if (not (undef.first or undef.second))
        //    std::swap(lo, li);
#endif
        undef = open_single(p_corner_tl, li, lc, lo);
        triplet_li it1 = advanced(m_tix, 1);
        m_triplets.insert(it1, undef.first);
        m_triplets.insert(it1, undef.second);
    }

    if ((defined_top or split_undef) and not closed_undef)
        std::advance(m_tix, 2); // hop over tail+head

    m_levels[iy] = m_level_br;
    m_level_br = level_tr; // next cell has this cells top-right as bottom-right
}

namespace {
inline size_t adiff(int a, int b)
{
  return a < b ? b - a : a - b;
}
inline bool chkmax(int a, int b)
{
    size_t n = adiff(a, b);
    return (n > MAX_LEVELS);
}
} // namespace

void runner::handle_def_inner(size_t ix, size_t iy, level_t level_bl, level_t level_tl, level_t level_tr)
{
    const size_t n_right  = adiff(m_level_br, level_tr);
    const size_t n_top    = adiff(  level_tr, level_tl);
    const size_t n_left   = adiff(  level_tl, level_bl);
    const size_t n_bottom = adiff(m_level_br, level_bl);

    point_generator right(m_field, ix+1, iy, false/*vertical*/, m_level_br, level_tr);
    point_generator top  (m_field, ix, iy+1, true/*horizontal*/, level_tr, level_tl);

    const size_t count_lr = count_cross(level_tl,   level_bl, level_tr, m_level_br);
    const size_t count_bt = count_cross(level_bl, m_level_br, level_tl,   level_tr);
    const size_t count_br = count_corner(m_level_br, level_tr, level_bl);
    const size_t count_tr = n_right - count_br - count_lr;
    const size_t count_bl = n_bottom - count_br - count_bt;
    const size_t count_tl = n_top - count_tr - count_bt;

    assert(count_tl == n_left - count_bl - count_lr);
    assert(n_left == count_bl + count_lr + count_tl);
    assert(not (count_bt != 0 and count_lr != 0));

    // bottom-right corner: extend line from bottom to right
    for (size_t i=0; i<count_br; ++i, ++m_tix, right.next())
        (*m_tix)->add(right.point());

    // bottom-left corner: close
    if (count_bl) {
        triplet_li it_after_bl = advanced(m_tix, count_bt+count_bl), it_before_bl = it_after_bl;
        for (size_t i=0; i<count_bl; ++i) {
            --it_before_bl;
            connect_up(*it_after_bl);
            POCO_DEBUG(P("handle_def_inner close inside"));
            close_inside(*it_after_bl, *it_before_bl); // left, bottom
            //POCO_DEBUG(P("handle_def_inner close inside => cu="); DUMP_LE("cu",m_connect_up.first); P(',');
            //           DUMP_LE("cuo",m_connect_up.second));
            ++it_after_bl;
        }
        if (it_before_bl == m_tix)
            m_tix = it_after_bl;
        erase(it_before_bl, it_after_bl);
    }

    // left-right connections
    if (count_lr) {
        connect_up(*m_tix);
        for (size_t i=0; i<count_lr; ++i, ++m_tix, right.next())
            (*m_tix)->add(right.point());
    }

    // top-right corner: open new lines
    triplet_li it_after_tr = m_tix;
    if (count_tr) {
        triplet_li it_insert_tail, it_insert_head = m_tix;
        for (size_t i=0; i<count_tr; ++i, right.rnext(), top.next()) {
            const level_t level_c = top.level();
            const level_t level_in  = level_c + right.increasing();
            const level_t level_out = level_c + 1 - right.increasing();
            triplet_pair tr = open_inside(top.point(), right.rpoint(), level_out, level_c, level_in);
            triplet_li h0 = m_triplets.insert(it_insert_head, tr.second);
            if (i == 0) {
                m_tix = h0;
                it_insert_tail = m_tix;
            }
            it_insert_tail = m_triplets.insert(it_insert_tail, tr.first);
        }
    }

    // bottom-top connections
    for (size_t i=0; i<count_bt; ++i, ++it_after_tr, top.next())
        (*it_after_tr)->add(top.point());

    // top-left corner: extend line from left to top
    if (count_tl) {
        connect_up(*it_after_tr);
        for (size_t i=0; i<count_tl; ++i, ++it_after_tr, top.next())
            (*it_after_tr)->add(top.point());
    }

    m_levels[iy] = m_level_br;
    m_level_br = level_tr; // next cell has this cells top-right as bottom-right
}

void runner::prepare_left_border()
{
    POCO_DEBUG_SCOPE();
    // bmts will contain current line head triplets

    // find level indices for all grid points on the left border
    m_levels.reserve(m_ny);
    for (size_t iy = 0; iy < m_ny; ++iy)
      m_levels.push_back(m_field.grid_level(0, iy));

    level_t level_bl = m_levels[0];
    POCO_DEBUG(P(LV(level_bl)));
    bool undef_bl = is_undefined(level_bl);
    point_t point_bl = m_field.grid_point(0, 0);
    m_le_border_low = new_line_end(point_bl, level_bl);
    m_le_border_top = new_line_end(m_le_border_low);
    if (undef_bl) {
        m_triplets.push_back(0); // tail
        m_triplets.push_back(0); // head
    }

    // make heads for all lines coming in on the left from outside
    for (size_t iy = 0; iy < m_ny-1; ++iy) {
        const level_t level_tl = m_levels[iy+1];
        const bool    undef_tl = is_undefined(level_tl);
        const point_t point_tl = m_field.grid_point(0, iy+1);

        if (not (undef_bl or undef_tl)) {
            POCO_DEBUG(P("all defined"));
            if (chkmax(level_bl, level_tl))
                throw too_many_levels(-1, iy, level_bl, 0, 0, level_tl);

            point_generator border(m_field, 0, iy, false/*vertical*/, level_bl, level_tl);
            for (; not border.done(); border.next()) {
                const level_t level_c = border.level();
                const level_t level_o = (level_tl > level_bl) ? (level_c+1) : level_c;
                m_triplets.push_back(open_left(border.point(), m_le_border_top, level_c, level_o));
                m_le_border_top = new_line_end(m_triplets.back()->over());
            }
            m_le_border_top->add(point_tl);
        } else if (undef_bl and undef_tl) {
            POCO_DEBUG(P("stay undefined"));
            // left border stays undef
            m_le_border_top->add(point_tl);
        } else if ((not undef_bl) and undef_tl) {
            POCO_DEBUG(P("new undefined"));
            // left border new undef
            const point_t p = m_le_border_top->get_point_at_end();
            m_le_border_top->drop_last_point(); // FIXME duplicate point in m_le_border_top
            line_triplet_x undef_tail = open_left(p, m_le_border_top, level_tl, level_tl);
            m_le_border_top = new_line_end(undef_tail->over());
            m_le_border_top->add(point_tl);
            m_triplets.push_back(undef_tail);
            m_triplets.push_back(0); // head
        } else if (undef_bl and (not undef_tl)) {
            POCO_DEBUG(P("end undefined"));
            // left border: end undef
            line_triplet_x undef_head = open_left(point_tl, m_le_border_top, level_bl, level_tl);
            m_triplets.back() = undef_head;
            m_le_border_top = new_line_end(undef_head->over());
        }

        level_bl = level_tl;
        undef_bl = undef_tl;
    }
}

void runner::prepare_column_bottom(size_t ix)
{
    POCO_DEBUG_SCOPE();
    m_level_br = m_field.grid_level(ix+1, 0); // bottom right of cell at lower border
    level_t level_bl = m_levels[0];
    POCO_DEBUG(P(LV(ix) << LV(level_bl) << LV(m_level_br)));
    bool undef_bl = is_undefined(level_bl);
    bool undef_br = is_undefined(m_level_br);
    const point_t point_br = m_field.grid_point(ix+1, 0); // lower right corner of bottom cell

    if (not (undef_bl or undef_br)) {
        if (chkmax(level_bl, m_level_br))
            throw too_many_levels(ix, -1, level_bl, m_level_br, 0, 0);

        // new points at bottom of grid, from left to right -- need to insert in bmts such that it ends right-to-left
        point_generator bottom(m_field, ix, 0, true/*horizontal*/, level_bl, m_level_br);
        for (; not bottom.done(); bottom.next()) {
            level_t level_c = bottom.level();
            level_t level_u = level_c + bottom.increasing();
            line_triplet_x t = open_bottom(bottom.point(), level_u, level_c, m_le_border_low); // FIXME this actually closes the polygon le_border_low
            m_le_border_low = new_line_end(t->under());
            m_triplets.push_front(t);
        }
        m_le_border_low->add(point_br);
    } else if (undef_bl and undef_br) {
        // bot border: stays undef
        m_le_border_low->add(point_br);
    } else if ((not undef_bl) and undef_br) {
        // bot border: new undef
        const point_t p = m_le_border_low->get_point_at_end();
        m_le_border_low->drop_last_point(); // FIXME duplicate point
        line_triplet_x undef_head = open_bottom(p, m_level_br, m_level_br, m_le_border_low); // FIXME can this close le_border_low?
        m_le_border_low = new_line_end(undef_head->under());
        m_le_border_low->add(point_br);
        m_triplets.push_front(undef_head);
        m_triplets.push_front(0); // tail
    } else if (undef_bl and (not undef_br)) {
        // bot border: end undef
        line_triplet_x undef_tail = open_bottom(point_br, m_level_br, level_bl, m_le_border_low);
        m_le_border_low = new_line_end(undef_tail->under());
        m_triplets.front() = undef_tail;
    }

    m_tix = m_triplets.begin();
    m_connect_up = connect_up_t(0,0);
}

void runner::handle_inner_cell(size_t ix, size_t iy)
{
    POCO_DEBUG_SCOPE();
    const level_t level_bl = m_levels[iy];
    const level_t level_tl = m_levels[iy+1];
    const level_t level_tr = m_field.grid_level(ix+1, iy+1); // top right of cell

    const bool undef_bl = is_undefined(level_bl);
    const bool undef_tl = is_undefined(level_tl);
    const bool undef_br = is_undefined(m_level_br);
    const bool undef_tr = is_undefined(level_tr);
    const bool undef_any = undef_bl or undef_tl or undef_br or undef_tr;

    POCO_DEBUG(P(LV(ix) << LV(iy) << LV(m_level_br) << LV(level_bl) << LV(level_tl) << LV(level_tr)));

    if (undef_any)
        handle_undef_inner(ix, iy, undef_bl, undef_tl, undef_br, undef_tr,
                           level_bl, level_tl, level_tr);
    else {
      if (chkmax(level_bl, m_level_br)
          or chkmax(m_level_br, level_tr)
          or chkmax(level_tr, level_tl)
          or chkmax(level_tl, level_bl))
        throw too_many_levels(ix, iy, level_bl, m_level_br, level_tr, level_tl);
      handle_def_inner(ix, iy, level_bl, level_tl, level_tr);
    }
    POCO_DEBUG(DUMP_LE("bt",m_le_border_top));
}

void runner::finish_column_top(size_t ix)
{
    POCO_DEBUG_SCOPE();
    const level_t level_bl = m_levels.back();
    const bool undef_bl = is_undefined(level_bl);
    const bool undef_br = is_undefined(m_level_br);
    POCO_DEBUG(P(LV(ix) << LV(level_bl) << LV(m_level_br)); DUMP_LE("bt",m_le_border_top));

    const point_t point_br = m_field.grid_point(ix + 1, m_ny - 1); // top-right corner of top cell

    if (m_connect_up.first) {
        POCO_DEBUG(P("finish_column_top with connect_up"));
        if (not undef_bl) {
            POCO_DEBUG(P("connect up"); DUMP_LE("bt",m_le_border_top));
            m_le_border_top = le_connect_up(m_le_border_top);
            POCO_DEBUG(DUMP_LE("bt",m_le_border_top));
            m_connect_up = connect_up_t(0,0);
        } else {
            POCO_DEBUG(P("connect_up undefined"));
            line_triplet_x undef_head = *advanced(m_tix, 1);
            if (undef_head) {
                connect_up(undef_head);
            } else {
                m_le_border_top = le_connect_up(m_le_border_top);
                m_connect_up = connect_up_t(0,0);
            }
        }
    }

    if (not (undef_bl or undef_br)) {
        POCO_DEBUG(P("bl+br defined"));
        const size_t n_exit_top = abs(m_level_br - level_bl); // 'bottom' side of cell above top
        POCO_DEBUG(P(LV(n_exit_top) << LV(m_triplets.size())));
        assert(m_triplets.end() == advanced(m_tix, n_exit_top));
        if (n_exit_top) {
            triplet_li it_exit = m_triplets.end();
            for (size_t i=0; i<n_exit_top; ++i) {
                POCO_DEBUG(P(LV(i)); DUMP_LE("bt",m_le_border_top));
                m_le_border_top = close_top(m_le_border_top, *(--it_exit));
            }
            POCO_DEBUG(P("finished"); DUMP_LE("bt",m_le_border_top));
            erase(m_tix, m_triplets.end());
        }
        POCO_DEBUG(DUMP_LE("bt",m_le_border_top));
        m_le_border_top->add(point_br);
    } else if (undef_bl and undef_br) {
        POCO_DEBUG(P("bl+br undefined"));
        // top border: stays undef
        m_le_border_top->add(point_br);
    } else if ((not undef_bl) and undef_br) {
        POCO_DEBUG(P("bl=defined => br=undefined"));
        // top border: new undef from below
        line_triplet_x& undef_head = *advanced(m_tix, 1);
        // join undef_head.over with m_le_border_top => undef_head.over
        // add point_br to undef.second.top
        undef_head->under()->add(point_br);
        m_le_border_top = close_top(m_le_border_top, undef_head);
        undef_head = 0;
    } else if (undef_bl and (not undef_br)) {
        POCO_DEBUG(P("bl=undefined => br=defined"));
        // top border: end undef from below
        triplet_li it_drop = m_tix;
        line_triplet_x undef_tail = *m_tix++;
        line_triplet_x undef_head = *m_tix++;
        POCO_DEBUG(P("end undef from below t=" << undef_tail << " h=" << undef_head));
        assert(m_tix == m_triplets.end());
        if (not undef_head)
            m_le_border_top = close_top(m_le_border_top, undef_tail);
        else if (undef_head->under())
            m_le_border_top = close_top(undef_head->under(), undef_tail);
        else
            m_le_border_top = 0;
        erase(it_drop, m_tix);
    }
    m_levels.back() = m_level_br;
    POCO_DEBUG(P("END finish_column_top"));
}

void runner::finish_right_border()
{
    POCO_DEBUG_SCOPE();
    m_tix = m_triplets.begin();
    for (size_t iy=0; iy<m_ny-1; ++iy) {
        level_t level_bl = m_levels[iy];
        level_t level_tl = m_levels[iy+1];
        bool undef_bl = is_undefined(level_bl);
        bool undef_tl = is_undefined(level_tl);

        POCO_DEBUG(P("iy=" << iy << " m_tix=" << std::distance(m_triplets.begin(), m_tix) << " #triplets==" << m_triplets.size()));

        const point_t point_tl = m_field.grid_point(m_nx-1, iy);

        if (not (undef_bl or undef_tl)) {
            const size_t n_left = abs(level_bl - level_tl);
            for (size_t i=0; i<n_left; ++i, ++m_tix) {
                POCO_DEBUG(P("iy=" << iy << " i=" << i << " n_left=" << n_left));
                m_le_border_low = close_right(*m_tix, m_le_border_low);
            }
        } else if (undef_bl and undef_tl) {
            // right border: stays undef
        } else if ((not undef_bl) and undef_tl) {
            // right border: new undef from left
            line_triplet_x undef_tail = *m_tix;
            m_le_border_low = close_right(undef_tail, m_le_border_low);
        } else if (undef_bl and (not undef_tl)) {
            // right border: end undef from left
            triplet_li it_drop = m_tix++;
            line_triplet_x undef_head = *m_tix++;
            m_le_border_low = close_right(undef_head, m_le_border_low);
            erase(it_drop, m_tix);
        }
        m_le_border_low->add(point_tl);
    }

    line_join(m_le_border_low, m_le_border_top, true);
}

void runner::run()
{
    POCO_DEBUG_SCOPE();
    prepare_left_border();

    for (size_t ix = 0; ix < m_nx - 1; ++ix) {
      prepare_column_bottom(ix);

      for (size_t iy = 0; iy < m_ny - 1; ++iy)
        handle_inner_cell(ix, iy);

      finish_column_top(ix);
    }

    finish_right_border();
}

void runner::delete_line_end(line_end_x le)
{
#ifdef USE_ALLOCATOR
    line_x l = le->stop_line();
    pool_line_end.destroy(le);
    if (l)
        pool_line.destroy(l);
#else // !USE_ALLOCATOR
    line_x l = le->stop_line();
    delete le;
    delete l;
#endif // !USE_ALLOCATOR
}

runner::runner(const field_t& field, lines_t& lout)
    : m_field(field)
    , m_lines(lout)
    , m_nx(field.nx())
    , m_ny(field.ny())
    , m_le_border_top(0)
    , m_le_border_low(0)
    , m_tix(m_triplets.begin())
    , m_connect_up(0, 0)
    , m_undefined_level(m_field.undefined_level())
{
}

} // namespace contouring::detail

void run(const field_t& field, lines_t& lines)
{
#ifdef LENGTH_STATS
    clear_join_size();
#endif
    if (field.nx() == 0 || field.ny() == 0)
      return;
    detail::runner r(field, lines);
    r.run();
#ifdef LENGTH_STATS
    dump_join_size();
#endif
}

} // namespace contouring
