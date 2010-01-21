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
#include <diTesselation.h>
#include <GL/glu.h>
#include <stdio.h>


#ifndef GLCALLBACK
#ifdef CALLBACK
#define GLCALLBACK CALLBACK
#else
#define GLCALLBACK
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


static void GLCALLBACK error_callback( GLenum err )
{
   const GLubyte* errmsg;
   errmsg = gluErrorString( err );
   fprintf(stderr, "tesselation error_callback %d : %s\n",err,errmsg );
}

/*
void combineCallback(GLdouble coords[3], 
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut )
*/

static void GLCALLBACK combine_callback( GLdouble coords[3],
					 GLdouble *vertex_data[4],
					 GLfloat weight[4], GLdouble **data )
{
   GLdouble	*vertex;
#ifdef DEBUGCALLBACK
   num_combine_callback++;
#endif

#ifdef DEBUGEACHCALLBACK
   n_combine_callback++;
   fprintf(stderr, "tesselation combine_callback %d\n",n_combine_callback );
#endif

   vertex = (GLdouble *) malloc( 3 * sizeof(GLdouble) );

   vertex[0] = coords[0];
   vertex[1] = coords[1];
   vertex[2] = coords[2];
   *data = vertex;
}


void beginTesselation()
{
  tess= gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN, glBegin );
  gluTessCallback(tess, GLU_TESS_EDGE_FLAG, glEdgeFlag );
  gluTessCallback(tess, GLU_TESS_VERTEX, glVertex2dv );
  gluTessCallback(tess, GLU_TESS_END, glEnd );

  gluTessCallback(tess, GLU_TESS_ERROR, error_callback );
  gluTessCallback(tess, GLU_TESS_COMBINE, combine_callback );

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


void tesselation(GLdouble *gldata, int ncontours, int *count)
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

void optimized_tesselation(GLdouble *gldata, int ncontours, int *count, int *to_small)
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
