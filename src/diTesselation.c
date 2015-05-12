/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

#include <qglobal.h>

#include <diTesselation.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef GLCALLBACK
#ifdef GLAPIENTRY
#define GLCALLBACK GLAPIENTRY
#else
#define GLCALLBACK APIENTRY
#endif
#endif

/************************
define DEBUGEACHCALLBACK
define DEBUGCALLBACK
 ************************/

static GLUtesselator *tess;

#ifdef DEBUGCALLBACK
static int num_start_tesselation=0;
static int num_combine_callback;
#endif

#ifdef DEBUGEACHCALLBACK
static int n_combine_callback;
#endif

typedef struct {
	GLdouble x;
	GLdouble y;
	GLdouble z;
	GLdouble r;
	GLdouble g;
	GLdouble b;
	GLdouble a;
} VERTEX;



static void GLCALLBACK error_callback( GLenum err )
{
  const GLubyte* errmsg;
  errmsg = gluErrorString( err );
  fprintf(stderr, "tesselation error_callback %d : %s\n",err,errmsg );
}

static void GLCALLBACK combineCallback(GLdouble coords[3],
                     VERTEX *vertex_data[4],
                     GLfloat weight[4], VERTEX **dataOut )
{
   VERTEX *vertex;

   vertex = (VERTEX *) malloc(sizeof(VERTEX));

#ifdef DEBUGEACHCALLBACK
   if (vertex_data[0])
	fprintf(stderr, "vertex_data[0] %f\n",vertex_data[0]->x );
   if (vertex_data[1])
	fprintf(stderr, "vertex_data[1] %f\n",vertex_data[1]->x );
   if (vertex_data[2])
	fprintf(stderr, "vertex_data[2] %f\n",vertex_data[2]->x );
   if (vertex_data[3])
	fprintf(stderr, "vertex_data[3] %f\n",vertex_data[3]->x );

   fprintf(stderr, "vertex_data[x] done \n");

   fprintf(stderr, "coords[0] %f\n",coords[0] );
   fprintf(stderr, "coords[1] %f\n",coords[1] );
   fprintf(stderr, "coords[2] %f\n",coords[2] );
#endif

   vertex->x = coords[0];
   vertex->y = coords[1];
   vertex->z = coords[2];

#ifdef DEBUGEACHCALLBACK
   fprintf(stderr, "coords to vertex done \n");
#endif

   if (vertex_data[0] != 0 && vertex_data[1] != 0 && vertex_data[2] != 0 && vertex_data[3] != 0)
   {

	   vertex->r = weight[0]*vertex_data[0]->r + weight[1]*vertex_data[1]->r + weight[2]*vertex_data[2]->r + 
		   weight[3]*vertex_data[3]->r; 
	   vertex->g = weight[0]*vertex_data[0]->g + weight[1]*vertex_data[1]->g + weight[2]*vertex_data[2]->g + 
		   weight[3]*vertex_data[3]->g; 
	   vertex->b = weight[0]*vertex_data[0]->b + weight[1]*vertex_data[1]->b + weight[2]*vertex_data[2]->b + 
		   weight[3]*vertex_data[3]->b; 
	   vertex->a = weight[0]*vertex_data[0]->a + weight[1]*vertex_data[1]->a + weight[2]*vertex_data[2]->a + 
		   weight[3]*vertex_data[3]->a;
   }
   else if (vertex_data[0] != 0 && vertex_data[1] != 0 && vertex_data[2] != 0)
   {
	   vertex->r = weight[0]*vertex_data[0]->r + weight[1]*vertex_data[1]->r + weight[2]*vertex_data[2]->r ; 
	   vertex->g = weight[0]*vertex_data[0]->g + weight[1]*vertex_data[1]->g + weight[2]*vertex_data[2]->g ; 
	   vertex->b = weight[0]*vertex_data[0]->b + weight[1]*vertex_data[1]->b + weight[2]*vertex_data[2]->b ; 
	   vertex->a = weight[0]*vertex_data[0]->a + weight[1]*vertex_data[1]->a + weight[2]*vertex_data[2]->a ;
   }
   else if (vertex_data[0] != 0 && vertex_data[1] != 0)
   {
	   vertex->r = weight[0]*vertex_data[0]->r + weight[1]*vertex_data[1]->r ; 
	   vertex->g = weight[0]*vertex_data[0]->g + weight[1]*vertex_data[1]->g ; 
	   vertex->b = weight[0]*vertex_data[0]->b + weight[1]*vertex_data[1]->b ; 
	   vertex->a = weight[0]*vertex_data[0]->a + weight[1]*vertex_data[1]->a ;
   }
   
   if (dataOut)
	*dataOut = vertex;

#ifdef DEBUGEACHCALLBACK
   fprintf(stderr, "combineCallback done \n");
#endif
}



void beginTesselation()
{
  tess= gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN, glBegin );
  gluTessCallback(tess, GLU_TESS_EDGE_FLAG, (GLvoid*)glEdgeFlag );
  gluTessCallback(tess, GLU_TESS_VERTEX, glVertex2dv );
  gluTessCallback(tess, GLU_TESS_END, glEnd );

  gluTessCallback(tess, GLU_TESS_ERROR, error_callback );
  gluTessCallback(tess, GLU_TESS_COMBINE, combineCallback );

  gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
  gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);
  gluTessProperty(tess, GLU_TESS_TOLERANCE, 0.0);

  gluTessNormal(tess, 0.0, 0.0, 1.0 );

#ifdef DEBUGCALLBACK
  num_start_tesselation++;
  num_combine_callback=0;
#endif

#ifdef DEBUGEACHCALLBACK
  n_combine_callback=0;
#endif
}


void endTesselation()
{
  gluDeleteTess(tess);
  tess= NULL;

#ifdef DEBUGCALLBACK
  if (num_combine_callback>0)
    fprintf(stderr, "tesselation start, combine_callbacks: %d %d\n",
        num_start_tesselation, num_combine_callback );
#endif
}


void tesselation(GLdouble *gldata, int ncontours, const int *count)
{
  int n,i,npos,j;
  j= 0;

  gluTessBeginPolygon(tess, NULL);

  for (n=0; n<ncontours; n++) {
    npos= count[n];

    gluTessBeginContour(tess);

    for (i=0; i<npos; i++) {
      gluTessVertex(tess, &gldata[j], &gldata[j]);
      j+=3;
    }
    gluTessEndContour(tess);
  }

  gluTessEndPolygon(tess);
}

#if 0
void optimized_tesselation(GLdouble *gldata, int ncontours, const int *count, int *to_small)
{
  int n,i,npos,j;

  int tesselate;

  tesselate = 0;

  for (n=0; n<ncontours; n++) {
    if (!to_small[n])
    {
      tesselate = 1;
      break;
    }
  }
  if (!tesselate)
    return;

  j= 0;

  //gluTessBeginPolygon(tess, NULL);
  beginTesselation();
  for (n=0; n<ncontours; n++) {
    npos= count[n];
    if (!to_small[n])
    {
      gluTessBeginPolygon(tess, NULL);
      gluTessBeginContour(tess);
    }

    for (i=0; i<npos; i++) {
      // the point it there, we must incremet j even if no tesselation!
      if (!to_small[n])
        gluTessVertex(tess, &gldata[j], &gldata[j]);
      j+=3;
    }
    if (!to_small[n])
    {
      gluTessEndContour(tess);
      gluTessEndPolygon(tess);
    }
  }
  endTesselation();
  //gluTessEndPolygon(tess);
}
#endif
