/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "diVcrossFile.h"

#include "diFtnVfile.h"
#include "diLocationPlot.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>

#include <boost/range/adaptor/map.hpp>
#include <boost/foreach.hpp>

#include <cmath>
#include <set>
#include <vector>

#define MILOGGER_CATEGORY "diana.VcrossFile"
#include <miLogger/miLogging.h>

using namespace std;

namespace /* anonymous */ {

const char PSURF_NAME[] = "8";

} // anonymous namespace

// ########################################################################

VcrossFile::VcrossFile(const std::string& filename, const std::string& modelname)
  : VcrossSource(modelname), fileName(filename), vfile(0),
    modificationtime(0),
    numCross(0), numTime(0), numLev(0), numPar2d(0), numPar1d(0),
    nposmap(0), xposmap(0), yposmap(0), dataAddress(0)
{
  METLIBS_LOG_SCOPE();
}


VcrossFile::~VcrossFile()
{
  METLIBS_LOG_SCOPE();
  cleanup();
}


void VcrossFile::cleanup()
{
  METLIBS_LOG_SCOPE(LOGVAL(fileName));

  delete[] xposmap;
  delete[] yposmap;
  delete[] dataAddress;
  delete vfile;
  xposmap= 0;
  yposmap= 0;
  dataAddress= 0;
  vfile= 0;
  modificationtime= 0;
}


bool VcrossFile::update()
{
  METLIBS_LOG_SCOPE(LOGVAL(fileName));

  bool ok= true;

  pu_struct_stat statbuf;

  if (pu_stat(fileName.c_str(),&statbuf)==0) {
    if (modificationtime != statbuf.st_ctime) {
      cleanup();
      if (readFileHeader())
        modificationtime= statbuf.st_ctime;
      else
        ok= false;
    }
  } else {
    ok= false;
  }

  if (!ok)
    cleanup();

  return ok;
}


bool VcrossFile::readFileHeader()
{
  METLIBS_LOG_SCOPE(LOGVAL(fileName));

  // reading and storing information, not data

  int bufferlength= 1024;

  vfile= new FtnVfile(fileName, bufferlength);

  int n, i, length, iscale, iundef;
  int *head, *vcinfo;
  int *tmp;

  iundef= 0;

  numCross= numTime= numLev= numPar2d= numPar1d= 0;
  vcoord= 0;

  try {

    vfile->init();
    length= 4;
    head= vfile->getInt(length);
    if (head[0]!=121 || head[1]>2 || head[2]!=bufferlength*2)
      throw VfileError();
    length= head[3];
    if (length<25)
      throw VfileError();
    vcinfo= vfile->getInt(length);

    int lctext,npmap,igtype;
    //int igtype,igsize[4],mgtype,mgsize[4],numlev1;
    float gridparam[6];

    vcoord=   vcinfo[ 0];
    //mpoint=   vcinfo[ 1];
    numCross= vcinfo[ 2];
    numLev=   vcinfo[ 3];
    numPar2d= vcinfo[ 4];
    //numlev1=  vcinfo[ 5];
    numPar1d= vcinfo[ 6];
    //ntimud= vcinfo[ 7];
    numTime=  vcinfo[ 8];
    nlvlid= vcinfo[ 9];
    //nrprod= vcinfo[10];
    //nrgrid= vcinfo[11];
    lctext= vcinfo[12];
    //mlname= vcinfo[13];
    npmap  =vcinfo[14];
    // data grid
    igtype   =vcinfo[15];
    //igsize[1]=vcinfo[16];
    //igsize[2]=vcinfo[17];
    //igsize[3]=vcinfo[18];
    //igsize[4]=vcinfo[19];
    // map grid (used if no other data or map spec. set)
    //mgtype   =vcinfo[20];
    //mgsize[1]=vcinfo[21];
    //mgsize[2]=vcinfo[22];
    //mgsize[3]=vcinfo[23];
    //mgsize[4]=vcinfo[24];

    delete[] vcinfo;

    // Vertical coordinates handled:
    // vcoord:  2 = sigma (0.-1. with ps and ptop) ... Norlam
    //         10 = eta (hybrid) ... Hirlam, Ecmwf,...
    //          1 = pressure
    //          4 = isentropic surfaces (potential temp., with p(th))
    //          5 = z levels from sea model (possibly incl. sea elevation and bottom)
    //         11 = sigma height levels (MEMO,MC2)
    //         12 = sigma.MM5 (input P in all levels)

    if (vcoord!=2 && vcoord!=10 && vcoord!=1 && vcoord!=4 &&
        vcoord!=5 && vcoord!=11 && vcoord!=12) {
      METLIBS_LOG_WARN("No or bad data in file. Vertical coordinate " << vcoord);
      throw VfileError();
    }

    // model name or some text
    modelName2= vfile->getString(lctext);

    // data grid parameters
    tmp= vfile->getInt(6);  // scaling of each element
    for (n=0; n<6; n++)
      gridparam[n]= vfile->getFloat(tmp[n],iundef);
    delete[] tmp;

    // map grid parameters ... NOT USED
    //tmp= vfile->getInt(6);  // scaling of each element
    //for (n=0; n<6; n++)
    //  mapgridparam[n]= getFloat(tmp[n],iundef);
    //delete[] tmp;
    vfile->skipData(6+6);

    // vertical range
    iscale= vfile->getInt();
    float *vrange= vfile->getFloat(2*numCross,iscale,iundef);
    // set vertical range equal for all crossections
    float vmin= vrange[0];
    float vmax= vrange[1];
    for (n=2; n<2*numCross; n+=2) {
      if (vmin>vrange[n])   vmin=vrange[n];
      if (vmax<vrange[n+1]) vmax=vrange[n+1];
    }
    for (n=0; n<numCross; n++) {
      vrangemin.push_back(vmin);
      vrangemax.push_back(vmax);
    }
    delete[] vrange;

    // parameter no. (multilevel and single level)
    identPar2d= vfile->getIntVector(numPar2d);
    identPar1d= vfile->getIntVector(numPar1d);
    //###################################################################
    //    cerr<<" numPar2d= "<<numPar2d<<endl;
    //    for (n=0; n<numPar2d; n++)
    //      cerr<<"    "<<n<<" : "<<identPar2d[n]<<endl;
    //    cerr<<" numPar1d= "<<numPar1d<<endl;
    //    for (n=0; n<numPar1d; n++)
    //      cerr<<"    "<<n<<" : "<<identPar1d[n]<<endl;
    //###################################################################

    // always need x,y positions in model grid
    nxgPar= nygPar=  nxsPar= nxdsPar= -1;
    for (n=0; n<numPar1d; n++) {
      if (identPar1d[n]==-1001) nxgPar= n;
      if (identPar1d[n]==-1002) nygPar= n;
      if (identPar1d[n]==-1003) nxsPar= n;
      if (identPar1d[n]==-1007) nxdsPar= n;
    }
    if (nxgPar<0 || nygPar<0) {
      METLIBS_LOG_WARN("No horizontal grid positions found!");
      throw VfileError();
    }

    // no. of positions in each crossection
    numPoint=  vfile->getIntVector(numCross);

    //###################################################################
    //cerr << "VcrossFile::readFileHeader fileName= " << fileName << endl;
    //cerr << "      sizes: "<<names.size()<<" "<<validTime.size()<<endl;
    //###################################################################
    std::vector<std::string> names;
    validTime.clear();
    forecastHour.clear();
    delete[] xposmap;
    delete[] yposmap;
    xposmap= 0;
    yposmap= 0;
    nposmap= 0;

    // crossection names and possibly annotation optons behind "!!"
    tmp= vfile->getInt(numCross);
    size_t pn;
    for (n=0; n<numCross; n++) {
      std::string str= vfile->getString(tmp[n]);
      std::string name,opts;
      if((pn=str.find("!!"))!=string::npos) {
        name=str.substr(0,pn-1);
        opts=str.substr(pn+2);
        miutil::trim(name);
        miutil::trim(opts);
      } else {
        name=str;
        miutil::trim(name);
      }
      //###################################################################
      //    cerr<<"crossection "<<n<<" : "<<name<<endl;
      //    cerr<<"       opts "<<n<<" : "<<opts<<endl;
      //###################################################################
      // had many names without the last ')' making serious PostScript errors
      const std::string misName(name);
      int nc1= miutil::count_char(misName, '(');
      int nc2= miutil::count_char(misName, ')');
      if (nc1!=nc2) {
        while (nc1<nc2) {
          name= '(' + name;
          nc1++;
        }
        while (nc2<nc1) {
          name= name + ')';
          nc2++;
        }
      }

      names.push_back(name);
      posOptions.push_back(opts);
    }
    delete[] tmp;

    // time
    tmp= vfile->getInt(numTime*5);
    i=0;
    for (n=0; n<numTime; n++) {
      int year  = tmp[i++];
      int month = tmp[i++];
      int day   = tmp[i++];
      int hour  = tmp[i++];
      int fchour= tmp[i++];
      miutil::miTime t= miutil::miTime(year,month,day,hour,0,0);
      if (fchour!=0) t.addHour(fchour);
      //###################################################################
      //      cerr<<"time "<<n<<" : "<<t<<endl;
      //###################################################################
      validTime.push_back(t);
      forecastHour.push_back(fchour);
    }
    delete[] tmp;

    // these map positions not used here
    // (diana requires more flexibility)
    // no. of map positions in each crossection
    //int *npmapcr= vfile->getInt(numCross);
    // map positions
    //iscale= vfile->getInt();
    //float *xposmap= vfile->getFloat(npmap,iscale,iundef);
    //float *yposmap= vfile->getFloat(npmap,iscale,iundef);
    vfile->skipData(numCross+1+npmap*2);

    // pointer to data for each crossection and time
    // (pointer as fortran record and word no.)
    if (head[1] == 1) {
      dataAddress= vfile->getInt(2*numCross*numTime);
    } else {
      dataAddress= vfile->getIntDuo(2*numCross*numTime);
    }
    // get map positions (in data grid units)....................
    // (some nasty stuff until program vcdata changed)
    nposmap= 0;
    for (n=0; n<numCross; n++)
      nposmap+=numPoint[n];
    xposmap= new float[nposmap];
    yposmap= new float[nposmap];
    int np= 0;
    int itime= 0;
    for (n=0; n<numCross; n++) {
      // set start position in file ("fortran" record and word)
      int record= dataAddress[itime*numCross*2+n*2];
      int word=   dataAddress[itime*numCross*2+n*2+1];
      // skip to single level data for first timestep
      int nskip= nlvlid*(1+numLev) + 2 + numPar2d*numLev
      + numPoint[n]*numLev*numPar2d;
      record+= (word+nskip)/bufferlength;  // ??????????????????????
      word=    (word+nskip)%bufferlength;  // ??????????????????????
      //record+= (word+nskip-1)/bufferlength;      // ??????????????????????
      //word=    (word+nskip-1)%bufferlength + 1;  // ??????????????????????
      vfile->setFilePosition(record,word);
      tmp= vfile->getInt(numPar1d);  // scaling of each element
      int iundef1d= 0;
      nskip= numPoint[n]*nxgPar;
      if (nskip>0) vfile->skipData(nskip);
      vfile->getFloat(&xposmap[np],numPoint[n],tmp[nxgPar],iundef1d);
      vfile->getFloat(&yposmap[np],numPoint[n],tmp[nygPar],iundef1d);
      //###################################################################
//            cerr<<"--------------------"<<endl;
//            cerr<<"  nxgPar,nygPar: "<<nxgPar<<" "<<nygPar<<endl;
//            cerr<<"  xscale,yscale: "<<tmp[nxgPar]<<" "<<tmp[nygPar]<<endl;
//            for (i=0; i<numPoint[n]; i++)
//      	cerr<<"  x,y: "<<xposmap[np+i]<<" "<<yposmap[np+i]<<endl;
      //###################################################################
      delete[] tmp;
      np+=numPoint[n];
    }
    // x,y from fortran...
    Projection p;
    double gridResolutionX;
    double gridResolutionY;
    p.set_mi_gridspec(igtype,gridparam, gridResolutionX, gridResolutionY);
    Rectangle  r;
    Area area(p,r);
    for (n=0; n<nposmap; n++) {
      xposmap[n]-=1.0f;
      xposmap[n]*=gridResolutionX;
      yposmap[n]-=1.0f;
      yposmap[n]*=gridResolutionY;
    }
    p.convertToGeographic(nposmap,xposmap,yposmap);
    //###################################################################
//        cerr<<"--------------------"<<endl;
//        np= 0;
//        for (n=0; n<numCross; n++) {
//          cerr<<"crossection "<<n<<" : "<<names[n]<<endl;
//          cerr<<"  lon,lat: "<<xposmap[np+0]<<" "<<yposmap[np+0]<<endl;
//          cerr<<"  lon,lat: "<<xposmap[np+numPoint[n]-1]<<" "<<yposmap[np+numPoint[n]-1]<<endl;
//          np+=numPoint[n];
//        }
    //###################################################################
    //...........................................................

    mCrossections = crossections_t(numCross);
    for (int n=0, np=0; n<numCross; n++) {
      VcrossData::Cut& cs = mCrossections[n];
      cs.name = names[n];
      const size_t npoints = numPoint[n];
      for (size_t j=0; j<npoints; ++j) {
        cs.lonlat.push_back(LonLat::fromDegrees(xposmap[np], yposmap[np]));
        np += 1;
      }
    }
  }  // end of try

  catch (...) {
    METLIBS_LOG_WARN("Bad Vcross file: " << fileName);
    delete vfile;
    vfile= 0;
    return false;
  }

  //############################################################################
  //  for (n=0; n<identPar1d.size(); n++)
  //    cerr<<"VcrossFile::readFileHeader n,identPar1d[n]: "<<n<<" "<<identPar1d[n]<<endl;
  //  for (n=0; n<identPar2d.size(); n++)
  //    cerr<<"VcrossFile::readFileHeader n,identPar2d[n]: "<<n<<" "<<identPar2d[n]<<endl;
  //############################################################################

  mParameterIds.clear();
  BOOST_FOREACH(int p2d, identPar2d) {
    mParameterIds.push_back(miutil::from_number(p2d));
  }

  return true;
}

VcrossData* VcrossFile::getCrossData(const std::string& csname, const std::set<std::string>& parameters, const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();

  if (!vfile)
    return 0;

  int iCross=0;
  while (iCross<numCross && mCrossections[iCross].name != csname)
    iCross++;
  if (iCross==numCross)
    return 0;

  int iTime= 0;
  while (iTime<numTime && validTime[iTime]!=time)
    iTime++;
  if (iTime==numTime)
      return 0;

  int nPoint= numPoint[iCross];
  if(nPoint==0) {
    METLIBS_LOG_WARN("The crossection '" << csname << "' contains no positions");
    return 0;
  }

  std::auto_ptr<VcrossData> data(new VcrossData());

  VcrossData::ZAxisPtr zax(new VcrossData::ZAxis());
  zax->name = "file z axis";
  zax->quantity = VcrossData::ZAxis::PRESSURE;
  zax->mPoints = nPoint;
  zax->mLevels = numLev;
  data->zAxes.push_back(zax);
  std::vector<float> alevel, blevel;
  
  // for TimeGraph data (when tgpos>=0)
  std::vector<float*> tgdata1d;
  std::vector<float*> tgdata2d;

  if (iTime<0)
    iTime= 0;
  if (iTime>=numTime)
    iTime= numTime-1;
  const int itime1= iTime, itime2= iTime;

  int iscale, iundef, iundef1d, iundef2d, ns;
  int *tmp;

  try {

    for (int itime=itime1; itime<=itime2; itime++) {

      //################################################################
      //if (tgpos>=0) cerr<<"read tgpos,itime: "<<tgpos<<" "<<itime<<endl;
      //################################################################

      // set start position in file ("fortran" record and word)
      int record= dataAddress[itime*numCross*2+iCross*2];
      int word=   dataAddress[itime*numCross*2+iCross*2+1];
      if (not vfile->setFilePosition(record, word))
        return 0;
      iundef= 0;

      // level values
      for (int n=0; n<nlvlid; n++) {
        iscale= vfile->getInt();
        if (n==0) {
          alevel= vfile->getFloatVector(numLev,iscale,iundef);
        } else if (n==1) {
          blevel= vfile->getFloatVector(numLev,iscale,iundef);
        } else {
          vfile->skipData(numLev);
        }
      }

      // existence of undefined values in the data
      iundef2d= vfile->getInt();
      iundef1d= vfile->getInt();

      // multilevel data
      tmp= vfile->getInt(numPar2d*numLev); // scaling, each param and level!
      ns= 0;
      for (int n=0; n<numPar2d; n++) {
        VcrossData::ParameterData pd;
        pd.mPoints = nPoint;
        pd.mLevels = numLev;
        pd.alloc();
        pd.zAxis = zax;

        for (int l=0; l<numLev; l++)
          vfile->getFloat(&(pd.values.get())[l*nPoint],nPoint,tmp[ns++],iundef2d);

        data->parameters.insert(std::make_pair(miutil::from_number(identPar2d[n]), pd));
      }
      delete[] tmp;

      // single level data (surface etc.)
      tmp= vfile->getInt(numPar1d); // scaling, each param
      for (int n=0; n<numPar1d; n++) {

        VcrossData::ParameterData pd;
        pd.mPoints = nPoint;
        pd.mLevels = 1;
        pd.values = boost::shared_array<float>(vfile->getFloat(nPoint,tmp[n],iundef1d));
        data->parameters.insert(std::make_pair(miutil::from_number(identPar1d[n]), pd));
      }
      delete[] tmp;
    }

  } catch (...) {
    METLIBS_LOG_WARN("Bad Vcross file: " << fileName);
    delete vfile;
    vfile= 0;
    return 0;
  }

  zax->alloc();
  if (vcoord == 10) { // 10 = eta (hybrid) ... Hirlam, Ecmwf,...
    const VcrossData::Parameters_t::const_iterator psurf_it = data->parameters.find(PSURF_NAME);
    for (int l=0; l<numLev; ++l) {
      for (int p=0; p<nPoint; ++p) {
        const float psurf = (psurf_it != data->parameters.end()) ? psurf_it->second.value(0, p) : 1013.5f;
        const float v = alevel[l] + blevel[l] * psurf;
        zax->setValue(l, p, v);
      }
    }
  } else if (vcoord == 1) { // 1 = pressure
    for (int l=0; l<numLev; ++l) {
      for (int p=0; p<nPoint; ++p) {
        zax->setValue(l, p, alevel[l]);
      }
    }
  }

  return data.release();
}

VcrossData* VcrossFile::getTimeData(const std::string& csname, const std::set<std::string>& parameters, int csPositionIndex)
{
  if (!vfile)
    return 0;

  int iCross=0;
  while (iCross<numCross && mCrossections[iCross].name!=csname)
    iCross++;
  if (iCross==numCross)
    return 0;

  return 0;
}
