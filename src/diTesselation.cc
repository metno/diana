/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diTesselation.h>

#include <GL/glu.h>

#include <list>

#define MILOGGER_CATEGORY "diana.tesselation"
#include <miLogger/miLogging.h>

namespace /*anonymous*/ {

struct VERTEX {
  GLdouble x;
  GLdouble y;
  GLdouble z;
  VERTEX(GLdouble xyz[3])
    : x(xyz[0]), y(xyz[1]), z(xyz[2]) { }
};

class GluTesselator {
public:
  GluTesselator();
  ~GluTesselator();
  void run(GLdouble *gldata, int ncontours, const int *count);

private:
  VERTEX* create_vertex(GLdouble coords[3]);

private:
  static void cb_error(GLenum err);
  static void cb_combine(GLdouble coords[3], VERTEX *v[4],
      GLfloat w[4], VERTEX **vertexp, void *polygon_data);

private:
  GLUtesselator *tess;
  std::list<VERTEX> vertices;
};

GluTesselator::GluTesselator()
  : tess(gluNewTess())
{
  gluTessCallback(tess, GLU_TESS_BEGIN,        (_GLUfuncptr) glBegin);
  gluTessCallback(tess, GLU_TESS_EDGE_FLAG,    (_GLUfuncptr) glEdgeFlag);
  gluTessCallback(tess, GLU_TESS_VERTEX,       (_GLUfuncptr) glVertex3dv);
  gluTessCallback(tess, GLU_TESS_END,          (_GLUfuncptr) glEnd);

  gluTessCallback(tess, GLU_TESS_ERROR,        (_GLUfuncptr) &GluTesselator::cb_error);
  gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (_GLUfuncptr) &GluTesselator::cb_combine);

  gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
  gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);
  gluTessProperty(tess, GLU_TESS_TOLERANCE, 0.0);
  gluTessNormal(tess, 0.0, 0.0, 1.0);
}

GluTesselator::~GluTesselator()
{
  gluDeleteTess(tess);
}

VERTEX* GluTesselator::create_vertex(GLdouble coords[3])
{
  vertices.push_front(VERTEX(coords));
  return &(*vertices.begin());
}

void GluTesselator::cb_error(GLenum err)
{
  const GLubyte* errmsg = gluErrorString(err);
  if (errmsg)
    METLIBS_LOG_ERROR("(" << err << ") " << errmsg);
  else
    METLIBS_LOG_ERROR("(" << err << ") <<unknown>>");
}

void GluTesselator::cb_combine(GLdouble coords[3], VERTEX *v[4],
    GLfloat w[4], VERTEX **vertexp, void *polygon_data)
{
   if (!vertexp || !polygon_data)
     return;

   GluTesselator* self = (GluTesselator*) polygon_data;
   *vertexp = self->create_vertex(coords);
}

void GluTesselator::run(GLdouble *gldata, int ncontours, const int *count)
{
  gluTessBeginPolygon(tess, this);
  int j= 0;
  for (int n=0; n<ncontours; n++) {
    gluTessBeginContour(tess);
    for (int i=0; i<count[n]; i++, j+=3)
      gluTessVertex(tess, &gldata[j], &gldata[j]);
    gluTessEndContour(tess);
  }
  gluTessEndPolygon(tess);
  vertices.clear();
}

} // anonymous namespace

void tesselation(GLdouble *gldata, int ncontours, const int *count)
{
  GluTesselator tess;
  tess.run(gldata, ncontours, count);
}
