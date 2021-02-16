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

#include "diVprofUtils.h"

#include "diVprofBoxFL.h"
#include "diVprofBoxLine.h"
#include "diVprofBoxPT.h"
#include "diVprofBoxSigWind.h"
#include "diVprofBoxVerticalWind.h"
#include "diVprofBoxWind.h"
#include "diVprofDiagram.h"
#include "diVprofOptions.h"
#include "diVprofPlotCommand.h"
#include "util/misc_util.h"

#include "diField/diPoint.h"

#include <boost/range/size.hpp>
#include <puTools/miStringFunctions.h>

using diutil::PointF;

#define MILOGGER_CATEGORY "diana.VprofDiagram"
#include <miLogger/miLogging.h>

namespace vprof {

// lengde i cm langs p=konst for 1 grad celsius
const float dx1deg = 0.418333;

const float chxbas = 0.5, chybas = 1.25 * chxbas; // character size in cm

const std::string VP_WIND_X = "vp_x_wind_ms";
const std::string VP_WIND_Y = "vp_y_wind_ms";
const std::string VP_WIND_DD = "vp_wind_dd";
const std::string VP_WIND_FF = "vp_wind_ff";
const std::string VP_WIND_SIG = "vp_wind_sig";
const std::string VP_RELATIVE_HUMIDITY = "vp_relative_humidity";
const std::string VP_OMEGA = "vp_omega_pas";
const std::string VP_AIR_TEMPERATURE = "vp_air_temperature_celsius";
const std::string VP_DEW_POINT_TEMPERATURE = "vp_dew_point_temperature_celsius";
const std::string VP_DUCTING_INDEX = "vp_ducting_index";
const std::string VP_CLOUDBASE = "vp_cloudbase";

const std::string VP_UNIT_COMPASS_DEGREES = "degree_N"; // actually meteorological degrees (0 == north, 90 = east, i.e. like a clock)

extern const std::vector<int> default_flightlevels = {0,   10,  20,  30,  40,  50,  60,  70,  80,  90,  100, 110, 120, 130, 140,
                                                      150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290,
                                                      300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 450, 500, 550, 600};

Colour alternateColour(const Colour& c)
{
  Colour c2("red");
  if (c2 == c)
    c2 = Colour("green");
  return c2;
}

TextSpacing::TextSpacing(float n, float l, float s)
    : next(n)
    , last(l)
    , spacing(s)
{
}

bool TextSpacing::accept(float v)
{
  if ((spacing > 0 && v > next && v < last) || (spacing < 0 && v < next && v > last)) {
    next = v + spacing;
    return true;
  } else {
    return false;
  }
}

void TextSpacing::flip()
{
  std::swap(next, last);
  spacing = -spacing;
}

PointF interpolateY(float y, const PointF& p1, const PointF& p2)
{
  const float r = (y - p1.y()) / (p2.y() - p1.y());
  const float x = p1.x() + (p2.x() - p1.x()) * r;
  return PointF(x, y);
}

PointF interpolateX(float x, const PointF& p1, const PointF& p2)
{
  const float r = (x - p1.x()) / (p2.x() - p1.x());
  const float y = p1.y() + (p2.y() - p1.y()) * r;
  return PointF(x, y);
}

PointF scaledTextSize(int length, float width)
{
  float chx = vprof::chxbas;
  float chy = vprof::chybas;
  if (chx * length > width) {
    chx = width / length;
    chy = chx * vprof::chybas / vprof::chxbas;
  }
  return PointF(chx, chy);
}

const std::string kv_linestyle_colour = ".colour";
const std::string kv_linestyle_linewidth = ".linewidth";
const std::string kv_linestyle_linetype = ".linetype";

bool kvLinestyle(Linestyle& ls, const std::string& prefix, const miutil::KeyValue& kv)
{
  if (kv.key() == prefix + kv_linestyle_colour)
    ls.colour = Colour(kv.value());
  else if (kv.key() == prefix + kv_linestyle_linewidth)
    ls.linewidth = kv.toFloat();
  else if (kv.key() == prefix + kv_linestyle_linetype)
    ls.linetype = Linetype(kv.value());
  else
    return false;
  return true;
}

namespace {

const std::string z_types[4] = {"amble", "exner", "linear", "log"};

VprofPlotCommand_p createBoxCommand(const std::string& type, const std::string& id)
{
  VprofPlotCommand_p cmd = std::make_shared<VprofPlotCommand>(VprofPlotCommand::BOX);
  cmd->add(VprofBox::key_type, type);
  cmd->add(VprofBox::key_id, id);
  cmd->add(miutil::kv(VprofBoxLine::key_x_grid, true));
  cmd->add(miutil::kv(VprofBoxLine::key_x_ticks_text, true));
  cmd->add(miutil::kv(VprofBoxLine::key_x_limits_in_corners, true));
  return cmd;
}

VprofPlotCommand_p createLineBoxCommand(const std::string& type, const std::string& id, float xmin, float xmax)
{
  VprofPlotCommand_p cmd = createBoxCommand(type, id);
  using miutil::kv;
  cmd->add(kv(VprofBoxLine::key_x_min, xmin));
  cmd->add(kv(VprofBoxLine::key_x_max, xmax));
  return cmd;
}

VprofPlotCommand_p createLineBoxCommand(const std::string& type, const std::string& id, float xmin, float xmax, const std::string& title)
{
  VprofPlotCommand_p cmd = createLineBoxCommand(type, id, xmin, xmax);
  cmd->add(miutil::kv(VprofBoxLine::key_title, title));
  return cmd;
}

VprofPlotCommand_p createLineGraphCommand(const std::string& id, const std::string& comp)
{
  VprofPlotCommand_p cmd = std::make_shared<VprofPlotCommand>(VprofPlotCommand::GRAPH);
  using miutil::kv;
  cmd->add(kv(VprofBoxLine::key_graph_type, "line"));
  cmd->add(kv(VprofBoxLine::key_graph_box, id));
  cmd->add(kv(VprofBoxLine::COMPONENTS, comp));
  return cmd;
}

} // namespace

PlotCommand_cpv createCommandsFromOptions(const VprofOptions* vpopt)
{
  VprofPlotCommand_cpv vpcmds = createVprofCommandsFromOptions(vpopt);
  PlotCommand_cpv cmds;
  cmds.reserve(vpcmds.size());
  std::copy(vpcmds.begin(), vpcmds.end(), std::back_inserter(cmds));
  return cmds;
}

VprofPlotCommand_cpv createVprofCommandsFromOptions(const VprofOptions* vpopt)
{
  using miutil::kv;

  VprofPlotCommand_cpv cmds, graph_cmds;

  VprofPlotCommand_p cmd_diagram = std::make_shared<VprofPlotCommand>(VprofPlotCommand::DIAGRAM);
  cmd_diagram->add(VprofDiagram::key_z_type, z_types[vpopt->diagramtype]);
  cmd_diagram->add(VprofDiagram::key_z_min, miutil::from_number(vpopt->pminDiagram));
  cmd_diagram->add(VprofDiagram::key_z_max, miutil::from_number(vpopt->pmaxDiagram));
  if (vpopt->pplinesfl)
    cmd_diagram->add(kv(VprofDiagram::key_z_unit, "FL"));
  cmd_diagram->add(kv(VprofDiagram::key_area_y1, -0.903486f));
  cmd_diagram->add(kv(VprofDiagram::key_area_y2, 27.8f));
  cmd_diagram->add(VprofDiagram::key_background + kv_linestyle_colour, vpopt->backgroundColour);
  cmd_diagram->add(kv(VprofDiagram::key_text, vpopt->ptext));
  cmd_diagram->add(kv(VprofDiagram::key_geotext, vpopt->pgeotext));
  cmd_diagram->add(kv(VprofDiagram::key_kindex, vpopt->pkindex));
  cmd_diagram->add(kv(VprofDiagram::key_frame, vpopt->pframe));
  cmd_diagram->add(kv(VprofDiagram::key_frame + kv_linestyle_colour, vpopt->frameColour));
  cmd_diagram->add(kv(VprofDiagram::key_frame + kv_linestyle_linewidth, vpopt->frameLinewidth));
  cmd_diagram->add(kv(VprofDiagram::key_frame + kv_linestyle_linetype, vpopt->frameLinetype));
  cmds.push_back(cmd_diagram);

  if (vpopt->pslwind) {
    cmds.push_back(createBoxCommand(VprofBoxSigWind::boxType(), "slwind"));
  }

  if (vpopt->pflevels) {
    VprofPlotCommand_p cmd = createBoxCommand(VprofBoxFL::boxType(), "flevels");
    cmd->add(kv(VprofBoxFL::key_style + kv_linestyle_colour, vpopt->flevelsColour));
    cmd->add(kv(VprofBoxFL::key_style + kv_linestyle_linewidth, vpopt->flevelsLinewidth1));
    cmd->add(kv(VprofBoxFL::key_text, vpopt->plabelflevels));
    cmd->add(kv(VprofBoxFL::key_levels, vpopt->flightlevels));
    cmds.push_back(cmd);
  }

  if (vpopt->ptttt || vpopt->ptdtd) {
    const std::string id = "pt";
    VprofPlotCommand_p cmd = createLineBoxCommand(VprofBoxPT::boxType(), id, vpopt->tminDiagram, vpopt->tmaxDiagram);
    cmd->add(kv(VprofBoxPT::key_frame, vpopt->pframe));
    cmd->add(kv(VprofBoxPT::key_width, (vpopt->tmaxDiagram - vpopt->tminDiagram) * dx1deg));

    cmd->add(kv(VprofBoxPT::key_z_grid, vpopt->ptlines));
    cmd->add(kv(VprofBoxPT::key_z_ticks_text, vpopt->plabelp));
    cmd->add(kv(VprofBoxPT::key_z_grid + kv_linestyle_colour, vpopt->pColour));
    cmd->add(kv(VprofBoxPT::key_z_grid + kv_linestyle_linewidth, vpopt->pLinewidth1));
    cmd->add(kv(VprofBoxPT::key_z_grid + kv_linestyle_linetype, vpopt->pLinetype));
    cmd->add(kv(VprofBoxPT::key_z_grid_linewidth2, vpopt->pLinewidth2));

    cmd->add(kv(VprofBoxPT::key_t_angle, vpopt->tangle));
    cmd->add(kv(VprofBoxPT::key_t_step, vpopt->tStep));
    cmd->add(kv(VprofBoxPT::key_x_grid, vpopt->ptlines));
    cmd->add(kv(VprofBoxPT::key_x_ticks_text, vpopt->plabelt));
    cmd->add(kv(VprofBoxPT::key_x_grid + kv_linestyle_colour, vpopt->tColour));
    cmd->add(kv(VprofBoxPT::key_x_grid + kv_linestyle_linewidth, vpopt->tLinewidth1));
    cmd->add(kv(VprofBoxPT::key_x_grid + kv_linestyle_linetype, vpopt->tLinetype));
    cmd->add(kv(VprofBoxPT::key_x_grid_linewidth2, vpopt->tLinewidth2));

    cmd->add(kv(VprofBoxPT::key_dryadiabat, vpopt->pdryadiabat));
    cmd->add(kv(VprofBoxPT::key_dryadiabat_step, vpopt->dryadiabatStep));
    cmd->add(kv(VprofBoxPT::key_dryadiabat + kv_linestyle_colour, vpopt->dryadiabatColour));
    cmd->add(kv(VprofBoxPT::key_dryadiabat + kv_linestyle_linetype, vpopt->dryadiabatLinetype));
    cmd->add(kv(VprofBoxPT::key_dryadiabat + kv_linestyle_linewidth, vpopt->dryadiabatLinewidth));

    cmd->add(kv(VprofBoxPT::key_wetadiabat, vpopt->pwetadiabat));
    cmd->add(kv(VprofBoxPT::key_wetadiabat_step, vpopt->wetadiabatStep));
    cmd->add(kv(VprofBoxPT::key_wetadiabat_pmin, vpopt->wetadiabatPmin));
    cmd->add(kv(VprofBoxPT::key_wetadiabat_tmin, vpopt->wetadiabatTmin));
    cmd->add(kv(VprofBoxPT::key_wetadiabat + kv_linestyle_colour, vpopt->wetadiabatColour));
    cmd->add(kv(VprofBoxPT::key_wetadiabat + kv_linestyle_linetype, vpopt->wetadiabatLinetype));
    cmd->add(kv(VprofBoxPT::key_wetadiabat + kv_linestyle_linewidth, vpopt->wetadiabatLinewidth));

    cmd->add(kv(VprofBoxPT::key_mixingratio, vpopt->pmixingratio));
    cmd->add(kv(VprofBoxPT::key_mixingratio_text, vpopt->plabelq));
    cmd->add(kv(VprofBoxPT::key_mixingratio_pmin, vpopt->mixingratioPmin));
    cmd->add(kv(VprofBoxPT::key_mixingratio_tmin, vpopt->mixingratioTmin));
    cmd->add(kv(VprofBoxPT::key_mixingratio + kv_linestyle_colour, vpopt->mixingratioColour));
    cmd->add(kv(VprofBoxPT::key_mixingratio + kv_linestyle_linetype, vpopt->mixingratioLinetype));
    cmd->add(kv(VprofBoxPT::key_mixingratio + kv_linestyle_linewidth, vpopt->mixingratioLinewidth));
    cmds.push_back(cmd);

    cmd->add(kv(VprofBoxPT::key_cotrails, vpopt->pcotrails));
    cmd->add(kv(VprofBoxPT::key_cotrails_pmin, vpopt->cotrailsPmin));
    cmd->add(kv(VprofBoxPT::key_cotrails_pmax, vpopt->cotrailsPmax));
    cmd->add(kv(VprofBoxPT::key_cotrails + kv_linestyle_colour, vpopt->cotrailsColour));
    cmd->add(kv(VprofBoxPT::key_cotrails + kv_linestyle_linetype, vpopt->cotrailsLinetype));
    cmd->add(kv(VprofBoxPT::key_cotrails + kv_linestyle_linewidth, vpopt->cotrailsLinewidth));

    if (vpopt->ptttt) {
      graph_cmds.push_back(createLineGraphCommand(id, vprof::VP_AIR_TEMPERATURE));
    }

    if (vpopt->ptdtd) {
      VprofPlotCommand_p cmd_td = createLineGraphCommand(id, vprof::VP_DEW_POINT_TEMPERATURE);
      cmd_td->add(kv(VprofBoxLine::key_graph_style + kv_linestyle_linetype, "dash"));
      graph_cmds.push_back(cmd_td);
    }
  }

  if (vpopt->pwind) {
    VprofPlotCommand_p cmd = createBoxCommand(VprofBoxWind::boxType(), "wind");
    cmd->add(kv(VprofBoxWind::key_separate, vpopt->windseparate));
    cmds.push_back(cmd);
  }

  if (vpopt->pvwind) {
    const std::string id = "vwind";
    const float range = vpopt->rvwind / 2;
    VprofPlotCommand_p cmd = createLineBoxCommand(VprofBoxVerticalWind::boxType(), id, +range, -range);
#if 0
    vwindColour = vprof::alternateColour(Colour(vpopt->frameColour));
    vwindLinewidth = vpopt->frameLinewidth;
#endif
    cmds.push_back(cmd);

    graph_cmds.push_back(createLineGraphCommand(id, vprof::VP_OMEGA));
  }

  if (vpopt->prelhum) {
    const std::string id = "relhum";
    VprofPlotCommand_p cmd = createLineBoxCommand(VprofBoxLine::boxType(), id, 0, 100, "RH");
    cmds.push_back(cmd);

    graph_cmds.push_back(createLineGraphCommand(id, vprof::VP_RELATIVE_HUMIDITY));
  }

  if (vpopt->pducting) {
    const std::string id = "ducting";
    VprofPlotCommand_p cmd = createLineBoxCommand(VprofBoxLine::boxType(), id, vpopt->ductingMin, vpopt->ductingMax, "DUCT");
    cmds.push_back(cmd);

    graph_cmds.push_back(createLineGraphCommand(id, vprof::VP_DUCTING_INDEX));
  }

  diutil::insert_all(cmds, graph_cmds);
  return cmds;
}

} // namespace vprof
