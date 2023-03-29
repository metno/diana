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

#include "diana_config.h"

#include "qtMapDialog.h"
#include "qtUtility.h"
#include "qtToggleButton.h"
#include "diKVListPlotCommand.h"
#include "diLinetype.h"
#include "diMapManager.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.MapDialog"
#include <miLogger/miLogging.h>

#include "ui_mapdialog.h"


#define HEIGHTLB 105
#define HEIGHTLBSMALL 70
#define HEIGHTLBXSMALL 50

static int density_idx(const std::vector<int>& densities, float mi_density)
{
  const int dens = int(mi_density*1000);
  for (size_t k=0; k<densities.size(); k++) {
    if (densities[k] == dens) {
      return k;
    }
  }
  return 0;
}

static void install_density(QComboBox* box, const std::vector<int>& densities, int defItem)
{
  for (int d : densities)
    box->addItem(QString::number(d/1000.0));
  box->setCurrentIndex(defItem);
}

/*********************************************/
MapDialog::MapDialog(QWidget* parent, Controller* llctrl)
  : QWidget(parent)
{
  METLIBS_LOG_SCOPE();
  m_ctrl = llctrl;

  ui = new Ui_MapDialog();
  ui->setupUi(this);
  ui->areabox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  ui->ff_linestyle->setWhat(tr("frame"));
  ui->lon_linestyle->setWhat(tr("longitude"));
  ui->lat_linestyle->setWhat(tr("latitude"));
  ui->cont_linestyle->setWhat(tr("contour"));

  connect(ui->mapapply, SIGNAL(clicked()), SIGNAL(MapApply()));
  connect(ui->maphide, SIGNAL(clicked()), SIGNAL(MapHide()));

  // all defined maps etc.
  MapManager mapm;
  m_MapDI = mapm.getMapDialogInfo();

  numMaps = m_MapDI.maps.size();
  activemap = -1;

  // zorders
  std::vector<std::string> zorders; // all defined zorders
  zorders.push_back(tr("lowest").toStdString());
  zorders.push_back(tr("auto").toStdString());
  zorders.push_back(tr("highest").toStdString());

  // latlon densities (degrees)
  densities.push_back( 500);
  densities.push_back(1000);
  densities.push_back(2000);
  densities.push_back(2500);
  densities.push_back(3000);
  densities.push_back(4000);
  densities.push_back(5000);
  densities.push_back(10000);
  densities.push_back(15000);
  densities.push_back(30000);
  densities.push_back(90000);
  densities.push_back(180000);

  // positions
  std::vector<std::string> positions_tr; // all defined positions
  positions_tr.push_back(tr("off").toStdString());
  positions_tr.push_back(tr("left").toStdString());
  positions_tr.push_back(tr("bottom").toStdString());
  positions_tr.push_back(tr("both").toStdString());
  positions.push_back("left");
  positions.push_back("bottom");
  positions.push_back("both");
  positions_map["left"]=0;;
  positions_map["bottom"]=1;;
  positions_map["both"]=2;;
//obsolete syntax
  positions_map["0"]=0;;
  positions_map["1"]=1;;
  positions_map["2"]=2;;

  // ==================================================

  int nr_area = m_MapDI.areas.size();
  int area_defIndex = 0;
  for (int i = 0; i < nr_area; i++) {
    ui->areabox->addItem(QString(m_MapDI.areas[i].c_str()));
    if (m_MapDI.areas[i] == m_MapDI.default_area)
      area_defIndex = i;
  }
  // select default area
  ui->areabox->setCurrentIndex(area_defIndex);

  // ==================================================

  // add maps, selecting default maps
  const std::set<std::string> defaultmaps(m_MapDI.default_maps.begin(), m_MapDI.default_maps.end());
  for (const MapInfo& mi : m_MapDI.maps) {
    QListWidgetItem* i = new QListWidgetItem(QString::fromStdString(mi.name), ui->mapbox);
    if (defaultmaps.count(mi.name))
      i->setSelected(true);
    ui->mapbox->addItem(i);
  }

  connect(ui->mapbox, SIGNAL(itemSelectionChanged ()), SLOT(mapboxChanged()));

  installCombo(ui->cont_zorder, zorders, false, 0);

  installColours(ui->land_colorcbox, Colour::getColourInfo(), true);
  installCombo(ui->land_zorder, zorders, false, 0 );
  installColours(ui->backcolorcbox, Colour::getColourInfo(), true);

  MapInfo mi;
  mi.reset();

  if (!m_MapDI.maps.empty())
    mi = m_MapDI.maps.back();

#if 0
    lonb= mi.lon.ison;
    lonlw= mi.lon.linewidth;
    lonlt= mi.lon.linetype;
    lond= ;
    lonz= mi.lon.zorder;
    lonshowvalue= mi.lon.showvalue;
    lonvaluepos = mi.lon.value_pos;
    lon_z= lonz;

    latb= mi.lat.ison;
    latlw= mi.lat.linewidth;
    latlt= mi.lat.linetype;
    latd= mi.lat.density;
    latz= mi.lat.zorder;
    latshowvalue= mi.lat.showvalue;
    latvaluepos = mi.lat.value_pos;
    lat_z= latz;
    lat_dens= 0;

    framelw= mi.frame.linewidth;
    framelt= mi.frame.linetype;
    framez= mi.frame.zorder;
    ff_z= framez;
#endif

  const int lon_dens = density_idx(densities, mi.lon.density);
  const int lat_dens = density_idx(densities, mi.lat.density);

  install_density(ui->lon_density, densities, lon_dens);
  installCombo(ui->lon_zorder, zorders, true, mi.lon.zorder);
  install_density(ui->lat_density, densities, lat_dens);
  installCombo(ui->lat_zorder, zorders, true, mi.lat.zorder);
  installCombo(ui->ff_zorder, zorders, true, mi.frame.zorder);
  installCombo(ui->lon_valuepos, positions_tr, true, 0);
  setLonLatValuePos(ui->lon_valuepos, mi.lon.showvalue, mi.lon.value_pos);
  installCombo(ui->lat_valuepos, positions_tr, true, 0);
  setLonLatValuePos(ui->lat_valuepos, mi.lat.showvalue, mi.lat.value_pos);

  ui->showlon->setChecked(mi.lon.ison);
  lon_checkboxActivated(mi.lon.ison);

  ui->showlat->setChecked(mi.lat.ison);
  lat_checkboxActivated(mi.lat.ison);

  ui->showframe->setChecked(false);
  showframe_checkboxActivated(false);

  mapboxChanged();

  if (favorite.size()==0)
    saveFavoriteClicked(); //default favorite
}

MapDialog::~MapDialog()
{
  delete ui;
}

static void setMapElementOption(LineStyleButton* uiTo, const MapElementOption& mFrom)
{
  uiTo->setLineColor(mFrom.linecolour);
  uiTo->setLineWidth(mFrom.linewidth);
  uiTo->setLineType(mFrom.linetype);
}

void MapDialog::setMapInfoToUi(const MapInfo& mi)
{
  ui->showlon->setChecked(mi.lon.ison);
  setMapElementOption(ui->lon_linestyle, mi.lon);
  ui->lon_density->setCurrentIndex(density_idx(densities, mi.lon.density));
  if (mi.lon.zorder >= 0)
    ui->lon_zorder->setCurrentIndex(mi.lon.zorder);
  setLonLatValuePos(ui->lon_valuepos, mi.lon.showvalue, mi.lon.value_pos);

  ui->showlat->setChecked(mi.lat.ison);
  setMapElementOption(ui->lat_linestyle, mi.lat);
  ui->lat_density->setCurrentIndex(density_idx(densities, mi.lat.density));
  if (mi.lat.zorder >= 0)
    ui->lat_zorder->setCurrentIndex(mi.lat.zorder);
  setLonLatValuePos(ui->lat_valuepos, mi.lat.showvalue, mi.lat.value_pos);

  // set frame options
  ui->showframe->setChecked(mi.frame.ison);
  setMapElementOption(ui->ff_linestyle, mi.frame);
  if (mi.frame.zorder >= 0)
    ui->ff_zorder->setCurrentIndex(mi.frame.zorder);
}

static void getMapElementOption(MapElementOption& mTo, LineStyleButton* uiFrom)
{
  mTo.linecolour = uiFrom->lineColor();
  mTo.linewidth = uiFrom->lineWidth();
  mTo.linetype = uiFrom->lineType();
}

void MapDialog::getMapInfoFromUi(MapInfo& mi)
{
  mi.lon.ison = ui->showlon->isChecked();
  getMapElementOption(mi.lon, ui->lon_linestyle);
  mi.lon.zorder = ui->lon_zorder->currentIndex();
  mi.lon.density = densities[ui->lon_density->currentIndex()] / 1000.0;
  getLonLatValuePos(ui->lon_valuepos, mi.lon.showvalue, mi.lon.value_pos);

  mi.lat.ison = ui->showlat->isChecked();
  getMapElementOption(mi.lat, ui->lat_linestyle);
  mi.lat.zorder = ui->lat_zorder->currentIndex();
  mi.lat.density = densities[ui->lat_density->currentIndex()] / 1000.0;
  getLonLatValuePos(ui->lat_valuepos, mi.lat.showvalue, mi.lat.value_pos);

  mi.frame.ison = ui->showframe->isChecked();;
  getMapElementOption(mi.frame, ui->ff_linestyle);
  mi.frame.zorder = ui->ff_zorder->currentIndex(); // DIFF not in writeLog
}

// FRAME SLOTS

void MapDialog::showframe_checkboxActivated(bool on)
{
  ui->ff_linestyle->setEnabled(on);
  ui->ff_zorder->setEnabled(on);
}

// MAP slots

void MapDialog::mapboxChanged()
{
  METLIBS_LOG_SCOPE();

  int current = -1;
  // find current new map - if any
  if (!selectedmaps.empty()) {
    for (int i = 0; i < numMaps && i < ui->mapbox->count(); i++) {
      if (ui->mapbox->item(i)->isSelected()) {
        if (std::find(selectedmaps.begin(), selectedmaps.end(), i) != selectedmaps.end())
          current = i; // the new map
      }
    }
  }

  selectedmaps.clear();
  int activeidx = -1;
  bool currmapok = false;

  // identify selected maps - keep index to current clicked or previous
  // selected map
  for (int i = 0; i < numMaps && i < ui->mapbox->count(); i++) {
    if (ui->mapbox->item(i)->isSelected()) {
      if (current == i || (!currmapok && activemap == i)) {
        currmapok = true;
        activeidx = selectedmaps.size();
      }
      selectedmaps.push_back(i);
    }
  }
  if (!currmapok && !selectedmaps.empty()) {
    activeidx = 0;
  }

  { // update listbox of selected maps - stop any signals
    diutil::BlockSignals blocked(ui->selectedMapbox);
    ui->selectedMapbox->clear();
    for (int sel : selectedmaps)
      ui->selectedMapbox->addItem(QString::fromStdString(m_MapDI.maps[sel].name));
  }

  if (selectedmaps.empty()) {
    ui->selectedMapbox->setEnabled(false);
    ui->mapdelete->setEnabled(false);
    ui->mapalldelete->setEnabled(false);
    ui->contours->setEnabled(false);
    ui->cont_linestyle->setEnabled(false);
    ui->cont_zorder->setEnabled(false);
    ui->filledland->setEnabled(false);
    ui->land_colorcbox->setEnabled(false);
    ui->land_zorder->setEnabled(false);
  } else {
    // select the active map
    ui->selectedMapbox->setCurrentRow(activeidx);
    ui->selectedMapbox->setEnabled(true);
    ui->mapdelete->setEnabled(true);
    ui->mapalldelete->setEnabled(true);
    selectedMapboxClicked(ui->selectedMapbox->currentItem());
  }
}

void MapDialog::selectedMapboxClicked(QListWidgetItem*)
{
  METLIBS_LOG_SCOPE();
  // new selection in list of selected maps
  // fill all option-widgets for map

  const int index = ui->selectedMapbox->currentRow();
  activemap = selectedmaps[index];

  const MapInfo& am = m_MapDI.maps[activemap];
  const bool isLandMap = (am.type == "triangles" || am.type == "shape");

  ui->contours->setEnabled(isLandMap);
  ui->cont_linestyle->setEnabled(am.contour.ison);
  ui->cont_zorder->setEnabled(am.contour.ison);

  ui->filledland->setEnabled(isLandMap);
  ui->land_colorcbox->setEnabled(isLandMap);
  ui->land_zorder->setEnabled(isLandMap);

  // set contour options
  ui->contours->setChecked(am.contour.ison);
  setMapElementOption(ui->cont_linestyle, am.contour);
  ui->cont_zorder->setCurrentIndex(am.contour.zorder);

  // set filled land options
  ui->filledland->setChecked(am.land.ison);
  SetCurrentItemColourBox(ui->land_colorcbox,am.land.fillcolour);
  ui->land_zorder->setCurrentIndex(am.land.zorder);
}

void MapDialog::mapdeleteClicked()
{
  if (activemap >= 0 && activemap < numMaps) {
    ui->mapbox->item(activemap)->setSelected(false);
  }
  mapboxChanged();
}

void MapDialog::mapalldeleteClicked()
{
  // deselect all maps
  ui->mapbox->clearSelection();
  mapboxChanged();
}

// CONTOUR SLOTS

void MapDialog::cont_checkboxActivated(bool on)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("checkboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.ison = on;

  ui->cont_linestyle->setEnabled(on);
  ui->cont_zorder->setEnabled(on);
}

void MapDialog::cont_linestyleActivated()
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("linetypeboxactivated::Catastrophic: activemap < 0");
    return;
  }
  MapInfo& am = m_MapDI.maps[activemap];
  getMapElementOption(am.contour, ui->cont_linestyle);
}

void MapDialog::cont_zordercboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("zordercboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].contour.zorder = index;
}

// LAND SLOTS


void MapDialog::land_checkboxActivated(bool on)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("checkboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].land.ison = on;

  ui->land_colorcbox->setEnabled(on);
  ui->land_zorder->setEnabled(on);
}

void MapDialog::land_colorcboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("colorcboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].land.fillcolour = ui->land_colorcbox->currentText().toStdString();
}

void MapDialog::land_zordercboxActivated(int index)
{
  if (activemap < 0) {
    METLIBS_LOG_ERROR("zordercboxactivated::Catastrophic: activemap < 0");
    return;
  }
  m_MapDI.maps[activemap].land.zorder = index;
}

// LATLON SLOTS

void MapDialog::lon_checkboxActivated(bool on)
{
  ui->lon_linestyle->setEnabled(on);
  ui->lon_density->setEnabled(on);
  ui->lon_zorder->setEnabled(on);
  ui->lon_valuepos->setEnabled(on);
}

void MapDialog::lat_checkboxActivated(bool on)
{
  ui->lat_linestyle->setEnabled(on);
  ui->lat_density->setEnabled(on);
  ui->lat_zorder->setEnabled(on);
  ui->lat_valuepos->setEnabled(on);
}

/*
 BUTTONS
 */

void MapDialog::saveFavoriteClicked()
{
  // save current status as favorite
  favorite = getOKString();

  // enable this button
  ui->usefavorite->setEnabled(true);
}

void MapDialog::useFavorite()
{
  if ( favorite.size() )
    putOKString(favorite);
}

void MapDialog::useFavoriteClicked()
{
  useFavorite();
}

void MapDialog::applyhideClicked()
{
  emit MapApply();
  emit MapHide();
}

void MapDialog::helpClicked()
{
  emit showsource("ug_mapdialogue.html");
}

void MapDialog::closeEvent(QCloseEvent* e)
{
  emit MapHide();
}

// -------------------------------------------------------
// get and put OK strings
// -------------------------------------------------------

static const std::string AREA = "AREA", MAP = "MAP";

void MapDialog::getLonLatValuePos(QComboBox* combo, bool& show, std::string& value_pos)
{
  METLIBS_LOG_SCOPE();
  // idx == 0 => off
  // idx > 1 && <= positions.size() => value_pos = positions[idx-1]
  const int idx = combo->currentIndex();
  if (idx < 1 || idx > int(positions.size())) {
    show = false;
  } else {
    show = true;
    value_pos = positions[size_t(idx-1)];
  }
  METLIBS_LOG_DEBUG(LOGVAL(idx) << LOGVAL(positions.size()) << LOGVAL(show) << LOGVAL(value_pos));
}

void MapDialog::setLonLatValuePos(QComboBox* combo, bool show, const std::string& value_pos)
{
  // idx == 0 => off
  // idx > 1 && <= positions.size() => value_pos = positions[idx-1]

  int idx = 0;
  if (show) {
    const std::map<std::string,int>::const_iterator it = positions_map.find(value_pos);
    if (it != positions_map.end())
      idx = 1 + it->second;
  }
  combo->setCurrentIndex(idx);
}

PlotCommand_cpv MapDialog::getOKString()
{
  METLIBS_LOG_SCOPE();
  MapManager mapm;
  PlotCommand_cpv vstr;

  //Area string
  if (ui->areabox->currentIndex() > -1) {
    KVListPlotCommand_p pc = std::make_shared<KVListPlotCommand>(AREA);
    pc->add("name", ui->areabox->currentText().toStdString());
    vstr.push_back(pc);
  }

  //Map strings
  int numselected = selectedmaps.size();

  if (numselected == 0 ) { // no maps selected
    // clear log-list
    logmaps.clear();
  }

  std::vector<int> lmaps; // check for logging

  for (int i = 0; i < numselected; i++) {
    const int lindex = selectedmaps[i];
    const MapInfo& mi = m_MapDI.maps[lindex];
    // check if ok to log
    if (mi.logok)
      lmaps.push_back(lindex);

    KVListPlotCommand_p pc = std::make_shared<KVListPlotCommand>(MAP);
    pc->add(mapm.MapInfo2str(mi));
    vstr.push_back(pc);
  }
  if (lmaps.size() > 0)
    logmaps = lmaps;


  // background/lat/lon/frame info
  KVListPlotCommand_p pc = std::make_shared<KVListPlotCommand>(MAP);
  pc->add("backcolour", ui->backcolorcbox->currentText().toStdString());

  MapInfo mi;
  getMapInfoFromUi(mi);

  pc->add(mapm.MapExtra2str(mi));
  vstr.push_back(pc);

  return vstr;
}

static bool get(const KVListPlotCommand_cp& c, const std::string& key, std::string& value_out)
{
  size_t i = c->rfind(key);
  if (i == size_t(-1))
    return false;

  value_out = c->get(i).value();
  return true;
}

void MapDialog::putOKString(const PlotCommand_cpv& vstr)
{
  METLIBS_LOG_SCOPE();
  MapManager mapm;
  std::vector<int> themaps;
  std::string bgcolour, area;
  MapInfo mi;

  for (PlotCommand_cp pc : vstr) {
    KVListPlotCommand_cp c = std::dynamic_pointer_cast<const KVListPlotCommand>(pc);
    if (!c)
      continue;
    // START very similar to readLog
    std::string themap = "";
    if (c->commandKey() == "AREA") {
      get(c, "name", area);
    } else if (c->commandKey() == "MAP" ) {
      get(c, "backcolour", bgcolour);
      get(c, "map", themap);
      get(c, "area", area);
    }

    const size_t i_lon = c->find("lon");
    const bool have_lon = (i_lon != size_t(-1));
    // find the logged map in full list
    if (!themap.empty()) {
      for (int k = 0; k < numMaps; k++) {
        MapInfo& mik = m_MapDI.maps[k];
        if (mik.name == themap) {
          mapm.fillMapInfo(c->all(), mik);
          if (have_lon)
            mi = mik;
          themaps.push_back(k); // keep list of selected maps // DIFF not in readLog
          break;
        }
      }
    } else {
      mapm.fillMapInfo(c->all(), mi);
    }
  }
  // END very similar to readLog

  // reselect area
  int area_index = 0;
  for (unsigned int i = 0; i < m_MapDI.areas.size(); i++)
    if (m_MapDI.areas[i] == area) {
      area_index = i;
      break;
    }
  ui->areabox->setCurrentIndex(area_index);

  // deselect all maps
  ui->mapbox->clearSelection();

  // reselect maps
  for (int m : themaps) {
    ui->mapbox->item(m)->setSelected(true);
  }
  mapboxChanged();

  setMapInfoToUi(mi);

  // set background
  SetCurrentItemColourBox(ui->backcolorcbox,bgcolour);
}

std::string MapDialog::getShortname()
{
  if (ui->areabox->count()== 0) {
    return std::string();
  }

  std::string name = "<font color=\"#009900\">"
      + ui->areabox->currentText().toStdString();
  for (int lindex : selectedmaps) {
    diutil::appendText(name, m_MapDI.maps[lindex].name);
  }
  name += "</font>";

  return name;
}

// ------------------------------------------------
// LOG-FILE read/write methods
// ------------------------------------------------

std::vector<std::string> MapDialog::writeLog()
{
  std::vector<std::string> vstr;
  MapManager mapm;

  // first: write all map-specifications
  for (const MapInfo& mi : m_MapDI.maps) {
    std::ostringstream ostr;
    ostr << mapm.MapInfo2str(mi);
    vstr.push_back(ostr.str());
  }

  // set lon options
  MapInfo mi;
  getMapInfoFromUi(mi);

  //write backcolour/lat/lon/frame
  std::ostringstream ostr;
  ostr << "backcolour=" << ui->backcolorcbox->currentText().toStdString();
  ostr << ' ' << mapm.MapExtra2str(mi);
  vstr.push_back(ostr.str());

  // end of complete map-list
  vstr.push_back("=================== Selected maps =========");

  // write name of current selected (and legal) maps
  int lindex;
  int numselected = logmaps.size();
  for (int i = 0; i < numselected; i++) {
    lindex = logmaps[i];
    vstr.push_back(m_MapDI.maps[lindex].name);
  }

  vstr.push_back("-------------------");

  vstr.push_back("=================== Selected area ============");

  // write name of current selected area
  if (ui->areabox->currentIndex() >= 0)
    vstr.push_back(ui->areabox->currentText().toStdString());

  vstr.push_back("=================== Favorites ============");
  // write favorite options
  for (unsigned int i = 0; i < favorite.size(); i++) {
    vstr.push_back(favorite[i]->toString());
  }

  vstr.push_back("===========================================");

  return vstr;
}

void MapDialog::readLog(const std::vector<std::string>& vstr, const std::string& thisVersion, const std::string& logVersion)
{
  // version-check
  //bool oldversion= (thisVersion!=logVersion && logVersion < "2001-08-25");
  MapManager mapm;

  int n = vstr.size();
  std::vector<int> themaps;
  std::string bgcolour, area;
  MapInfo mi;
  int iline;
  for (iline = 0; iline < n; iline++) {
    std::string str = vstr[iline];
    miutil::trim(str);
    if (not str.empty() && str[0] == '-')
      continue;
    if (not str.empty() && str[0] == '=')
      break;

    // FIXME this is almost identical to putOKString
    const miutil::KeyValue_v kvs = miutil::splitKeyValue(str);
    std::string themap = "";
    for (const miutil::KeyValue& kv : kvs) {
      if (!kv.value().empty()) {
        if (kv.key() == "backcolour")
          bgcolour = kv.value();
        else if (kv.key() == "area") //obsolete syntax
          area = kv.value();
        else if (kv.key() == "map")
          themap = kv.value();
      }
    }
    // find the logged map in full list
    if (not themap.empty()) {
      int idx = -1;
      for (int k = 0; k < numMaps; k++)
        if (m_MapDI.maps[k].name == themap) {
          idx = k;
          break;
        }
      // update options
      if (idx >= 0)
        mapm.fillMapInfo(kvs, m_MapDI.maps[idx]);
    } else {
      mapm.fillMapInfo(kvs, mi);
    }
    // END very similar to putOKString
  }

  // read previous selected maps
  if (iline < n && vstr[iline][0] == '=') {
    themaps.clear();
    iline++;
    for (; iline < n; iline++) {
      std::string themap = vstr[iline];
      miutil::trim(themap);
      if (themap.empty())
        continue;
      if (themap[0] == '=')
        break;
      for (int k = 0; k < numMaps; k++)
        if (m_MapDI.maps[k].name == themap) {
          themaps.push_back(k);
          break;
        }
    }
  }

  //read area
  if (iline < n && vstr[iline][0] == '=') {
    iline++;
    for (; iline < n; iline++) {
      std::string str = vstr[iline];
      miutil::trim(str);
      if (str.empty())
        continue;
      if (str[0] == '=')
        break;
      area = str;
    }
  }

  // read favorite
  favorite.clear();
  if (iline < n && vstr[iline][0] == '=') {
    iline++;
    for (; iline < n; iline++) {
      std::string str = vstr[iline];
      miutil::trim(str);
      if (str.empty())
        continue;
      if (str[0] == '=')
        break;
      miutil::KeyValue_v fkv = miutil::splitKeyValue(str);
      if (fkv.empty())
        continue;
      KVListPlotCommand_p f = std::make_shared<KVListPlotCommand>(fkv.front().key());
      fkv.erase(fkv.begin());
      f->add(fkv);
      favorite.push_back(f);
    }
  }
  ui->usefavorite->setEnabled(favorite.size() > 0);


  // reselect area
  int area_index = 0;
  for (unsigned int i = 0; i < m_MapDI.areas.size(); i++)
    if (m_MapDI.areas[i] == area) {
      area_index = i;
      break;
    }
  ui->areabox->setCurrentIndex(area_index);


  // deselect all maps
  ui->mapbox->clearSelection();

  int nm = themaps.size();
  // reselect maps
  for (int i = 0; i < nm; i++) {
    ui->mapbox->item(themaps[i])->setSelected(true);
  }
  // keep for logging
  logmaps = themaps;

  setMapInfoToUi(mi);

  // set background
  SetCurrentItemColourBox(ui->backcolorcbox,bgcolour);
}
