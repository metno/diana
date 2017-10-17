// -*- c++ -*-
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

#ifndef DIFIELD_FIELDCALCULATIONS_H
#define DIFIELD_FIELDCALCULATIONS_H

#include "diFieldDefined.h"

#include <string>
#include <vector>

namespace FieldCalculations {

namespace calculations {

inline bool is_defined(bool allDefined, float in1, float undef)
{
  return allDefined or in1 != undef;
}

inline bool is_defined(bool allDefined, float in1, float in2, float undef)
{
  return allDefined or (in1 != undef and in2 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4, float in5, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4,
    float in5, float in6, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef and in6 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4,
    float in5, float in6, float in7, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef
      and in6 != undef and in7 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4,
    float in5, float in6, float in7, float in8, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef
      and in6 != undef and in7 != undef and in8 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4,
    float in5, float in6, float in7, float in8, float in9, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef
      and in6 != undef and in7 != undef and in8 != undef and in9 != undef);
}

inline bool is_defined(bool allDefined, float in1, float in2, float in3, float in4,
    float in5, float in6, float in7, float in8, float in9, float in10, float undef)
{
  return allDefined or (in1 != undef and in2 != undef and in3 != undef and in4 != undef and in5 != undef
      and in6 != undef and in7 != undef and in8 != undef and in9 != undef and in10 != undef);
}

void copy_field(float* fout, const float* fin, size_t fsize);

} // namespace calculations

//---------------------------------------------------
// pressure level (PLEVEL) functions
//---------------------------------------------------

bool pleveltemp(int compute, int nx, int ny, const float *tinp, float *tout,
    float p, difield::ValuesDefined& fDefined, float undef, const std::string& unit);

bool plevelthe(int compute, int nx, int ny, const float *t, const float *rh, float *the,
    float p, difield::ValuesDefined& fDefined, float undef);

bool plevelhum(int compute, int nx, int ny, const float *t, const float *huminp,
    float *humout, float p, difield::ValuesDefined& fDefined, float undef, const std::string& unit);

bool pleveldz2tmean(int compute, int nx, int ny, const float *z1, const float *z2,
    float *tmean, float p1, float p2, difield::ValuesDefined& fDefined, float undef);

bool plevelqvector(int compute, int nx, int ny, const float *z, const float *t,
    float *qcomp, const float *xmapr, const float *ymapr, const float *fcoriolis,
    float p, difield::ValuesDefined& fDefined, float undef);

bool plevelducting(int compute, int nx, int ny, const float *t, const float *h,
    float *duct, float p, difield::ValuesDefined& fDefined, float undef);

bool plevelgwind_xcomp(int nx, int ny, const float *z, float *ug,
    const float *xmapr, const float *ymapr, const float *fcoriolis, difield::ValuesDefined& fDefined,
    float undef);

bool plevelgwind_ycomp(int nx, int ny, const float *z, float *vg,
    const float *xmapr, const float *ymapr, const float *fcoriolis, difield::ValuesDefined& fDefined,
    float undef);

bool plevelgvort(int nx, int ny, const float *z, float *gvort, const float *xmapr,
    const float *ymapr, const float *fcoriolis, difield::ValuesDefined& fDefined, float undef);

bool kIndex(int compute, int nx, int ny, const float *t500, const float *t700,
    const float *rh700, const float *t850, const float *rh850, float *kfield, float p500,
    float p700, float p850, difield::ValuesDefined& fDefined, float undef);

bool ductingIndex(int compute, int nx, int ny, const float *t850, const float *rh850,
    float *duct, float p850, difield::ValuesDefined& fDefined, float undef);

bool showalterIndex(int compute, int nx, int ny, const float *t500, const float *t850,
      const float *rh850, float *sfield, float p500, float p850, difield::ValuesDefined& fDefined,
    float undef);

bool boydenIndex(int compute, int nx, int ny, const float *t700, const float *z700,
    const float *z1000, float *bfield, float p700, float p1000, difield::ValuesDefined& fDefined,
    float undef);

bool sweatIndex(int compute, int nx, int ny, const float *t850,const float *t500,
    const float *td850, const float *td500, const float *u850, const float *v850,
    const float *u500, const float *v500, float *sindex,
    difield::ValuesDefined& fDefined, float undef);

//---------------------------------------------------
// hybrid model level (HLEVEL) functions
//---------------------------------------------------

bool hleveltemp(int compute, int nx, int ny, const float *tinp, const float *ps,
    float *tout, float alevel, float blevel, difield::ValuesDefined& fDefined, float undef,
    const std::string& unit);

bool hlevelthe(int compute, int nx, int ny, const float *t, const float *q, const float *ps,
    float *the, float alevel, float blevel, difield::ValuesDefined& fDefined, float undef);

bool hlevelhum(int compute, int nx, int ny, const float *t, const float *huminp,
    const float *ps, float *humout, float alevel, float blevel, difield::ValuesDefined& fDefined,
    float undef, const std::string& unit);

bool hlevelducting(int compute, int nx, int ny, const float *t, const float *h,
    const float *ps, float *duct, float alevel, float blevel, difield::ValuesDefined& fDefined,
    float undef);

bool hlevelpressure(int nx, int ny, const float *ps, float *p, float alevel,
    float blevel, difield::ValuesDefined& fDefined, float undef);

//---------------------------------------------------
// atmospheric model level (ALEVEL) functions
//---------------------------------------------------

bool aleveltemp(int compute, int nx, int ny, const float *tinp, const float *p,
    float *tout, difield::ValuesDefined& fDefined, float undef, const std::string& unit);

bool alevelthe(int compute, int nx, int ny, const float *t, const float *q, const float *p,
    float *the, difield::ValuesDefined& fDefined, float undef);

bool alevelhum(int compute, int nx, int ny, const float *t, const float *huminp,
    const float *p, float *humout, difield::ValuesDefined& fDefined, float undef, const std::string& unit);

bool alevelducting(int compute, int nx, int ny, const float *t, const float *h, const float *p,
    float *duct, difield::ValuesDefined& fDefined, float undef);

//---------------------------------------------------
// isentropic level (ILEVEL) function
//---------------------------------------------------

bool ilevelgwind(int nx, int ny, const float *mpot, float *ug, float *vg,
    const float *xmapr, const float *ymapr, const float *fcoriolis, difield::ValuesDefined& fDefined,
    float undef);

//---------------------------------------------------
// ocean depth level (OZLEVEL) functions
//---------------------------------------------------

bool seaSoundSpeed(int compute, int nx, int ny, const float *t, const float *s,
    float *soundspeed, float z, difield::ValuesDefined& fDefined, float undef);

//---------------------------------------------------
// level (pressure) independant functions
//---------------------------------------------------

bool cvtemp(int compute, int nx, int ny, const float *tinp, float *tout,
    difield::ValuesDefined& fDefined, float undef);

bool cvhum(int compute, int nx, int ny, const float *t, const float *huminp,
    float *humout, difield::ValuesDefined& fDefined, float undef, const std::string& unit);

bool vectorabs(int nx, int ny, const float *u, const float *v, float *ff,
    difield::ValuesDefined& fDefined, float undef);

bool relvort(int nx, int ny, const float *u, const float *v, float *rvort, const float *xmapr,
    const float *ymapr, difield::ValuesDefined& fDefined, float undef);

bool absvort(int nx, int ny, const float *u, const float *v, float *avort, const float *xmapr,
    const float *ymapr, const float *fcoriolis, difield::ValuesDefined& fDefined, float undef);

bool divergence(int nx, int ny, const float *u, const float *v, float *diverg,
    const float *xmapr, const float *ymapr, difield::ValuesDefined& fDefined, float undef);

bool advection(int nx, int ny, const float *f, const float *u, const float *v, float *advec,
    const float *xmapr, const float *ymapr, float hours, difield::ValuesDefined& fDefined, float undef);

bool gradient(int compute, int nx, int ny, const float *field, float *fgrad,
    const float *xmapr, const float *ymapr, difield::ValuesDefined& fDefined, float undef);

bool shapiro2_filter(int nx, int ny, float *field, float *fsmooth,
    difield::ValuesDefined& fDefined, float undef);

bool windCooling(int compute, int nx, int ny, const float *t, const float *u, const float *v,
    float *dtcool, difield::ValuesDefined& fDefined, float undef);

bool underCooledRain(int nx, int ny, const float *precip, const float *snow, const float *tk,
    float *undercooled, float precipMin, float snowRateMax, float tcMax,
    difield::ValuesDefined& fDefined, float undef);

bool thermalFrontParameter(int nx, int ny, const float *t, float *tfp,
    const float *xmapr, const float *ymapr, difield::ValuesDefined& fDefined, float undef);

bool pressure2FlightLevel(int nx, int ny, const float *pressure,
    float *flightlevel, difield::ValuesDefined& fDefined, float undef);

bool momentumXcoordinate(int nx, int ny, const float *v, float *mxy, const float *xmapr,
    const float *fcoriolis, float fcoriolisMin, difield::ValuesDefined& fDefined, float undef);

bool momentumYcoordinate(int nx, int ny, const float *u, float *nxy, const float *ymapr,
    const float *fcoriolis, float fcoriolisMin, difield::ValuesDefined& fDefined, float undef);

bool jacobian(int nx, int ny, const float *field1, const float *field2, float *fjacobian,
    const float *xmapr, const float *ymapr, difield::ValuesDefined& fDefined, float undef);

//obsolete - will be replaced by vesselIcingOverland2
bool vesselIcingOverland(int nx, int ny, const float *airtemp, const float *seatemp, const float *u,
    const float *v, float *icing, float freezingpoint, difield::ValuesDefined& fDefined, float undef);

//obsolete - will be replaced by vesselIcingMertins2
bool vesselIcingMertins(int nx, int ny, const float *airtemp, const float *seatemp, const float *u,
    const float *v, float *icing, float freezingpoint, difield::ValuesDefined& fDefined, float undef);

bool vesselIcingOverland2(int nx, int ny, const float *airtemp, const float *seatemp, const float *u,
    const float *v, const float *sal, const float *aice, float *icing, difield::ValuesDefined& fDefined,
    float undef);

bool vesselIcingMertins2(int nx, int ny, const float *airtemp, const float *seatemp, const float *u,
    const float *v, const float *sal, const float *aice, float *icing, difield::ValuesDefined& fDefined,
    float undef);

bool vesselIcingModStall(int nx, int ny, const float *sal, const float *wave, const float *x_wind,
    const float *y_wind, const float *airtemp, const float *rh, const float *sst, const float *p, const float *Pw,
    const float *aice, const float *depth, float *icing,
    const float vs, const float alpha, const float zmin, const float zmax, difield::ValuesDefined& fDefined, float undef);

bool vesselIcingTestMod(int nx, int ny, const float *sal, const float *wave, const float *x_wind,
    const float *y_wind, const float *airtemp, const float *rh, const float *sst, const float *p, const float *Pw,
    const float *aice, const float *depth, float *icing,
    const float vs, const float alpha, const float zmin, const float zmax, difield::ValuesDefined& fDefined, float undef);

bool values2classes(int nx, int ny, const float *fvalue, float *fclass,
    const std::vector<float>& values, difield::ValuesDefined& fDefined, float undef);

bool fieldOPERfield(int compute, int nx, int ny, const float *field1,
    const float *field2, float *fres, difield::ValuesDefined& fDefined, float undef);

bool fieldOPERconstant(int compute, int nx, int ny, const float *field,
    float constant, float *fres, difield::ValuesDefined& fDefined, float undef);

bool constantOPERfield(int compute, int nx, int ny, float constant,
    const float *field, float *fres, difield::ValuesDefined& fDefined, float undef);

bool sumFields(int nx, int ny, const std::vector<float*>& fields,
    float *fres, difield::ValuesDefined& fDefined, float undef);

void fillEdges(int nx, int ny, float *field);

bool meanValue(int nx, int ny, const std::vector<float*>& fields, const std::vector<difield::ValuesDefined> &fDefinedIn,
    float *fres, difield::ValuesDefined& fDefinedOut, float undef);

bool stddevValue(int nx, int ny, const std::vector<float*>& fields, const std::vector<difield::ValuesDefined> &fDefinedIn,
    float *fres, difield::ValuesDefined& fDefinedOut, float undef);

bool extremeValue(int compute, int nx, int ny, const std::vector<float*>& fields,
    float *fres, difield::ValuesDefined& fDefined, float undef);

bool probability(int compute, int nx, int ny, const std::vector<float*>& fields, const std::vector<difield::ValuesDefined> &fDefinedIn,
    const std::vector<float>& limits,
    float *fres, difield::ValuesDefined& fDefinedOut, float undef);

bool neighbourFunctions(int nx, int ny, const float* field, const std::vector<float>& constants,
    int compute, float *fres, difield::ValuesDefined& fDefined, float undef);

bool snow_in_cm(int nx, int ny, const float *snow_water, const float *tk2m, const float *td2m,
    float *snow_cm, difield::ValuesDefined& fDefined, float undef);

} // namespace FieldCalculations

#endif // DIFIELD_FIELDCALCULATIONS_H
