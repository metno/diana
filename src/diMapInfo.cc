#include "diMapInfo.h"

void MapInfo::reset()
{
  name = "";
  mapfiles.clear();
  type = "pland";
  logok = true;
  special = false;
  symbol = 0;
  dbfcol = "";

  land.ison = false;
  land.fillcolour = "white";
  land.zorder = 0;

  contour.ison = true;
  contour.linecolour = "black";
  contour.linewidth = "1";
  contour.linetype = "solid";
  contour.zorder = 1;

  lon = contour; // copy linecolour, linewidth, linetype
  lon.ison = false;
  lon.zorder = 2;
  lon.density = 10.0;
  lon.showvalue = false;
  lon.value_pos = "bottom";
  lon.fontsize=10;

  lat = lon; // copy everything

  frame = contour; // copy linecolour, linewidth, linetype
  frame.ison = false;
  frame.zorder = 2;
}
