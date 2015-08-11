
#include "diMetConstants.h"

#include <cmath>

namespace MetNo {
namespace Constants {

float ewt_calculator::inverse(float et) const
{
  int ll = l;
  while (ll>0 and ll<N_EWT-1 and ewt[ll] > et)
    ll--;

  float r = (et - ewt[ll]) / (ewt[ll + 1] - ewt[ll]);
  return -100. + (float(ll) + r) * 5.;
}

static const double ICAO_g = 9.80665; // m/sec**2
static const double ICAO_R = 287.05287;

// static const double ICAO_T_0K = t0 + 15; // 15 deg C temperature at msl
// static const double ICAO_p_0 = 1013.15; // hPa pressure at msl

static const int ICAO_n = 8;
static const double ICAO_lambdas[ICAO_n-1] = { -6.5, 0, +1.0, +2.8, 0, -2.8, -2.0 }; // K/km -- temperature gradient for ICAO layers
static const double ICAO_base_h[ICAO_n] = { 0, 11, 20, 32, 47, 51, 71, 84.852 };  // km -- heights for ICAO layers

// def calculate_T_n(T_n1, h_n, h_n1, lambda_n1):
//     return T_n1 + (h_n - h_n1)*lambda_n1
// 
// def calculate_p_factor(h, h_n, l_n, T_n):
//     if l_n != 0:
//         return math.pow(1+(h-h_n)*l_n/T_n, -g/(l_n*R))
//     else:
//         return math.exp(-(h-h_n)*g/(R*T_n))
// 
// base_T = [T_0K]
// base_p = [p_0]
// 
// for i in range(1,8,1):
//     T_i = calculate_T_n(base_T[i-1], base_h[i], base_h[i-1], lambdas[i-1])
//     base_T.append(T_i)
// 
//     p_i = base_p[i-1]*calculate_p_factor(base_h[i], base_h[i-1], lambdas[i-1], base_T[i-1])
//     base_p.append(p_i)
//
// print "base_T[] =", base_T
// print "base_p[] =", base_p

// derived from ICAO_base_h and ICAO_lambdas
static const double ICAO_base_T[ICAO_n] = {
  288.15, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65, 186.946
};
static const double ICAO_base_p[ICAO_n] = {
  1013.15, 226.29806486313493, 54.743370958898005, 8.679301101236328,
  1.1089482781849516, 0.6693192180209551, 0.0395600169484907,
  0.0037334345211142398
};

double ICAO_geo_altitude_from_pressure(double pressure)
{
  int i = 1;
  while (i < ICAO_n && pressure < ICAO_base_p[i])
    i += 1;
  if (i >= ICAO_n)
    return 1000*(ICAO_base_h[ICAO_n-1] + 1); // beyond standard atmosphere

  const int l = i-1;
  const double lambda_l = ICAO_lambdas[l] / 1000,
      h_l = ICAO_base_h[l] * 1000,
      t_l = ICAO_base_T[l],
      p_l = ICAO_base_p[l];
  const double r_pressure = pressure/p_l;

  if (lambda_l != 0)
    return (t_l/lambda_l) * (std::pow(r_pressure, -(lambda_l*ICAO_R)/ICAO_g)-1) + h_l;
  else
    return h_l - std::log(r_pressure)*(ICAO_R*t_l)/ICAO_g;
}

double ICAO_pressure_from_geo_altitude(double altitude)
{
  const double h = altitude / 1000; // h is in km
  int i = 1;
  while (i < ICAO_n && h > ICAO_base_h[i])
    i += 1;
  if (i >= ICAO_n)
    return (ICAO_base_p[ICAO_n-1] - 1); // beyond standard atmosphere

  const int l = i-1;
  const double lambda_l = ICAO_lambdas[l] / 1000,
      altitude_l = ICAO_base_h[l] * 1000,
      t_l = ICAO_base_T[l],
      p_l = ICAO_base_p[l];
  const double d_altitude = altitude - altitude_l;

  double p_factor;
  if (lambda_l != 0)
    p_factor = std::pow(1 + d_altitude*lambda_l/t_l, -ICAO_g/(lambda_l*ICAO_R));
  else
    p_factor = std::exp(-d_altitude*ICAO_g/(ICAO_R*t_l));

  return p_l * p_factor;
}

int FL_from_geo_altitude(double a)
{
  return 5 * (int)round(a * ft_per_m / 500);
}

double geo_altitude_from_FL(double fl)
{
  return fl * 100 / ft_per_m;
}

} // namespace Constants
} // namespace MetNo
