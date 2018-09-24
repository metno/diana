/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef VPROFBOXPT_H
#define VPROFBOXPT_H

#include "diVprofBoxLine.h"

class VprofBoxPT : public VprofBoxLine
{
public:
  VprofBoxPT();

  void setVerticalAxis(vcross::detail::AxisPtr zaxis) override;

  void setTAngle(float angle_deg);
  void setTStep(int step);
  void setDryAdiabat(bool on) { pdryadiabat = on; }
  void setDryAdiabatStep(int step);
  void setDryAdiabatStyle(const Linestyle& s) { dryadiabatLineStyle = s; }
  void setWetAdiabat(bool on) { pwetadiabat = on; }
  void setWetAdiabatStep(int step);
  void setWetAdiabatPMin(int pmin) { wetadiabatPmin = pmin; }
  void setWetAdiabatTMin(int tmin) { wetadiabatTmin = tmin; }
  void setWetAdiabatStyle(const Linestyle& s) { wetadiabatLineStyle = s; }
  void setMixingRatio(bool on) { pmixingratio = on; }
  void setMixingRatioTable(const std::vector<float>& table) { qtable = table; }
  void setMixingRatioPMin(int pmin) { mixingratioPmin = pmin; }
  void setMixingRatioTMin(int tmin) { mixingratioTmin = tmin; }
  void setMixingRatioStyle(const Linestyle& s) { mixingratioLineStyle = s; }
  void setCotrails(bool on) { pmixingratio = on; }
  void setCotrailsPMin(int pmin) { cotrailsPmin = pmin; }
  void setCotrailsPMax(int pmax) { cotrailsPmax = pmax; }
  void setCotrailsStyle(const Linestyle& s) { cotrailsLineStyle = s; }

  void updateLayout() override;
  void plotDiagram(VprofPainter* p) override;

  static const std::string& boxType();

  static const std::string key_t_angle;
  static const std::string key_t_step;

  static const std::string key_dryadiabat;
  static const std::string key_dryadiabat_step;

  static const std::string key_wetadiabat;
  static const std::string key_wetadiabat_step;
  static const std::string key_wetadiabat_pmin;
  static const std::string key_wetadiabat_tmin;

  static const std::string key_mixingratio;
  static const std::string key_mixingratio_text;
  static const std::string key_mixingratio_pmin;
  static const std::string key_mixingratio_tmin;
  static const std::string key_mixingratio_table;

  static const std::string key_cotrails;
  static const std::string key_cotrails_pmin;
  static const std::string key_cotrails_pmax;

protected:
  void configureDefaults() override;
  void configureOptions(const miutil::KeyValue_v& options) override;

  void configureXAxisLabelSpace() override;

  void plotXAxisGrid(VprofPainter* p) override;
  void plotXAxisLabels(VprofPainter* p) override;

private:
  void prepareDiagram();
  void condensationtrails();

  void plotDryAdiabats(VprofPainter* p);
  void plotWetAdiabats(VprofPainter* p);
  void plotMixingRatioLines(VprofPainter* p);
  void plotCondensationTrailLines(VprofPainter* p);

  bool useTiltedTemperatureLabels() const;

private:
  int pminDiagram;
  int pmaxDiagram;
  int tminDiagram;
  int tmaxDiagram;

  bool pplinesfl;
  std::vector<float> plevels;

  int tstep_;
  float tangle_;

  bool pdryadiabat;
  Linestyle dryadiabatLineStyle;
  int dryadiabatstep_; // temperature step (C at 1000hPa)

  bool pwetadiabat;
  int wetadiabatPmin;
  Linestyle wetadiabatLineStyle;
  int wetadiabatTmin;
  int wetadiabatstep_;

  bool pmixingratio;
  bool plabelq;
  Linestyle mixingratioLineStyle;
  int mixingratioPmin;
  int mixingratioTmin;
  std::vector<float> qtable; // units: g/kg

  bool pcotrails;
  Linestyle cotrailsLineStyle;
  int cotrailsPmin;
  int cotrailsPmax;

private:
  // vertical table resolution in hPa
  static const int idptab = 5;

  // length of vertical tables (0-1300 hPa)
  static const int mptab = 261;

  // cotrails : lines for evaluation of possibility for condensation trails
  //            (kondensstriper fra fly)
  //                 cotrails[0][k] : t(rw=0%)
  //                 cotrails[1][k] : t(ri=40%)
  //                 cotrails[2][k] : t(ri=100%)
  //                 cotrails[3][k] : t(rw=100%)
  static float cotrails[4][mptab];
  static bool init_cotrails;
};

typedef std::shared_ptr<VprofBoxPT> VprofBoxPT_p;

#endif // VPROFBOXPT_H
