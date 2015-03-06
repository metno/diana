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

#include "diTrajectoryPlot.h"
#include <diField/diField.h>
#include <puTools/miStringFunctions.h>
#include <GL/gl.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.TrajectoryPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

TrajectoryPlot::TrajectoryPlot()
:Plot(){
  oldArea=getStaticPlot()->getMapArea();
  lineWidth=1;
  numMarker=1;
  markerRadius=50;
  timeMarker=0;
  fu1= fv1= 0;
  timeStep= 900.;
  numIterations= 5;
  computing= false;
  runningForward= runningBackward= 0;
  plot_on = true;
}


TrajectoryPlot::~TrajectoryPlot(){
  delete fu1;
  delete fv1;
  clearData();
}


bool TrajectoryPlot::prepare()
{
  METLIBS_LOG_SCOPE();

  //Change projection

  if (oldArea.P() == getStaticPlot()->getMapArea().P()) //nothing to do
    return true;

  int npos = x.size();
  if (npos==0)
    return true;

  float *xpos= new float[npos];
  float *ypos= new float[npos];

  for (int i=0; i<npos; i++){
    xpos[i] = x[i];
    ypos[i] = y[i];
  }

  // convert points to correct projection
  getStaticPlot()->ProjToMap(oldArea.P(), npos, xpos, ypos);

  for (int i=0; i<npos; i++){
    x[i] = xpos[i];
    y[i] = ypos[i];
  }

  delete[] xpos;
  delete[] ypos;

  oldArea = getStaticPlot()->getMapArea();
  return true;
}

int TrajectoryPlot::trajPos(const vector<string>& vstr)
{
  METLIBS_LOG_SCOPE();
  if (METLIBS_LOG_DEBUG_ENABLED()) {
    for(size_t i=0;i<vstr.size();i++)
      METLIBS_LOG_DEBUG(vstr[i]);
  }

  int action= 0;  // action to be taken by PlotModule (0=none)

  int nvstr = vstr.size();
  for(int k=0; k<nvstr; k++){

    vector<float> longitude;
    vector<float> latitude;

    std::string value,orig_value,key;
    std::string pin = vstr[k];
    vector<std::string> tokens = miutil::split_protected(pin, '"','"');
    int n = tokens.size();

    for( int i=0; i<n; i++){
      vector<std::string> stokens = miutil::split(tokens[i], 0, "=");
      if (METLIBS_LOG_DEBUG_ENABLED()) {
        METLIBS_LOG_DEBUG("stokens:");
        for (size_t j=0; j<stokens.size(); j++)
          METLIBS_LOG_DEBUG("  " << stokens[j]);
      }
      if( stokens.size() == 1) {
        key= miutil::to_lower(stokens[0]);
        if (key == "clear") {
          clearData();
          action= 1;  // remove annotation
        }
        else if (key == "delete"){
          stopComputation();
          lat.clear();
          lon.clear();
          x.clear();
          y.clear();
        }
      } else if( stokens.size() == 2) {
        key        = miutil::to_lower(stokens[0]);
        orig_value = stokens[1];
        value      = miutil::to_lower(stokens[1]);
        if (key == "plot" ){
          if(value == "on")
            plot_on = true;
          else
            plot_on = false;
          action= 1;  // add or remove annotation
        } else if (key == "longitudelatitude" ) {
          stopComputation();
          vector<std::string> lonlat = miutil::split(value, 0, ",");
          int npos=lonlat.size()/2;
          for( int i=0; i<npos; i++){
            longitude.push_back(atof(lonlat[2*i].c_str()));
            latitude.push_back(atof(lonlat[2*i+1].c_str()));
          }
        } else if (key == "latitudelongitude" ) {
          stopComputation();
          vector<std::string> latlon = miutil::split(value, 0, ",");
          int npos=latlon.size()/2;
          for( int i=0; i<npos; i++){
            latitude.push_back(atof(latlon[2*i].c_str()));
            longitude.push_back(atof(latlon[2*i+1].c_str()));
          }
        } else if (key == "field" ) {
          if (orig_value[0]=='"')
            fieldStr= orig_value.substr(1,orig_value.length()-2);
          else
            fieldStr = orig_value;
        } else if (key == "colour" )
          colour = value;
        else if (key == "linewidth" )
          lineWidth = atoi(value.c_str());
        else if (key == "linetype" )
          lineType = Linetype(value);
        else if (key == "radius" )
          markerRadius = atoi(value.c_str());
        else if (key == "numpos" )
          numMarker = atoi(value.c_str());
        else if (key == "timemarker" )
          timeMarker = atoi(value.c_str());
      }
    }

    //if no positions are given, return
    int nlon=longitude.size();
    if (nlon==0)
      continue;

    //add numMarker positions

    float *xpos= new float[nlon];
    float *ypos= new float[nlon];
    for (int i=0; i<nlon; i++){
      xpos[i] = longitude[i];
      ypos[i] = latitude[i];
    }

    getStaticPlot()->GeoToMap(nlon, xpos, ypos);

    if (numMarker==5 || numMarker==9){

      float *xnew= new float[nlon*numMarker];
      float *ynew= new float[nlon*numMarker];

      float s= 0.5 * sqrtf(2.0f);
      float cx[9]= { 0.,  0., -1.,  0., +1., -s, -s, +s, +s };
      float cy[9]= { 0., -1.,  0., +1.,  0., -s, +s, +s, -s };
      int n=0;

      for (int i=0; i<nlon; i++) {

        float dlat,dlon;
        getStaticPlot()->getMapArea().P().getLatLonIncrement(latitude[i],longitude[i],dlat,dlon);
        float lats[2] = { latitude[i], float(markerRadius)*1000*dlat + latitude[i] };
        float lons[2] = { float(markerRadius)*1000*dlon + longitude[i], longitude[i] };
        getStaticPlot()->GeoToMap(2,lons,lats);
        float dx=lons[0] - xpos[i];
        float dy=lats[1] - ypos[i];
        for (int j=0; j<numMarker; j++, n++) {
          xnew[n]= xpos[i] + dx*cx[j];
          ynew[n]= ypos[i] + dy*cy[j];
        }
      }

      for (int i=0; i<n; i++) {
        x.push_back(xnew[i]);
        y.push_back(ynew[i]);
      }

      getStaticPlot()->MapToGeo(n,xnew,ynew);

      for (int i=0; i<n; i++) {
        lon.push_back(xnew[i]);
        lat.push_back(ynew[i]);
      }

      delete[] xnew;
      delete[] ynew;

    } else {

      for (int i=0; i<nlon; i++) {
        lat.push_back(latitude[i]);
        lon.push_back(longitude[i]);
        x.push_back(xpos[i]);
        y.push_back(ypos[i]);
      }

    }

    delete[] xpos;
    delete[] ypos;

    if (METLIBS_LOG_DEBUG_ENABLED()) {
      const int nlon = lon.size();
      METLIBS_LOG_DEBUG("  lon.size= "<<nlon);
      for (int i=0; i<nlon; i++) {
        METLIBS_LOG_DEBUG("   i,lat,lon,x,y: "<<i<<"  "<<lat[i]<<" "<<lon[i]
            <<"    "<<x[i]<<" "<<y[i]);
        int ndup=0;
        for (int j=i+1; j<nlon; j++)
          if (lat[i]==lat[j] && lon[i]==lon[j])
            ndup++;
        if (ndup>0)
          METLIBS_LOG_DEBUG("   duplikat i,lat,lon,x,y: "
              <<i<<"  "<<lat[i]<<" "<<lon[i]
              <<"    "<<xpos[i]<<" "<<ypos[i]<<"  ndup= "<<ndup);
      }
    }
  }

  return action;
}


void TrajectoryPlot::plot(PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();

  if (!plot_on || !isEnabled() || zorder != LINES)
    return;

  if (colour==getStaticPlot()->getBackgroundColour())
    colour= getStaticPlot()->getBackContrastColour();
  glColor4ubv(colour.RGBA());
  glLineWidth(float(lineWidth)+0.1f);

  float d= 5*getStaticPlot()->getPhysToMapScaleX();

  if (vtrajdata.size()==0) {

    // plot start posistions

    int m = x.size();
    glBegin(GL_LINES);
    for (int i=0; i<m; i++) {
      glVertex2f(x[i]-d,y[i]-d);
      glVertex2f(x[i]+d,y[i]+d);
      glVertex2f(x[i]-d,y[i]+d);
      glVertex2f(x[i]+d,y[i]-d);
    }
    glEnd();

  } else {


    // plot trajectories

    int vtsize= vtrajdata.size();

    for (int n=0; n<vtsize; n++) {
      if (vtrajdata[n]->area.P() != getStaticPlot()->getMapArea().P()) {
        int npos= numTraj * vtrajdata[n]->ndata;
        if (!getStaticPlot()->ProjToMap(vtrajdata[n]->area.P(),
                npos, vtrajdata[n]->x, vtrajdata[n]->y))
        {
          METLIBS_LOG_ERROR("TrajectoryPlot::plot  getPoints ERROR");
          return;
        }
        vtrajdata[n]->area= getStaticPlot()->getMapArea();
      }
    }


    vector <float> xmark,ymark;
    TrajectoryData *td;
    for (int i=0; i<numTraj; i++) {
      //      METLIBS_LOG_DEBUG("Traj no:"<<i);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(lineType.factor,lineType.bmap);
      glBegin(GL_LINE_STRIP);
      for (int n=0; n<vtsize; n++) {
        //        METLIBS_LOG_DEBUG("??:"<<n);
        td= vtrajdata[n];
        int j1= td->first[i];
        int j2= td->last[i] + 1;
        if (j1<j2) {
          int begin= td->ndata * i;
          for (int j=j1; j<j2; j++){
            //	    METLIBS_LOG_DEBUG("x:"<<td->x[begin+j]<<"  y:"<< td->y[begin+j]);
            glVertex2f(td->x[begin+j], td->y[begin+j]);
            miTime thistime = td->time[j];
            int diff = miTime::minDiff(firstTime,thistime);
            if (timeMarker && (n<vtsize-1 || j<j2-1)
                &&  diff%timeMarker==0){
              xmark.push_back(td->x[begin+j]);
              ymark.push_back(td->y[begin+j]);
            }
          }
        }
      }
      glEnd();
      glDisable(GL_LINE_STIPPLE);

      int nmark=xmark.size();

      glBegin(GL_LINES);
      for (int ih=1;ih<nmark;ih++){
        float  deltay = ymark[ih]-ymark[ih-1];
        float  deltax = xmark[ih]-xmark[ih-1];
        float hyp = sqrtf(deltay*deltay+deltax*deltax);
        float dx = d*deltay/hyp;
        float dy = d*deltax/hyp;
        float x1=xmark[ih]-dx;
        float y1=ymark[ih]+dy;
        float x2=xmark[ih]+dx;
        float y2=ymark[ih]-dy;
        glVertex2f(x1,y1);
        glVertex2f(x2,y2);
      }
      glEnd();
      xmark.clear();
      ymark.clear();

    }


    int n,j1,j2,ndata;
    float r,x,y,dx,dy,dr;

    // "x" in first position (in direction of movement, as data are stored)
    n= 0;
    td= vtrajdata[n];
    ndata= td->ndata;
    j1= 0;
    j2= 1;
    r= d*0.8;
    glBegin(GL_LINES);
    for (int i=0; i<numTraj; i++) {
      if (runningBackward[i]) {
        x=  td->x[ndata*i+j1];
        y=  td->y[ndata*i+j1];
        dx= x - td->x[ndata*i+j2];
        dy= y - td->y[ndata*i+j2];
        dr= sqrtf(dx*dx+dy*dy);
        dx/=dr;
        dy/=dr;
        glVertex2f(x+r*(dx-dy), y+r*(dy+dx));
        glVertex2f(x-r*(dx-dy), y-r*(dy+dx));
        glVertex2f(x+r*(dx+dy), y+r*(dy-dx));
        glVertex2f(x-r*(dx+dy), y-r*(dy-dx));
      }
    }
    glEnd();

    // arrow in last position (in direction of movement, as data are stored)
    n= vtsize - 1;
    td= vtrajdata[n];
    ndata= td->ndata;
    j1= ndata - 1;
    j2= ndata - 2;
    r= d*1.3;
    glBegin(GL_LINES);
    for (int i=0; i<numTraj; i++) {
      if (runningForward[i]) {
        x=  td->x[ndata*i+j1];
        y=  td->y[ndata*i+j1];
        dx= x - td->x[ndata*i+j2];
        dy= y - td->y[ndata*i+j2];
        dr= sqrtf(dx*dx+dy*dy);
        dx/=dr;
        dy/=dr;
        glVertex2f(x-r*(dx+dy), y-r*(dy-dx));
        glVertex2f(x,y);
        glVertex2f(x,y);
        glVertex2f(x-r*(dx-dy), y-r*(dy+dx));
      }
    }
    glEnd();

    // mark current time (not at first and last time)
    if (getStaticPlot()->getTime()>firstTime && getStaticPlot()->getTime()<lastTime) {

      r= d*1.4;
      const int nc= 16;
      float xc[nc];
      float yc[nc];
      float cstep= 2 * M_PI / float(nc);
      for (int j=0; j<nc; j++) {
        xc[j]= r * cosf(cstep*float(j));
        yc[j]= r * sinf(cstep*float(j));
      }

      int nt= -1;
      int it= -1;
      n=0;
      while (nt<0 && n<vtsize) {
        td= vtrajdata[n];
        ndata= td->ndata;
        it= 0;
        while (it<ndata && getStaticPlot()->getTime() > td->time[it]) it++;
        if (it<ndata) nt= n;
        n++;
      }
      if (nt>=0) {
        td= vtrajdata[nt];
        ndata= td->ndata;
        r= d;
        for (int i=0; i<numTraj; i++) {
          if (it>=td->first[i] && it<=td->last[i]) {
            x= td->x[ndata*i+it];
            y= td->y[ndata*i+it];
            glBegin(GL_LINE_LOOP);
            for (int j=0; j<nc; j++)
              glVertex2f(x+xc[j], y+yc[j]);
            glEnd();
          }
        }
      }
    }
  }
}


bool TrajectoryPlot::startComputation(vector<Field*> vf)
{
  METLIBS_LOG_SCOPE();

  stopComputation();  //in case not done before..
  clearData();

  if (vf.size()<2)  return false;
  if (!vf[0])       return false;
  if (!vf[0]->data) return false;
  if (!vf[1])       return false;
  if (!vf[1]->data) return false;

  fu1= new Field();
  fv1= new Field();
  *(fu1)= *(vf[0]); // copy fields
  *(fv1)= *(vf[1]);
  firstTime= lastTime= fu1->validFieldTime;
  fieldArea= fu1->area;

  computing= true;
  firstStep= true;

  return true;
}


void TrajectoryPlot::stopComputation()
{
  METLIBS_LOG_SCOPE();

  // remove fields etc...
  // mark as not running

  delete fu1;
  delete fv1;
  fu1= fv1= 0;

  computing= false;
  firstStep= true;
}


void TrajectoryPlot::clearData()
{
  METLIBS_LOG_SCOPE();

  int n= vtrajdata.size();
  for (int i=0; i<n; i++) {
    delete[] vtrajdata[i]->x;
    delete[] vtrajdata[i]->y;
    delete[] vtrajdata[i]->first;
    delete[] vtrajdata[i]->last;
  }
  vtrajdata.clear();
  delete[] runningForward;
  delete[] runningBackward;
  runningForward= runningBackward= 0;
}


bool TrajectoryPlot::compute(vector<Field*> vf)
{
  // return true  if trajectories are computed (changed)
  // return false if trajectories are not computed, this is not an error!
  METLIBS_LOG_SCOPE();

  if (!computing) return false;

  if (vf.size()<2)  return false;
  if (!vf[0])       return false;
  if (!vf[0]->data) return false;
  if (!vf[1])       return false;
  if (!vf[1]->data) return false;

  Field *fu2= vf[0]; // only set pointer
  Field *fv2= vf[1];

  if (fu2->area != fieldArea) return false;

  miTime t2= fu2->validFieldTime;

  if (!firstStep) {
    if (t2>firstTime && t2<lastTime) {
      // trajectories are computed before
      delete fu1;
      delete fv1;
      fu1= fv1= 0;
      return false;
    } else if (t2==firstTime || t2==lastTime) {
      // may be computing trajectories next timestep
      delete fu1;
      delete fv1;
      fu1= new Field();
      fv1= new Field();
      *(fu1)= *(fu2);  // copy fields
      *(fv1)= *(fv2);
      return false;
    }
  }

  if (!fu1 || !fv1) return false;  // when looping timeseries

  miTime t1= fu1->validFieldTime;

  bool forward;

  if (t2>t1)
    forward= true;
  else if (t2<t1)
    forward= false;
  else
    return false;  // may happen ???????????

  float seconds= miTime::secDiff(t2, t1);

  int nstep= int(fabsf(seconds)/timeStep + 0.5);
  if (nstep<1) nstep= 1;
  float tStep= seconds/float(nstep);

  int npos=0;
  float *sx= 0;
  float *sy= 0;

  if (firstStep) {

    // start positions
    npos= lon.size();

    sx= new float[npos];
    sy= new float[npos];
    float *su= new float[npos];
    float *sv= new float[npos];

    for (int i=0; i<npos; i++) {
      sx[i]= lon[i];
      sy[i]= lat[i];
    }

    fu1->area.P().convertFromGeographic(npos,sx,sy);
    fu1->convertToGrid(npos,sx,sy);

    // check if inside area and not in undefined area of the fields
    int interpoltype= 101;
    fu1->interpolate(npos, sx, sy, su, interpoltype);
    fv1->interpolate(npos, sx, sy, sv, interpoltype);

    numTraj= 0;

    for (int i=0; i<npos; i++) {
      METLIBS_LOG_DEBUG("x,y,u,v:  "<<sx[i]<<"  "<<sy[i]<<"   "<<su[i]<<"  "<<sv[i]);
      if(su[i]!=fieldUndef && sv[i]!=fieldUndef) {
        sx[numTraj]= sx[i];
        sy[numTraj]= sy[i];
        numTraj++;
      }
    }
    METLIBS_LOG_DEBUG("npos,numTraj: "<<npos<<" "<<numTraj);

    delete[] su;
    delete[] sv;

    if (numTraj==0) {
      delete[] sx;
      delete[] sy;
      stopComputation();
      return false;
    }

  }

  // map ratios
  float *xmapr, *ymapr, *coriolis;
  float dxgrid, dygrid;
  int imapr=2;  // xmapratio/dxgrid and ymapratio/dygrid
  // mapratios in Norlam style (inverse of Hirlam style)
  int icori=0;
  if (!getStaticPlot()->gc.getMapFields(fu1->area, imapr, icori,
      fu1->nx, fu1->ny, &xmapr, &ymapr, &coriolis,
      dxgrid, dygrid)) {
    METLIBS_LOG_ERROR("TrajectoryPlot::compute : getStaticPlot()->gc.getMapFields ERROR."
        <<"  Cannot compute trajectories !");
    stopComputation();
    return false;
  }

  Field* frx= new Field();
  frx->nx=     fu1->nx;
  frx->ny=     fu1->ny;
  frx->area=   fu1->area;
  frx->allDefined= true;
  frx->data=   xmapr;
  Field* fry= new Field();
  fry->nx=     fu1->nx;
  fry->ny=     fu1->ny;
  fry->area=   fu1->area;
  fry->allDefined= true;
  fry->data=   ymapr;

  int ndata= nstep;
  if (firstStep) ndata++;

  TrajectoryData *trajdata= new TrajectoryData();

  trajdata->area=  fu1->area;
  trajdata->ndata= ndata;
  trajdata->time=  new miTime[ndata];
  trajdata->first= new int[numTraj];
  trajdata->last=  new int[numTraj];
  trajdata->x=     new float[numTraj*ndata];
  trajdata->y=     new float[numTraj*ndata];

  float *xt= new float[numTraj];
  float *yt= new float[numTraj];
  float *xa= new float[numTraj];
  float *ya= new float[numTraj];
  float *u1= new float[numTraj];
  float *v1= new float[numTraj];
  float *u2= new float[numTraj];
  float *v2= new float[numTraj];
  float *rx= new float[numTraj];
  float *ry= new float[numTraj];

  int idata,incdata;

  if (forward) {
    idata= 0;
    incdata= 1;
  } else {
    idata= ndata - 1;
    incdata= -1;
  }

  if (firstStep) {

    for (int i=0; i<numTraj; i++) {
      trajdata->x[ndata*i+idata]= xt[i]= sx[i];
      trajdata->y[ndata*i+idata]= yt[i]= sy[i];
      fu1->convertFromGrid(1, &trajdata->x[ndata*i+idata], &trajdata->y[ndata*i+idata]);
    }

    delete[] sx;
    delete[] sy;
    sx= sy= 0;
    trajdata->time[idata]= t1;
    idata+=incdata;
    runningForward=  new bool[numTraj];
    runningBackward= new bool[numTraj];
    for (int i=0; i<numTraj; i++)
      runningForward[i]= runningBackward[i]= true;

  } else {

    int n,nd,id;
    if (forward) {
      n=  vtrajdata.size() - 1;
      nd= vtrajdata[n]->ndata;
      id= nd - 1;
    } else {
      n=  0;
      nd= vtrajdata[n]->ndata;
      id= 0;
    }
    for (int i=0; i<numTraj; i++) {
      xt[i]= vtrajdata[n]->x[nd*i+id];
      yt[i]= vtrajdata[n]->y[nd*i+id];
    }

    if (vtrajdata[n]->area.P() != fieldArea.P()) {
      // posistions are converted to a different map projection
      if(!getStaticPlot()->gc.getPoints(vtrajdata[n]->area.P(),fieldArea.P(),numTraj,xt,yt)) {
        METLIBS_LOG_ERROR("TrajectoryPlot::compute : getStaticPlot()->gc.getMapFields ERROR."
            <<"  Trajectory computation stopped !");
        stopComputation();
        return false;
      }
    }
    fu1->convertToGrid(numTraj,xt, yt);

  }

  bool *running;
  int  *endnum;

  if (forward) {
    running= runningForward;
    endnum=  trajdata->last;
  } else {
    running= runningBackward;
    endnum=  trajdata->first;
  }

  for (int i=0; i<numTraj; i++) {
    if (running[i]) {
      trajdata->first[i]= 0;
      trajdata->last[i]=  ndata-1;
    } else {
      trajdata->first[i]=  0;
      trajdata->last[i]=  -1;
    }
  }

  //stop trajectory if it is more than one grid point outside the field
  for (int i=0; i<numTraj; i++) {
    if (xt[i] < -1 || xt[i]>fu1->nx || yt[i] < -1 || yt[i] > fu1->ny) {
      endnum[i]=  idata - incdata;
      running[i]=false;
    }
  }

  float dt= tStep * 0.5;
  float u,v;

  for (int istep=0; istep<nstep; istep++) {
    METLIBS_LOG_DEBUG("istep,nstep,tStep: "<<istep<<" "<<nstep<<" "<<tStep);

    float ct1b= float(istep)/float(nstep);
    float ct1a= 1.0 - ct1b;
    float ct2b= float(istep+1)/float(nstep);
    float ct2a= 1.0 - ct2b;

    // iteration no. 0 to get a first guess (then the real iterations)

    for (int iter=0; iter<=numIterations; iter++) {
      METLIBS_LOG_DEBUG("   iter: "<<iter);

      int interpoltype= 101;
      fu1->interpolate(numTraj, xt, yt, u1, interpoltype);
      fv1->interpolate(numTraj, xt, yt, v1, interpoltype);
      fu2->interpolate(numTraj, xt, yt, u2, interpoltype);
      fv2->interpolate(numTraj, xt, yt, v2, interpoltype);
      frx->interpolate(numTraj, xt, yt, rx, interpoltype);
      fry->interpolate(numTraj, xt, yt, ry, interpoltype);

      //stop trajectory if field is undef
      for (int i=0; i<numTraj; i++) {
        if (running[i]) {
          if (u1[i]==fieldUndef || v1[i]==fieldUndef ||
              u2[i]==fieldUndef || v2[i]==fieldUndef) {
            running[i]= false;
            endnum[i]=  idata - incdata;
          }
        }
      }

      if (iter==0) {
        for (int i=0; i<numTraj; i++) {
          if (running[i]) {
            u= ct1a * u1[i] + ct1b * u2[i];
            v= ct1a * v1[i] + ct1b * v2[i];
            xa[i]= xt[i] + rx[i] * u * dt;
            ya[i]= yt[i] + ry[i] * v * dt;
          }
        }
      }

      for (int i=0; i<numTraj; i++) {
        if (running[i]) {
          u= ct2a * u1[i] + ct2b * u2[i];
          v= ct2a * v1[i] + ct2b * v2[i];
          xt[i]= xa[i] + rx[i] * u * dt;
          yt[i]= ya[i] + ry[i] * v * dt;
        }
      }

    }  // end of iteration loop

    for (int i=0; i<numTraj; i++) {
      if (running[i]) {
        trajdata->x[ndata*i+idata]= xt[i];
        trajdata->y[ndata*i+idata]= yt[i];
      } else {
        trajdata->x[ndata*i+idata]= HUGE_VAL;
        trajdata->y[ndata*i+idata]= HUGE_VAL;
      }
      fu1->convertFromGrid(1, &trajdata->x[ndata*i+idata], &trajdata->y[ndata*i+idata]);
    }

    miTime tnow= t1;
    if (forward)
      tnow.addSec(int(tStep*(istep+1) + 0.5));
    else
      tnow.addSec(int(tStep*(istep+1) - 0.5));

    trajdata->time[idata]= tnow;

    idata+=incdata;

  }

  if (forward)
    vtrajdata.push_back(trajdata);
  else
    vtrajdata.push_front(trajdata);

  if (forward)
    lastTime= t2;
  else
    firstTime= t2;

  frx->data= 0;  // was just a pointer to the GridConverter data !
  fry->data= 0;  // was just a pointer to the GridConverter data !
  delete frx;
  delete fry;

  // store the current fields;
  delete fu1;
  delete fv1;
  fu1= new Field();
  fv1= new Field();
  *(fu1)= *(fu2);  // copy fields
  *(fv1)= *(fv2);

  firstStep= false;

  return true;
}


void TrajectoryPlot::getTrajectoryAnnotation(string& s, Colour& c)
{
  //#### if (vtrajdata.size()>0) {
  if (plot_on && vtrajdata.size()>0) {
    int l= 16;
    if (firstTime.min()==0 && lastTime.min()==0) l= 13;
    s= "Trajektorier " + fieldStr
        + " "   + firstTime.isoTime().substr(0,l)
        + " - " +  lastTime.isoTime().substr(0,l) + " UTC";
    if (colour==getStaticPlot()->getBackgroundColour())
      c= getStaticPlot()->getBackContrastColour();
    else
      c= colour;
  } else {
    s= "";
  }

  // ADC temporary hack
  setPlotName(s);
}


bool TrajectoryPlot::printTrajectoryPositions(const std::string& filename)
{
  //output
  ofstream fs;

  fs.open(filename.c_str());

  if(!fs){
    METLIBS_LOG_ERROR("ERROR  printTrajectoryPositions: can't open file: "
        <<filename);
    return false;
  }


  int vtsize= vtrajdata.size();

  if(vtsize==0) return false;

  int npos = (vtsize + 1) * numTraj;
  float xxx[npos];
  float yyy[npos];
  //start points
  for (int i=0;i<numTraj;i++){
    xxx[i]=vtrajdata[0]->x[i*vtrajdata[0]->ndata];
    yyy[i]=vtrajdata[0]->y[i*vtrajdata[0]->ndata];
  }

  //next points
  for (int n=0; n<vtsize; n++) {
    for (int i=0;i<numTraj;i++){
      xxx[(n+1)*numTraj+i]=vtrajdata[n]->x[(i+1)*(vtrajdata[n]->ndata)-1];
      yyy[(n+1)*numTraj+i]=vtrajdata[n]->y[(i+1)*(vtrajdata[n]->ndata)-1];
    }
  }
  getStaticPlot()->MapToGeo(npos,xxx,yyy);

  //print to file
  fs <<"[NAME TRAJECTORY]"<<endl;
  fs <<"[COLUMNS "<<endl;
  fs <<"Date:d   Time:t      Lon:r   Lat:r   No:r]"<<endl;
  fs <<"[DATA]"<<endl;
  for (int i=0;i<numTraj;i++){
    fs.setf(ios::showpoint);
    fs.precision(7);
    fs <<(*vtrajdata[0]->time).isoTime()<<"  ";
    fs <<setw(10) <<xxx[i]<<"  "<<
        setw(10) <<yyy[i]<<"  T"<<i<<endl;
  }

  for (int n=0; n<vtsize; n++) {
    for (int i=0;i<numTraj;i++){
      fs <<vtrajdata[n]->time[vtrajdata[n]->ndata-1].isoTime();
      fs <<"  "<<setw(10) <<xxx[(n+1)*numTraj+i]<<"  "<<
          setw(10) <<yyy[(n+1)*numTraj+i]<<"  T"<< i<<endl;
    }
  }

  fs.close();
  return true;
}
