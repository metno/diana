/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diRadarEchoPlot.cc 1 2007-09-12 08:06:42Z lisbethb $

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

#include <fstream>
#include <diRadarEchoPlot.h>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include <diField/diField.h>
#include <GL/gl.h>

using namespace std; using namespace miutil;


RadarEchoPlot::RadarEchoPlot()
:Plot(){
  oldArea=area;
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


RadarEchoPlot::~RadarEchoPlot(){
  delete fu1;
  delete fv1;
  clearData();
}


bool RadarEchoPlot::prepare(void){
#ifdef DEBUGPRINT
  cerr << "++ RadarEchoPlot::prepare() ++" << endl;
#endif

  //Change projection

  if (oldArea.P() == area.P()) //nothing to do
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
  gc.getPoints(oldArea.P(),area.P(),npos,xpos,ypos);//) {

  for (int i=0; i<npos; i++){
    x[i] = xpos[i];
    y[i] = ypos[i];
  }

  delete[] xpos;
  delete[] ypos;

  oldArea = area;
  return true;
}

int RadarEchoPlot::radePos(vector<miString>& vstr)
{
#ifdef DEBUGPRINT
  for(int i=0;i<vstr.size();i++)
    cerr << "++ RadarEchoPlot::radePos() " << vstr[i] << endl;
#endif

  int action= 0;  // action to be taken by PlotModule (0=none)

  int nvstr = vstr.size();
  for(int k=0; k<nvstr; k++){

    vector<float> longitude;
    vector<float> latitude;

    miString value,orig_value,key;
    miString pin = vstr[k];
    vector<miString> tokens = pin.split('"','"');
    int n = tokens.size();

    for( int i=0; i<n; i++){
      vector<miString> stokens = tokens[i].split('=');
#ifdef DEBUGPRINT
      cerr << "stokens:";
      for (int j=0; j<stokens.size(); j++)
        cerr << "  " << stokens[j];
      cerr << endl;
#endif
      if( stokens.size() == 1) {
        key= stokens[0].downcase();
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
        key        = stokens[0].downcase();
        orig_value = stokens[1];
        value      = stokens[1].downcase();
        if (key == "plot" ){
          if(value == "on")
            plot_on = true;
          else
            plot_on = false;
          action= 1;  // add or remove annotation
        } else if (key == "longitudelatitude" ) {
          stopComputation();
          vector<miString> lonlat = value.split(',');
          int npos=lonlat.size()/2;
          for( int i=0; i<npos; i++){
            longitude.push_back(atof(lonlat[2*i].c_str()));
            latitude.push_back(atof(lonlat[2*i+1].c_str()));
          }
        } else if (key == "latitudelongitude" ) {
          stopComputation();
          vector<miString> latlon = value.split(',');
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
        } else if (key == "colour" ) {

          colour = value;
          //colour = "name: black red: 000 green: 000 blue: 000 alpha: 255 Index: 52";
        }
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
#ifdef DEBUGPRINT
    cerr << "\n\nnumMarker: " << numMarker << "\n"<< endl;
    cerr << "\nmarkerRadius: " << markerRadius << "\n\n" << endl;
    cerr << "\ncolour: " << colour << "\n\n" << endl;
#endif
    //    colour = "name: black red: 000 green: 000 blue: 000 alpha: 255 Index: 52";

    float *xpos= new float[nlon];
    float *ypos= new float[nlon];
    for (int i=0; i<nlon; i++){
      xpos[i] = longitude[i];
      ypos[i] = latitude[i];
    }
    gc.geo2xy(area,nlon,xpos,ypos);

    if (numMarker==5 || numMarker==9){

      float *xnew= new float[nlon*numMarker];
      float *ynew= new float[nlon*numMarker];

      float s= 0.5 * sqrtf(2.0f);
      float cx[9]= { 0.,  0., -1.,  0., +1., -s, -s, +s, +s };
      float cy[9]= { 0., -1.,  0., +1.,  0., -s, +s, +s, -s };
      int n=0;

      for (int i=0; i<nlon; i++) {

        float dlat,dlon;
        Projection::getLatLonIncrement(latitude[i],longitude[i],dlat,dlon);
        float lat1 = latitude[i];
        float lon1 = float(markerRadius)*1000*dlon + longitude[i];
        float lat2 = float(markerRadius)*1000*dlat + latitude[i];
        float lon2 = longitude[i];
        int one=1;
        gc.geo2xy(area,one,&lon1,&lat1);
        gc.geo2xy(area,one,&lon2,&lat2);
        float dx=lon1 - xpos[i];
        float dy=lat2-ypos[i];

        for (int j=0; j<numMarker; j++, n++) {
          xnew[n]= xpos[i] + dx*cx[j];
          ynew[n]= ypos[i] + dy*cy[j];
        }
      }

      for (int i=0; i<n; i++) {
        x.push_back(xnew[i]);
        y.push_back(ynew[i]);
      }

      gc.xy2geo(area,n,xnew,ynew);

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

#ifdef DEBUGPRINT
    nlon = lon.size();
    for (int i=0; i<nlon; i++){
      cerr<<"   i,lat,lon,x,y: "<<i<<"  "<<lat[i]<<" "<<lon[i]
                                                            <<"    "<<xpos[i]<<" "<<ypos[i]<<endl;
    }
#endif

#ifdef DEBUGPRINT
    cerr<<"RadarEchoPlot::trajPos  lon.size= "<<lon.size()<<endl;
    for (int i=0; i<nlon; i++){
      int ndup=0;
      for (int j=i+1; j<nlon; j++)
        if (lat[i]==lat[j] && lon[i]==lon[j]) ndup++;
      if (ndup>0)
        cerr<<"   duplikat i,lat,lon,x,y: "<<i<<"  "<<lat[i]<<" "<<lon[i]
                                                                       <<"    "<<xpos[i]<<" "<<ypos[i]<<"  ndup= "<<ndup<<endl;
    }
#endif
  }

  return action;
}


bool RadarEchoPlot::plot(){
#ifdef DEBUGPRINT
  cerr << "++ RadarEchoPlot::plot() ++" << endl;
#endif

  if (!plot_on || !enabled)
    return false;

  if (colour==backgroundColour)
    colour= backContrastColour;
  glColor4ubv(colour.RGBA());
  glLineWidth(float(lineWidth)+0.1f);

  float d= 5*fullrect.width()/pwidth;

  if (vradedata.size()==0) {

    // plot start posistions

    int m = x.size();
    glBegin(GL_LINES);
    for (int i=0; i<m; i++) {
      //       glVertex2f(pos[i].x-d,pos[i].y-d);
      //       glVertex2f(pos[i].x+d,pos[i].y+d);
      //       glVertex2f(pos[i].x-d,pos[i].y+d);
      //       glVertex2f(pos[i].x+d,pos[i].y-d);
      glVertex2f(x[i]-d,y[i]-d);
      glVertex2f(x[i]+d,y[i]+d);
      glVertex2f(x[i]-d,y[i]+d);
      glVertex2f(x[i]+d,y[i]-d);
    }
    glEnd();

  } else {


    // plot trajectories

    int vtsize= vradedata.size();



    for (int n=0; n<vtsize; n++) {
      if (vradedata[n]->area.P() != area.P()) {
        int npos= numTraj * vradedata[n]->ndata;
        if (!gc.getPoints(vradedata[n]->area.P(), area.P(),
            npos, vradedata[n]->x, vradedata[n]->y)) {
          cerr << "RadarEchoPlot::plot  getPoints ERROR" << endl;
          return false;
        }
        vradedata[n]->area= area;
      }
    }


    vector <float> xmark,ymark;
    RadarEchoData *td;
    for (int i=0; i<numTraj; i++) {
      //      cerr <<"Traj no:"<<i<<endl;
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(lineType.factor,lineType.bmap);
      glBegin(GL_LINE_STRIP);
      for (int n=0; n<vtsize; n++) {
        //	cerr <<"??:"<<n<<endl;
        td= vradedata[n];
        int j1= td->first[i];
        int j2= td->last[i] + 1;
        if (j1<j2) {
          int begin= td->ndata * i;
          for (int j=j1; j<j2; j++){
            //	    cerr <<"x:"<<td->x[begin+j]<<"  y:"<< td->y[begin+j]<<endl;
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
    td= vradedata[n];
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
    td= vradedata[n];
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
    if (ctime>firstTime && ctime<lastTime) {

      r= d*1.4;
      const int nc= 16;
      float xc[nc];
      float yc[nc];
      float cstep= 2 * acosf(-1.0) / float(nc);
      for (int j=0; j<nc; j++) {
        xc[j]= r * cosf(cstep*float(j));
        yc[j]= r * sinf(cstep*float(j));
      }

      int nt= -1;
      int it= -1;
      n=0;
      while (nt<0 && n<vtsize) {
        td= vradedata[n];
        ndata= td->ndata;
        it= 0;
        while (it<ndata && ctime > td->time[it]) it++;
        if (it<ndata) nt= n;
        n++;
      }
      if (nt>=0) {
        td= vradedata[nt];
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

  return true;
}


bool RadarEchoPlot::startComputation(vector<Field*> vf)
{
#ifdef DEBUGPRINT
  cerr << "++ RadarEchoPlot::startComputation" << endl;
#endif

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

#ifdef DEBUGPRINT
  cerr << ".....OK" << endl;
#endif
  return true;
}


void RadarEchoPlot::stopComputation()
{
#ifdef DEBUGPRINT
  cerr << "++ RadarEchoPlot::stopComputation" << endl;
#endif
  // remove fields etc...
  // mark as not running

  delete fu1;
  delete fv1;
  fu1= fv1= 0;

  computing= false;
  firstStep= true;

}


void RadarEchoPlot::clearData()
{
#ifdef DEBUGPRINT
  cerr << "++ RadarEchoPlot::clearData" << endl;
#endif

  int n= vradedata.size();
  for (int i=0; i<n; i++) {
    delete[] vradedata[i]->x;
    delete[] vradedata[i]->y;
    delete[] vradedata[i]->first;
    delete[] vradedata[i]->last;
  }
  vradedata.clear();
  delete[] runningForward;
  delete[] runningBackward;
  runningForward= runningBackward= 0;
}


bool RadarEchoPlot::compute(vector<Field*> vf)
{
  // return true  if trajectories are computed (changed)
  // return false if trajectories are not computed, this is not an error!
#ifdef DEBUGPRINT
  cerr << "++ RadarEchoPlot::compute" << endl;
#endif

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

    gc.geo2xy(fu1->area,npos,sx,sy);

    // check if inside area and not in undefined area of the fields
    int interpoltype= 1;
    fu1->interpolate(npos, sx, sy, su, interpoltype);
    fv1->interpolate(npos, sx, sy, sv, interpoltype);

    numTraj= 0;

    for (int i=0; i<npos; i++) {
      //cerr<<"x,y,u,v:  "<<sx[i]<<"  "<<sy[i]<<"   "<<su[i]<<"  "<<sv[i]<<endl;
      if(su[i]!=fieldUndef && sv[i]!=fieldUndef) {
        sx[numTraj]= sx[i];
        sy[numTraj]= sy[i];
        numTraj++;
      }
    }
    //cerr<<"npos,numTraj: "<<npos<<" "<<numTraj<<endl;

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
  if (!gc.getMapFields(fu1->area, imapr, icori,
      fu1->nx, fu1->ny, &xmapr, &ymapr, &coriolis,
      dxgrid, dygrid)) {
    cerr<<"RadarEchoPlot::compute : gc.getMapFields ERROR."
    <<"  Cannot compute RadarEchoPlot !"<<endl;
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

  //........................................

  int ndata= nstep;
  if (firstStep) ndata++;

  RadarEchoData *radedata= new RadarEchoData();

  radedata->area=  fu1->area;
  radedata->ndata= ndata;
  radedata->time=  new miTime[ndata];
  radedata->first= new int[numTraj];
  radedata->last=  new int[numTraj];
  radedata->x=     new float[numTraj*ndata];
  radedata->y=     new float[numTraj*ndata];

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
      radedata->x[ndata*i+idata]= xt[i]= sx[i];
      radedata->y[ndata*i+idata]= yt[i]= sy[i];
    }
    delete[] sx;
    delete[] sy;
    sx= sy= 0;
    radedata->time[idata]= t1;
    idata+=incdata;
    runningForward=  new bool[numTraj];
    runningBackward= new bool[numTraj];
    for (int i=0; i<numTraj; i++)
      runningForward[i]= runningBackward[i]= true;

  } else {

    int n,nd,id;
    if (forward) {
      n=  vradedata.size() - 1;
      nd= vradedata[n]->ndata;
      id= nd - 1;
    } else {
      n=  0;
      nd= vradedata[n]->ndata;
      id= 0;
    }
    for (int i=0; i<numTraj; i++) {
      xt[i]= vradedata[n]->x[nd*i+id];
      yt[i]= vradedata[n]->y[nd*i+id];
    }
    if (vradedata[n]->area.P() != fieldArea.P()) {
      // posistions are converted to a different map projection
      if(!gc.getPoints(vradedata[n]->area.P(),fieldArea.P(),numTraj,xt,yt)) {
        cerr<<"RadarEchoPlot::compute : gc.getMapFields ERROR."
        <<"  RadarEchoPlot computation stopped !"<<endl;
        stopComputation();
        return false;
      }
    }

  }

  bool *running;
  int  *endnum;

  if (forward) {
    running= runningForward;
    endnum=  radedata->last;
  } else {
    running= runningBackward;
    endnum=  radedata->first;
  }

  for (int i=0; i<numTraj; i++) {
    if (running[i]) {
      radedata->first[i]= 0;
      radedata->last[i]=  ndata-1;
    } else {
      radedata->first[i]=  0;
      radedata->last[i]=  -1;
    }
  }

  float dt= tStep * 0.5;
  float u,v;

  for (int istep=0; istep<nstep; istep++) {
    //cerr<<"istep,nstep,tStep: "<<istep<<" "<<nstep<<" "<<tStep<<endl;

    float ct1b= float(istep)/float(nstep);
    float ct1a= 1.0 - ct1b;
    float ct2b= float(istep+1)/float(nstep);
    float ct2a= 1.0 - ct2b;

    // iteration no. 0 to get a first guess (then the real iterations)

    for (int iter=0; iter<=numIterations; iter++) {
      //cerr<<"   iter: "<<iter<<endl;

      int interpoltype= 1;
      fu1->interpolate(numTraj, xt, yt, u1, interpoltype);
      fv1->interpolate(numTraj, xt, yt, v1, interpoltype);
      fu2->interpolate(numTraj, xt, yt, u2, interpoltype);
      fv2->interpolate(numTraj, xt, yt, v2, interpoltype);
      frx->interpolate(numTraj, xt, yt, rx, interpoltype);
      fry->interpolate(numTraj, xt, yt, ry, interpoltype);

      //############# eller kutte ut posisjoner etterhvert med posIndex[] ???
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
          //	  if (running[i]) {
          u= ct1a * u1[i] + ct1b * u2[i];
          v= ct1a * v1[i] + ct1b * v2[i];
          xa[i]= xt[i] + rx[i] * u * dt;
          ya[i]= yt[i] + ry[i] * v * dt;
          //	  }
        }
      }

      for (int i=0; i<numTraj; i++) {
        //	if (running[i]) {
        u= ct2a * u1[i] + ct2b * u2[i];
        v= ct2a * v1[i] + ct2b * v2[i];
        xt[i]= xa[i] + rx[i] * u * dt;
        yt[i]= ya[i] + ry[i] * v * dt;
        //	}
      }

    }  // end of iteration loop

    for (int i=0; i<numTraj; i++) {
      if (running[i]) {
        radedata->x[ndata*i+idata]= xt[i];
        radedata->y[ndata*i+idata]= yt[i];
      } else {
        radedata->x[ndata*i+idata]= 0.; // a very legal position !!!
        radedata->y[ndata*i+idata]= 0.; // (cannot be checked later)
      }
    }

    miTime tnow= t1;
    if (forward)
      tnow.addSec(int(tStep*(istep+1) + 0.5));
    else
      tnow.addSec(int(tStep*(istep+1) - 0.5));

    radedata->time[idata]= tnow;

    idata+=incdata;

  }

  if (forward)
    vradedata.push_back(radedata);
  else
    vradedata.push_front(radedata);

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


#ifdef DEBUGPRINT
  cerr << "...finished" << endl;
#endif
  return true;
}


void RadarEchoPlot::getRadarEchoAnnotation(miString& s,
    Colour& c)
{
  //#### if (vradedata.size()>0) {
  if (plot_on && vradedata.size()>0) {
    int l= 16;
    if (firstTime.min()==0 && lastTime.min()==0) l= 13;
    s= "Trajektorier " + fieldStr
    + " "   + firstTime.isoTime().substr(0,l)
    + " - " +  lastTime.isoTime().substr(0,l) + " UTC";
    if (colour==backgroundColour)
      c= backContrastColour;
    else
      c= colour;
  } else {
    s= "";
  }

  // ADC temporary hack
  plotname= s;
}


bool RadarEchoPlot::printRadarEchoPositions(const miString& filename)
{

  //output
  ofstream fs;

  fs.open(filename.cStr());

  if(!fs){
    cerr << "ERROR  printTrajectoryPositions: can't open file: "
    <<filename << endl;
    return false;
  }


  int vtsize= vradedata.size();

  if(vtsize==0) return false;

  int npos = (vtsize + 1) * numTraj;
  float xxx[npos];
  float yyy[npos];
  //start points
  for (int i=0;i<numTraj;i++){
    xxx[i]=vradedata[0]->x[i*vradedata[0]->ndata];
    yyy[i]=vradedata[0]->y[i*vradedata[0]->ndata];
  }

  //next points
  for (int n=0; n<vtsize; n++) {
    for (int i=0;i<numTraj;i++){
      xxx[(n+1)*numTraj+i]=vradedata[n]->x[(i+1)*(vradedata[n]->ndata)-1];
      yyy[(n+1)*numTraj+i]=vradedata[n]->y[(i+1)*(vradedata[n]->ndata)-1];
    }
  }
  gc.xy2geo(area,npos,xxx,yyy);

  //print to file
  fs <<"[NAME TRAJECTORY]"<<endl;
  fs <<"[COLUMNS "<<endl;
  fs <<"Date:d   Time:t      Lon:r   Lat:r   No:r]"<<endl;
  fs <<"[DATA]"<<endl;
  for (int i=0;i<numTraj;i++){
    fs.setf(ios::showpoint);
    fs.precision(7);
    fs <<(*vradedata[0]->time).isoTime()<<"  ";
    fs <<setw(10) <<xxx[i]<<"  "<<
    setw(10) <<yyy[i]<<"  T"<<i<<endl;
  }

  for (int n=0; n<vtsize; n++) {
    for (int i=0;i<numTraj;i++){
      fs <<vradedata[n]->time[vradedata[n]->ndata-1].isoTime();
      fs <<"  "<<setw(10) <<xxx[(n+1)*numTraj+i]<<"  "<<
      setw(10) <<yyy[(n+1)*numTraj+i]<<"  T"<< i<<endl;
    }
  }

  fs.close();
  return true;


}
